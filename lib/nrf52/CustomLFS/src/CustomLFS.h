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

// CustomLFS.h - Custom LittleFS wrapper to support arbitrary flash regions
#ifndef CUSTOMLFS_H_
#define CUSTOMLFS_H_

#include "Adafruit_LittleFS.h"
#include "flash/flash_nrf5x.h"

using namespace Adafruit_LittleFS_Namespace;

// Default flash configuration (maintains backward compatibility)
#ifdef NRF52840_XXAA
  #define LFS_DEFAULT_FLASH_ADDR        0xED000
#else
  #define LFS_DEFAULT_FLASH_ADDR        0x6D000
#endif

#define LFS_DEFAULT_FLASH_TOTAL_SIZE  (7*FLASH_NRF52_PAGE_SIZE)
#define LFS_DEFAULT_BLOCK_SIZE        128

class CustomLFS : public Adafruit_LittleFS
{
protected:
  // Flash region configuration - now protected for derived classes
  uint32_t _flash_addr;
  uint32_t _flash_total_size;
  uint32_t _block_size;
  
  // LFS configuration structure (one per instance) - now protected
  struct lfs_config _lfs_config;
  
  // Helper function to convert block to address
  uint32_t lba2addr(uint32_t block) const {
    return _flash_addr + block * _block_size;
  }
  
  // Configure the LFS config structure - now protected for derived classes
  virtual void _configure_lfs();

private:
  // Static callback functions for LittleFS operations
  static int _flash_read(const struct lfs_config *c, lfs_block_t block, 
                         lfs_off_t off, void *buffer, lfs_size_t size);
  static int _flash_prog(const struct lfs_config *c, lfs_block_t block, 
                         lfs_off_t off, const void *buffer, lfs_size_t size);
  static int _flash_erase(const struct lfs_config *c, lfs_block_t block);
  static int _flash_sync(const struct lfs_config *c);
  
  bool _begun = false;

public:
  // Default constructor (backward compatibility)
  CustomLFS(void);
  
  // Constructor with custom flash region
  CustomLFS(uint32_t flash_addr, uint32_t flash_size, 
            uint32_t block_size = LFS_DEFAULT_BLOCK_SIZE);
  
  // Protected constructor for derived classes (doesn't auto-configure)
  CustomLFS(bool auto_configure);
  
  // Set flash region (must be called before begin())
  bool setFlashRegion(uint32_t flash_addr, uint32_t flash_size, 
                      uint32_t block_size = LFS_DEFAULT_BLOCK_SIZE);
  
  // Get current configuration
  uint32_t getFlashAddr(void) const { return _flash_addr; }
  uint32_t getFlashSize(void) const { return _flash_total_size; }
  uint32_t getBlockSize(void) const { return _block_size; }
  
  // Begin to handle custom flash regions
  virtual bool begin(void);
  
  // Format the specific flash region
  bool formatRegion(void);
  
  // Validation helper
  static bool validateFlashRegion(uint32_t flash_addr, uint32_t flash_size, uint32_t block_size);
};

// Global instance for backward compatibility
extern CustomLFS CustomFS;

#endif /* CUSTOMLFS_H_ */