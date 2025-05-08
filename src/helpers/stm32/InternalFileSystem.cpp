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
    
#include <Arduino.h>
#include "InternalFileSystem.h"

#ifdef LFS_DEBUG_LOGGING
    #define LFS_DEBUG_PRINTLN(F, ...) Serial.printf("LFS: " F);Serial.println("");
#else
    #define LFS_DEBUG_PRINTLN(...) {}
#endif

//--------------------------------------------------------------------+
// LFS Disk IO
//--------------------------------------------------------------------+
static int _internal_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{    
    if (!buffer || !size) {
        LFS_DEBUG_PRINTLN("%s Invalid parameter!\r\n", __func__);
        return LFS_ERR_INVAL;
    }
     
    lfs_block_t address = LFS_FLASH_ADDR_BASE + (block * FLASH_PAGE_SIZE + off);
    memcpy(buffer, (void *)address, size);
     
    return LFS_ERR_OK;
}    

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int _internal_flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{    
    lfs_block_t address = LFS_FLASH_ADDR_BASE + (block * FLASH_PAGE_SIZE + off);
    HAL_StatusTypeDef hal_rc = HAL_OK;
    uint32_t dw_count = size / 8;
    uint64_t *bufp = (uint64_t *)buffer;
     
    LFS_DEBUG_PRINTLN("Programming %d bytes/%d doublewords at address 0x%08x/block %d, offset %d.", size, dw_count, address, block, off);
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return LFS_ERR_IO;
    }
    for (uint32_t i = 0; i < dw_count; i++) {
        if ((address < LFS_FLASH_ADDR_BASE) || (address > FLASH_END_ADDR)) {
            LFS_DEBUG_PRINTLN("Wanted to program out of bound of FLASH: 0x%08x.\n", address);
            HAL_FLASH_Lock();
            return LFS_ERR_INVAL;
        }
        hal_rc = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, *bufp);
        if (hal_rc != HAL_OK) {
            LFS_DEBUG_PRINTLN("Program error at (0x%08x), 0x%X, error: 0x%08x\n", address, hal_rc, HAL_FLASH_GetError());
        }
        address += 8;
        bufp += 1;
    }
    if (HAL_FLASH_Lock() != HAL_OK) {
        return LFS_ERR_IO;
    }
     
    return hal_rc == HAL_OK ? LFS_ERR_OK : LFS_ERR_IO;
    // If HAL_OK, return LFS_ERR_OK, else return LFS_ERR_IO
}    

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int _internal_flash_erase(const struct lfs_config *c, lfs_block_t block)
{   
    lfs_block_t address = LFS_FLASH_ADDR_BASE + (block * FLASH_PAGE_SIZE);
    HAL_StatusTypeDef hal_rc;
    uint32_t pageError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct = {
        .TypeErase = FLASH_TYPEERASE_PAGES, 
        .Page = 0, 
        .NbPages = 1
    };
    
    if ((address < LFS_FLASH_ADDR_BASE) || (address > FLASH_END_ADDR)) {
        LFS_DEBUG_PRINTLN("Wanted to erase out of bound of FLASH: 0x%08x.\n", address);
        return LFS_ERR_INVAL;
    }
    /* calculate the absolute page, i.e. what the ST wants */
    EraseInitStruct.Page = (address - FLASH_BASE) / FLASH_PAGE_SIZE;
    LFS_DEBUG_PRINTLN("Erasing block %d at 0x%08x... ", block, address);
    HAL_FLASH_Unlock();
    hal_rc = HAL_FLASHEx_Erase(&EraseInitStruct, &pageError);
    HAL_FLASH_Lock();

    return hal_rc == HAL_OK ? LFS_ERR_OK : LFS_ERR_IO; 
    // If HAL_OK, return LFS_ERR_OK, else return LFS_ERR_IO
}   

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int _internal_flash_sync(const struct lfs_config *c)
{    
    // write function performs no caching.  No need for sync.
    return LFS_ERR_OK;
}    

struct lfs_config _InternalFSConfig = {
    .context = NULL, 
    .read = _internal_flash_read,
    .prog = _internal_flash_prog,
    .erase = _internal_flash_erase,
    .sync = _internal_flash_sync,

    .read_size = LFS_BLOCK_SIZE,
    .prog_size = LFS_BLOCK_SIZE,
    .block_size = LFS_BLOCK_SIZE,
    .block_count = LFS_FLASH_TOTAL_SIZE / LFS_BLOCK_SIZE,
    .lookahead = 128,

    .read_buffer = NULL,
    .prog_buffer = NULL,
    .lookahead_buffer = NULL,
    .file_buffer = NULL
};

InternalFileSystem InternalFS;

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+

InternalFileSystem::InternalFileSystem(void)
  : Adafruit_LittleFS(&_InternalFSConfig)
{

}

bool InternalFileSystem::begin(void)
{
  // failed to mount, erase all sector then format and mount again
  if ( !Adafruit_LittleFS::begin() )
  {
    // lfs format
    this->format();
    // mount again if still failed, give up
    if ( !Adafruit_LittleFS::begin() ) return false;
  }

  return true;
}
