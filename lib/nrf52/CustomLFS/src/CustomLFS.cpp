/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 hathach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "CustomLFS.h"

// Global instance for backward compatibility
CustomLFS CustomFS;

//--------------------------------------------------------------------+
// LFS Disk IO Callbacks
//--------------------------------------------------------------------+

int CustomLFS::_flash_read(const struct lfs_config *c, lfs_block_t block, 
                           lfs_off_t off, void *buffer, lfs_size_t size)
{
  CustomLFS* fs = (CustomLFS*)c->context;
  uint32_t addr = fs->lba2addr(block) + off;
  
  VERIFY(flash_nrf5x_read(buffer, addr, size) > 0, -1);
  return 0;
}

int CustomLFS::_flash_prog(const struct lfs_config *c, lfs_block_t block, 
                           lfs_off_t off, const void *buffer, lfs_size_t size)
{
  CustomLFS* fs = (CustomLFS*)c->context;
  uint32_t addr = fs->lba2addr(block) + off;
  
  VERIFY(flash_nrf5x_write(addr, buffer, size), -1);
  return 0;
}

int CustomLFS::_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
  CustomLFS* fs = (CustomLFS*)c->context;
  uint32_t addr = fs->lba2addr(block);

  // Implement as write 0xff to whole block address
  for(int i = 0; i < fs->_block_size; i++)
  {
    flash_nrf5x_write8(addr + i, 0xFF);
  }

  return 0;
}

int CustomLFS::_flash_sync(const struct lfs_config *c)
{
  (void) c;
  flash_nrf5x_flush();
  return 0;
}

//--------------------------------------------------------------------+
// CustomLFS Implementation
//--------------------------------------------------------------------+

CustomLFS::CustomLFS(void)
  : Adafruit_LittleFS(&_lfs_config)
  , _flash_addr(LFS_DEFAULT_FLASH_ADDR)
  , _flash_total_size(LFS_DEFAULT_FLASH_TOTAL_SIZE)
  , _block_size(LFS_DEFAULT_BLOCK_SIZE)
{
  _configure_lfs();
}

CustomLFS::CustomLFS(uint32_t flash_addr, uint32_t flash_size, uint32_t block_size)
  : Adafruit_LittleFS(&_lfs_config)
  , _flash_addr(flash_addr)
  , _flash_total_size(flash_size)
  , _block_size(block_size)
{
  // Validate the configuration
  if (!validateFlashRegion(flash_addr, flash_size, block_size)) {
    // Fall back to default configuration if invalid
    _flash_addr = LFS_DEFAULT_FLASH_ADDR;
    _flash_total_size = LFS_DEFAULT_FLASH_TOTAL_SIZE;
    _block_size = LFS_DEFAULT_BLOCK_SIZE;
  }
  
  _configure_lfs();
}

CustomLFS::CustomLFS(bool auto_configure)
  : Adafruit_LittleFS(&_lfs_config)
  , _flash_addr(0)
  , _flash_total_size(0)
  , _block_size(0)
{
  // Clear the config but don't configure if auto_configure is false
  memset(&_lfs_config, 0, sizeof(_lfs_config));
  if (auto_configure) {
    _flash_addr = LFS_DEFAULT_FLASH_ADDR;
    _flash_total_size = LFS_DEFAULT_FLASH_TOTAL_SIZE;
    _block_size = LFS_DEFAULT_BLOCK_SIZE;
    _configure_lfs();
  }
}

void CustomLFS::_configure_lfs()
{
  // Clear the configuration structure
  memset(&_lfs_config, 0, sizeof(_lfs_config));
  
  // Set up the configuration
  _lfs_config.context = this;
  
  // Block device operations
  _lfs_config.read = _flash_read;
  _lfs_config.prog = _flash_prog;
  _lfs_config.erase = _flash_erase;
  _lfs_config.sync = _flash_sync;
  
  // Block device configuration
  _lfs_config.read_size = _block_size;
  _lfs_config.prog_size = _block_size;
  _lfs_config.block_size = _block_size;
  _lfs_config.block_count = _flash_total_size / _block_size;
  _lfs_config.lookahead = 128;
  
  // Buffers (set to NULL for dynamic allocation)
  _lfs_config.read_buffer = NULL;
  _lfs_config.prog_buffer = NULL;
  _lfs_config.lookahead_buffer = NULL;
  _lfs_config.file_buffer = NULL;
  
  // Note: The parent class constructor already received our config pointer
  // so no need to call _setConfig() - it's already using our _lfs_config
}

bool CustomLFS::setFlashRegion(uint32_t flash_addr, uint32_t flash_size, uint32_t block_size)
{
  // Check if filesystem is already mounted by trying to check if we can access it
  // This is a workaround since we don't have direct access to _begun
  File testRoot = open("/");
  bool isMounted = testRoot;
  if (testRoot) testRoot.close();
  
  if (isMounted) {
    return false;  // Cannot change configuration if already mounted
  }
  
  // Validate the configuration
  if (!validateFlashRegion(flash_addr, flash_size, block_size)) {
    return false;
  }
  
  // Update configuration
  _flash_addr = flash_addr;
  _flash_total_size = flash_size;
  _block_size = block_size;
  
  // Reconfigure LFS
  _configure_lfs();
  
  return true;
}

bool CustomLFS::validateFlashRegion(uint32_t flash_addr, uint32_t flash_size, uint32_t block_size)
{
  // Basic validation
  if (flash_addr == 0 || flash_size == 0 || block_size == 0) {
    return false;
  }
  
  // Check alignment - flash address should be page-aligned for efficiency
  if (flash_addr % FLASH_NRF52_PAGE_SIZE != 0) {
    return false;
  }
  
  // Block size should be reasonable (at least 16 bytes, at most page size)
  if (block_size < 16 || block_size > FLASH_NRF52_PAGE_SIZE) {
    return false;
  }
  
  // Flash size should be multiple of block size
  if (flash_size % block_size != 0) {
    return false;
  }
  
  // Check if region is within valid flash range
  // nRF52840 has 1MB flash, nRF52832 has 512KB
  #ifdef NRF52840_XXAA
    uint32_t max_flash = 0x100000;  // 1MB
  #else
    uint32_t max_flash = 0x80000;   // 512KB
  #endif
  
  if (flash_addr >= max_flash || (flash_addr + flash_size) > max_flash) {
    return false;
  }
  
  return true;
}

bool CustomLFS::begin(void)
{
  // Try to mount the filesystem
  if (!Adafruit_LittleFS::begin()) {
    // Failed to mount, erase all sectors in our flash region and format
    for (uint32_t addr = _flash_addr; addr < _flash_addr + _flash_total_size; addr += FLASH_NRF52_PAGE_SIZE) {
      VERIFY(flash_nrf5x_erase(addr));
    }

    // Format the filesystem
    this->format();

    // Try to mount again - if this fails, give up
    if (!Adafruit_LittleFS::begin()) {
      return false;
    }
  }

  return true;
}

bool CustomLFS::formatRegion(void)
{
  // Check if filesystem is mounted and unmount if necessary
  File testRoot = open("/");
  bool isMounted = testRoot;
  if (testRoot) testRoot.close();
  
  if (isMounted) {
    end();
  }
  
  // Erase all sectors in our flash region
  for (uint32_t addr = _flash_addr; addr < _flash_addr + _flash_total_size; addr += FLASH_NRF52_PAGE_SIZE) {
    if (!flash_nrf5x_erase(addr)) {
      return false;
    }
  }
  
  // Format the filesystem
  return format();
}