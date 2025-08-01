/* 
 * CustomLFS_SPIFlash.cpp - SPI Flash support for CustomLFS
 * Based on Paul Stoffregen's LittleFS implementation
 * 
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 
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

#include "CustomLFS_SPIFlash.h"

#define CMD_READ_JEDEC_ID     0x9F
#define CMD_READ_STATUS       0x05
#define CMD_WRITE_ENABLE      0x06
#define CMD_WRITE_DISABLE     0x04
#define CMD_PAGE_PROGRAM      0x02
#define CMD_READ_DATA         0x03
#define CMD_FAST_READ         0x0B
#define CMD_SECTOR_ERASE_4K   0x20
#define CMD_BLOCK_ERASE_32K   0x52
#define CMD_BLOCK_ERASE_64K   0xD8
#define CMD_CHIP_ERASE        0xC7

// Status register bits
#define STATUS_WIP            0x01  // Write in progress
#define STATUS_WEL            0x02  // Write enable latch

// Known SPI Flash chips (subset from Paul Stoffregen's implementation)
const SPIFlashChip spiFlashChips[] = {
  // Winbond chips
  {{0xEF, 0x40, 0x14}, 24, 256, 4096, 0x20, 1048576, 3000, 400000, "W25Q80BV"},
  {{0xEF, 0x40, 0x15}, 24, 256, 4096, 0x20, 2097152, 3000, 400000, "W25Q16JV*Q/W25Q16FV"},
  {{0xEF, 0x40, 0x16}, 24, 256, 4096, 0x20, 4194304, 3000, 400000, "W25Q32JV*Q/W25Q32FV"},
  {{0xEF, 0x40, 0x17}, 24, 256, 4096, 0x20, 8388608, 3000, 400000, "W25Q64JV*Q/W25Q64FV"},
  {{0xEF, 0x40, 0x18}, 24, 256, 4096, 0x20, 16777216, 3000, 400000, "W25Q128JV*Q/W25Q128FV"},
  {{0xEF, 0x40, 0x19}, 32, 256, 4096, 0x20, 33554432, 3000, 400000, "W25Q256JV*Q"},
  {{0xEF, 0x40, 0x20}, 32, 256, 4096, 0x20, 67108864, 3000, 400000, "W25Q512JV*Q"},
  
  // Adesto/Atmel chips  
  {{0x1F, 0x84, 0x01}, 24, 256, 4096, 0x20, 524288, 2500, 300000, "AT25SF041"},
  
  // Spansion chips
  {{0x01, 0x40, 0x15}, 24, 256, 4096, 0x20, 2097152, 1400, 300000, "S25FL208K"},
  
  // Macronix chips
  {{0xC2, 0x20, 0x16}, 24, 256, 4096, 0x20, 4194304, 1400, 300000, "MX25L3233F"},
  {{0xC2, 0x20, 0x17}, 24, 256, 4096, 0x20, 8388608, 1400, 300000, "MX25L6433F"},
};

const uint32_t spiFlashChipCount = sizeof(spiFlashChips) / sizeof(spiFlashChips[0]);

// Extended chip database with advanced features
const SPIFlashChipExt spiFlashChipsExt[] = {
  // P25Q16H - Built from the P25Q16H datasheet
  {
    .total_size = (1UL << 21),           // 2MiB
    .start_up_time_us = 10000,
    .manufacturer_id = 0x85,
    .memory_type = 0x60,
    .capacity = 0x15,
    .max_clock_speed_mhz = 55,
    .quad_enable_bit_mask = 0x02,        // Datasheet p. 27
    .has_sector_protection = 1,          // Datasheet p. 27
    .supports_fast_read = 1,             // Datasheet p. 29
    .supports_qspi = 1,                  // Obviously
    .supports_qspi_writes = 1,           // Datasheet p. 41
    .write_status_register_split = 1,    // Datasheet p. 28
    .single_status_byte = 0,             // 2 bytes
    .is_fram = 0,                        // Flash Memory
    .name = "P25Q16H"
  },
  
  // Add more extended chips here as needed
};

const uint32_t spiFlashChipExtCount = sizeof(spiFlashChipsExt) / sizeof(spiFlashChipsExt[0]);

// Global instances
CustomLFS_SPIFlash SPIFlash;
CustomLFS_SPIFlash FlashFS;  // Alternative name for compatibility

//--------------------------------------------------------------------+
// SPI Flash IO Callbacks
//--------------------------------------------------------------------+

int CustomLFS_SPIFlash::_spi_read(const struct lfs_config *c, lfs_block_t block, 
                                  lfs_off_t off, void *buffer, lfs_size_t size)
{
  CustomLFS_SPIFlash* fs = (CustomLFS_SPIFlash*)c->context;
  uint32_t addr = block * c->block_size + off;
  
  if (!fs->flashRead(addr, buffer, size)) {
    return -1;
  }
  return 0;
}

int CustomLFS_SPIFlash::_spi_prog(const struct lfs_config *c, lfs_block_t block, 
                                  lfs_off_t off, const void *buffer, lfs_size_t size)
{
  CustomLFS_SPIFlash* fs = (CustomLFS_SPIFlash*)c->context;
  uint32_t addr = block * c->block_size + off;
  
  if (!fs->flashProgram(addr, buffer, size)) {
    return -1;
  }
  return 0;
}

int CustomLFS_SPIFlash::_spi_erase(const struct lfs_config *c, lfs_block_t block)
{
  CustomLFS_SPIFlash* fs = (CustomLFS_SPIFlash*)c->context;
  uint32_t addr = block * c->block_size;
  
  if (!fs->flashErase(addr)) {
    return -1;
  }
  return 0;
}

int CustomLFS_SPIFlash::_spi_sync(const struct lfs_config *c)
{
  CustomLFS_SPIFlash* fs = (CustomLFS_SPIFlash*)c->context;
  fs->flashSync();
  return 0;
}

//--------------------------------------------------------------------+
// CustomLFS_SPIFlash Implementation
//--------------------------------------------------------------------+

CustomLFS_SPIFlash::CustomLFS_SPIFlash(uint8_t csPin, SPIClass &spiPort)
  : CustomLFS(false)  // Call base constructor without auto-configuration
  , _csPin(csPin)
  , _spi(&spiPort)
  , _spiSettings(1000000, MSBFIRST, SPI_MODE0)  // Default 1MHz
  , _chip(nullptr)
  , _chipExt(nullptr)
  , _totalSize(0)
  , _sectorSize(4096)  // Most SPI flash uses 4KB sectors
  , _is4ByteAddr(false)
  , _flashReady(false)
  , _supportsFastRead(false)
  , _supportsQSPI(false)
  , _maxClockMHz(1)
{
  // Initialize base class members but don't configure LFS yet
  _flash_addr = 0;
  _flash_total_size = 0;
  _block_size = 4096;
}

bool CustomLFS_SPIFlash::begin(uint8_t csPin, SPIClass &spiPort)
{
  if (csPin != 255) {
    _csPin = csPin;
  }
  _spi = &spiPort;
  
  if (_csPin == 255) {
    return false;  // Must have valid CS pin
  }
  
  // Initialize SPI
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);
  _spi->begin();
  
  // Detect flash chip
  if (!detectChip()) {
    return false;
  }
  
  // Configure LittleFS for this flash chip
  _configure_lfs();
  
  _flashReady = true;
  
  // Try to mount the filesystem
  return CustomLFS::begin();
}

bool CustomLFS_SPIFlash::detectChip()
{
  uint8_t id[3];
  
  if (!readJEDECID(id)) {
    return false;
  }
  
  // First check extended chip database
  for (uint32_t i = 0; i < spiFlashChipExtCount; i++) {
    if (id[0] == spiFlashChipsExt[i].manufacturer_id && 
        id[1] == spiFlashChipsExt[i].memory_type && 
        id[2] == spiFlashChipsExt[i].capacity) {
      _chipExt = &spiFlashChipsExt[i];
      _chip = nullptr;  // Using extended format
      _totalSize = _chipExt->total_size;
      _sectorSize = 4096;  // Standard for most chips
      _is4ByteAddr = (_totalSize > 16777216);  // >16MB needs 4-byte addressing
      _supportsFastRead = _chipExt->supports_fast_read;
      _supportsQSPI = _chipExt->supports_qspi;
      _maxClockMHz = _chipExt->max_clock_speed_mhz;
      
      Serial.print("Detected extended chip: ");
      Serial.print(_chipExt->name);
      Serial.print(" (");
      Serial.print(_totalSize / 1024);
      Serial.print(" KB, max ");
      Serial.print(_maxClockMHz);
      Serial.println(" MHz)");
      
      return true;
    }
  }
  
  // Then check standard chip database
  for (uint32_t i = 0; i < spiFlashChipCount; i++) {
    if (id[0] == spiFlashChips[i].id[0] && 
        id[1] == spiFlashChips[i].id[1] && 
        id[2] == spiFlashChips[i].id[2]) {
      _chip = &spiFlashChips[i];
      _chipExt = nullptr;  // Using standard format
      _totalSize = _chip->totalsize;
      _sectorSize = _chip->erasesize;
      _is4ByteAddr = (_chip->addrbits == 32);
      _supportsFastRead = true;  // Assume supported for standard chips
      _supportsQSPI = false;     // Conservative default
      _maxClockMHz = 25;         // Conservative default
      
      Serial.print("Detected standard chip: ");
      Serial.print(_chip->name);
      Serial.print(" (");
      Serial.print(_totalSize / 1024);
      Serial.println(" KB)");
      
      return true;
    }
  }
  
  // Unknown chip - try to use it anyway with basic parameters
  _chip = nullptr;
  _chipExt = nullptr;
  _totalSize = 1048576;  // Assume 1MB
  _sectorSize = 4096;    // Standard 4KB sectors
  _is4ByteAddr = false;
  _supportsFastRead = false;
  _supportsQSPI = false;
  _maxClockMHz = 1;      // Very conservative
  
  Serial.print("Unknown chip with JEDEC ID: 0x");
  Serial.print(id[0], HEX);
  Serial.print(" 0x");
  Serial.print(id[1], HEX);
  Serial.print(" 0x");
  Serial.print(id[2], HEX);
  Serial.println(" - using default parameters");
  
  return true;  // Return true to allow operation with unknown chips
}

bool CustomLFS_SPIFlash::readJEDECID(uint8_t *id)
{
  _spi->beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  
  _spi->transfer(CMD_READ_JEDEC_ID);
  id[0] = _spi->transfer(0);
  id[1] = _spi->transfer(0);
  id[2] = _spi->transfer(0);
  
  digitalWrite(_csPin, HIGH);
  _spi->endTransaction();
  
  // Valid JEDEC ID should not be all 0x00 or all 0xFF
  return (id[0] != 0x00 && id[0] != 0xFF);
}

bool CustomLFS_SPIFlash::waitReady(uint32_t timeout_ms)
{
  uint32_t start = millis();
  
  while ((millis() - start) < timeout_ms) {
    if ((readStatus() & STATUS_WIP) == 0) {
      return true;
    }
    delayMicroseconds(100);
  }
  
  return false;
}

void CustomLFS_SPIFlash::writeEnable()
{
  _spi->beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi->transfer(CMD_WRITE_ENABLE);
  digitalWrite(_csPin, HIGH);
  _spi->endTransaction();
}

void CustomLFS_SPIFlash::writeDisable()
{
  _spi->beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi->transfer(CMD_WRITE_DISABLE);
  digitalWrite(_csPin, HIGH);
  _spi->endTransaction();
}

uint8_t CustomLFS_SPIFlash::readStatus()
{
  uint8_t status;
  
  _spi->beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi->transfer(CMD_READ_STATUS);
  status = _spi->transfer(0);
  digitalWrite(_csPin, HIGH);
  _spi->endTransaction();
  
  return status;
}

void CustomLFS_SPIFlash::sendAddress(uint32_t addr)
{
  if (_is4ByteAddr) {
    _spi->transfer((addr >> 24) & 0xFF);
  }
  _spi->transfer((addr >> 16) & 0xFF);
  _spi->transfer((addr >> 8) & 0xFF);
  _spi->transfer(addr & 0xFF);
}

bool CustomLFS_SPIFlash::flashRead(uint32_t addr, void *buffer, uint32_t size)
{
  if (!_flashReady || !buffer || size == 0) {
    return false;
  }
  
  uint8_t *buf = (uint8_t*)buffer;
  
  _spi->beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  
  if (_supportsFastRead) {
    // Use fast read command (0x0B) for better performance
    _spi->transfer(CMD_FAST_READ);
    sendAddress(addr);
    _spi->transfer(0);  // Dummy byte for fast read
  } else {
    // Use standard read command (0x03)
    _spi->transfer(CMD_READ_DATA);
    sendAddress(addr);
  }
  
  for (uint32_t i = 0; i < size; i++) {
    buf[i] = _spi->transfer(0);
  }
  
  digitalWrite(_csPin, HIGH);
  _spi->endTransaction();
  
  return true;
}

bool CustomLFS_SPIFlash::flashProgram(uint32_t addr, const void *buffer, uint32_t size)
{
  if (!_flashReady || !buffer || size == 0) {
    return false;
  }
  
  const uint8_t *buf = (const uint8_t*)buffer;
  uint32_t pageSize = 256;  // Standard page size for most SPI flash
  
  // Use chip-specific page size if available
  if (_chip && _chip->progsize > 0) {
    pageSize = _chip->progsize;
  }
  
  // Program in page-sized chunks
  while (size > 0) {
    uint32_t pageAddr = addr & ~(pageSize - 1);
    uint32_t pageOffset = addr - pageAddr;
    uint32_t chunkSize = pageSize - pageOffset;
    if (chunkSize > size) chunkSize = size;
    
    // Wait for any previous operation to complete
    if (!waitReady()) {
      return false;
    }
    
    // Enable write
    writeEnable();
    
    // Program page
    _spi->beginTransaction(_spiSettings);
    digitalWrite(_csPin, LOW);
    
    _spi->transfer(CMD_PAGE_PROGRAM);
    sendAddress(addr);
    
    for (uint32_t i = 0; i < chunkSize; i++) {
      _spi->transfer(buf[i]);
    }
    
    digitalWrite(_csPin, HIGH);
    _spi->endTransaction();
    
    // Update pointers
    addr += chunkSize;
    buf += chunkSize;
    size -= chunkSize;
  }
  
  return waitReady();
}

bool CustomLFS_SPIFlash::flashErase(uint32_t addr)
{
  if (!_flashReady) {
    return false;
  }
  
  // Wait for any previous operation to complete
  if (!waitReady()) {
    return false;
  }
  
  // Enable write
  writeEnable();
  
  // Erase sector
  _spi->beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  
  _spi->transfer(CMD_SECTOR_ERASE_4K);  // Use 4KB sector erase
  sendAddress(addr);
  
  digitalWrite(_csPin, HIGH);
  _spi->endTransaction();
  
  return waitReady();
}

void CustomLFS_SPIFlash::flashSync()
{
  // Wait for any pending operations
  waitReady();
}

void CustomLFS_SPIFlash::_configure_lfs()
{
  // Clear the configuration structure
  memset(&_lfs_config, 0, sizeof(_lfs_config));
  
  // Set up the configuration
  _lfs_config.context = this;
  
  // Block device operations
  _lfs_config.read = _spi_read;
  _lfs_config.prog = _spi_prog;
  _lfs_config.erase = _spi_erase;
  _lfs_config.sync = _spi_sync;
  
  // Block device configuration
  _lfs_config.read_size = 1;                    // Minimum read size
  _lfs_config.prog_size = 1;                    // Minimum program size  
  _lfs_config.block_size = _sectorSize;         // Erase size (typically 4KB)
  _lfs_config.block_count = _totalSize / _sectorSize;
  _lfs_config.lookahead = 128;                  // Lookahead buffer size
  
  // Cache configuration (optional - can be NULL for dynamic allocation)
  _lfs_config.read_buffer = NULL;
  _lfs_config.prog_buffer = NULL;
  _lfs_config.lookahead_buffer = NULL;
  _lfs_config.file_buffer = NULL;
  
  // Update parent class members to match
  _flash_addr = 0;  // SPI flash starts at address 0
  _flash_total_size = _totalSize;
  _block_size = _sectorSize;
}

bool CustomLFS_SPIFlash::setCSPin(uint8_t csPin)
{
  if (_flashReady) {
    return false;  // Can't change after initialization
  }
  _csPin = csPin;
  return true;
}

bool CustomLFS_SPIFlash::setSPIPort(SPIClass &spiPort)
{
  if (_flashReady) {
    return false;  // Can't change after initialization
  }
  _spi = &spiPort;
  return true;
}

bool CustomLFS_SPIFlash::setSPISpeed(uint32_t speed)
{
  // Limit speed to chip's maximum if known
  uint32_t maxSpeed = (uint32_t)_maxClockMHz * 1000000;
  if (_maxClockMHz > 0 && speed > maxSpeed) {
    Serial.print("Warning: Requested SPI speed (");
    Serial.print(speed / 1000000);
    Serial.print(" MHz) exceeds chip maximum (");
    Serial.print(_maxClockMHz);
    Serial.print(" MHz). Using ");
    Serial.print(_maxClockMHz);
    Serial.println(" MHz.");
    speed = maxSpeed;
  }
  
  _spiSettings = SPISettings(speed, MSBFIRST, SPI_MODE0);
  return true;
}

bool CustomLFS_SPIFlash::lowLevelFormat()
{
  if (!_flashReady) {
    return false;
  }
  
  // Unmount if mounted
  File testRoot = open("/");
  bool isMounted = testRoot;
  if (testRoot) testRoot.close();
  
  if (isMounted) {
    end();
  }
  
  // Erase all sectors
  for (uint32_t addr = 0; addr < _totalSize; addr += _sectorSize) {
    if (!flashErase(addr)) {
      return false;
    }
  }
  
  // Format the filesystem
  return format();
}

bool CustomLFS_SPIFlash::testFlash()
{
  if (!_flashReady) {
    return false;
  }
  
  // Simple test: write and read back a pattern
  uint8_t testData[256];
  uint8_t readData[256];
  
  // Fill with test pattern
  for (int i = 0; i < 256; i++) {
    testData[i] = i ^ 0xA5;
  }
  
  // Use last sector for test
  uint32_t testAddr = _totalSize - _sectorSize;
  
  // Erase test sector
  if (!flashErase(testAddr)) {
    return false;
  }
  
  // Write test data
  if (!flashProgram(testAddr, testData, sizeof(testData))) {
    return false;
  }
  
  // Read back data
  if (!flashRead(testAddr, readData, sizeof(readData))) {
    return false;
  }
  
  // Compare
  for (int i = 0; i < 256; i++) {
    if (testData[i] != readData[i]) {
      return false;
    }
  }
  
  return true;
}