/* 
 * CustomLFS_SPIFlash.h - SPI Flash support for CustomLFS
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

#ifndef CUSTOMLFS_SPIFLASH_H_
#define CUSTOMLFS_SPIFLASH_H_

#include "CustomLFS.h"
#include <SPI.h>

using namespace Adafruit_LittleFS_Namespace;

// SPI Flash chip information structure (compatible with multiple formats)
struct SPIFlashChip {
  uint8_t id[3];           // JEDEC ID bytes [manufacturer, memory_type, capacity]
  uint8_t addrbits;        // Address bits (24 or 32)
  uint16_t progsize;       // Page program size
  uint32_t erasesize;      // Sector erase size
  uint8_t erasecmd;        // Erase command
  uint32_t totalsize;      // Total flash size in bytes
  uint16_t tprog;          // Page program time (µs)
  uint32_t terase;         // Sector erase time (µs)
  const char *name;        // Chip name
};

// Extended chip info for advanced features (optional)
struct SPIFlashChipExt {
  uint32_t total_size;
  uint32_t start_up_time_us;
  uint8_t manufacturer_id;
  uint8_t memory_type;
  uint8_t capacity;
  uint8_t max_clock_speed_mhz;
  uint8_t quad_enable_bit_mask;
  uint8_t has_sector_protection : 1;
  uint8_t supports_fast_read : 1;
  uint8_t supports_qspi : 1;
  uint8_t supports_qspi_writes : 1;
  uint8_t write_status_register_split : 1;
  uint8_t single_status_byte : 1;
  uint8_t is_fram : 1;
  const char *name;
};

class CustomLFS_SPIFlash : public CustomLFS
{
private:
  // SPI configuration
  uint8_t _csPin;
  SPIClass *_spi;
  SPISettings _spiSettings;
  
  // Flash chip information
  const SPIFlashChip *_chip;
  const SPIFlashChipExt *_chipExt;  // Extended chip info if available
  uint32_t _totalSize;
  uint32_t _sectorSize;
  bool _is4ByteAddr;
  
  // Flash state
  bool _flashReady;
  
  // Enhanced features
  bool _supportsFastRead;
  bool _supportsQSPI;
  uint8_t _maxClockMHz;
  
  // Flash operation methods
  bool detectChip();
  bool waitReady(uint32_t timeout_ms = 10000);
  void writeEnable();
  void writeDisable();
  uint8_t readStatus();
  bool readJEDECID(uint8_t *id);
  
  // Flash I/O operations
  bool flashRead(uint32_t addr, void *buffer, uint32_t size);
  bool flashProgram(uint32_t addr, const void *buffer, uint32_t size);
  bool flashErase(uint32_t addr);
  void flashSync();
  
  // Address handling for 3-byte vs 4-byte addressing
  void sendAddress(uint32_t addr);
  
  // Static callback functions for LittleFS operations
  static int _spi_read(const struct lfs_config *c, lfs_block_t block, 
                       lfs_off_t off, void *buffer, lfs_size_t size);
  static int _spi_prog(const struct lfs_config *c, lfs_block_t block, 
                       lfs_off_t off, const void *buffer, lfs_size_t size);
  static int _spi_erase(const struct lfs_config *c, lfs_block_t block);
  static int _spi_sync(const struct lfs_config *c);
  
  // Configure LFS for SPI flash
  virtual void _configure_lfs() override;

public:
  // Constructor
  CustomLFS_SPIFlash(uint8_t csPin = 255, SPIClass &spiPort = SPI);
  
  // Initialization
  bool begin(uint8_t csPin = 255, SPIClass &spiPort = SPI);
  
  // Configuration
  bool setCSPin(uint8_t csPin);
  bool setSPIPort(SPIClass &spiPort);
  bool setSPISpeed(uint32_t speed);
  
  // Flash information
  uint32_t getFlashSize() const { return _totalSize; }
  uint32_t getSectorSize() const { return _sectorSize; }
  const char* getChipName() const { 
    if (_chipExt) return _chipExt->name;
    if (_chip) return _chip->name;
    return "Unknown";
  }
  bool isReady() const { return _flashReady; }
  bool supportsFastRead() const { return _supportsFastRead; }
  bool supportsQSPI() const { return _supportsQSPI; }
  uint8_t getMaxClockMHz() const { return _maxClockMHz; }
  
  // Low-level flash operations (for advanced users)
  bool lowLevelFormat();
  bool testFlash();
  
  // Override parent class methods that might conflict
  bool setFlashRegion(uint32_t flash_addr, uint32_t flash_size, 
                      uint32_t block_size = 0) = delete; // Not applicable for SPI flash
};

// Known SPI flash chips database (from Paul Stoffregen's implementation)
extern const SPIFlashChip spiFlashChips[];
extern const uint32_t spiFlashChipCount;

// Extended chip database (for advanced chips like P25Q16H)
extern const SPIFlashChipExt spiFlashChipsExt[];
extern const uint32_t spiFlashChipExtCount;

// Global instance for convenience (multiple naming options)
extern CustomLFS_SPIFlash SPIFlash;
extern CustomLFS_SPIFlash FlashFS;  // Alternative name for compatibility

// Type alias for easier usage
typedef CustomLFS_SPIFlash LittleFS_SPIFlash;

#endif /* CUSTOMLFS_SPIFLASH_H_ */