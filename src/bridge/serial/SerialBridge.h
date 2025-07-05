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

#include "Packet.h"
#include "PacketQueue.h"

#include <Packet.h>

namespace bridge {

class SerialBridge {
private:
  PacketQueue rx_queue, tx_queue;

protected:
  bool shouldRetransmit(const mesh::Packet *);

public:
  void loop();
  void setup();
  void readFromSerial();
  void writeToSerial();

  size_t getPacket(uint8_t *);
  size_t sendPacket(const mesh::Packet *);

  static uint16_t calculateCRC(const uint8_t *bytes, size_t len) {
    // Fletcher-16
    // https://en.wikipedia.org/wiki/Fletcher%27s_checksum
    uint8_t sum1 = 0, sum2 = 0;

    for (size_t i = 0; i < len; i++) {
      sum1 = (sum1 + bytes[i]) % 255;
      sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
  };

  static bool verifyCRC(const uint8_t *bytes, size_t len, uint16_t crc) {
    uint16_t computedChecksum = calculateCRC(bytes, len);
    return (computedChecksum == crc);
  }
};

} // namespace bridge
