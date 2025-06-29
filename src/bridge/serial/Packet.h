/**
 * MeshCore - A new lightweight, hybrid routing mesh protocol for packet radios
 * Copyright (c) 2025 Scott Powell / rippleradios.com
 *
 * This project is maintained by the contributors listed in
 * https://github.com/ripplebiz/MeshCore/graphs/contributors
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#pragma once

#include "MeshCore.h"

#include <string.h>

namespace bridge {

/*
 * +----------------------------------------------------+
 * | SERIAL PACKET SPEC                                 |
 * +-----------+---------+------------------------------+
 * |   TYPE    |  NAME   | DESCRIPTION                  |
 * +-----------+---------+------------------------------+
 * | uint16_t  | MAGIC   | The packet start marker      |
 * | uint16_t  | LEN     | Length of the payload        |
 * | uint16_t  | CRC     | Checksum for error checking  |
 * | uint8_t[] | PAYLOAD | Actual rf packet data        |
 * +-----------+---------+------------------------------+
 */
#define SERIAL_PKT_MAGIC 0xdead

struct Packet {
  uint16_t magic, len, crc;
  uint8_t payload[MAX_TRANS_UNIT];

  Packet() : magic(SERIAL_PKT_MAGIC), len(0), crc(0) {}
};

} // namespace bridge