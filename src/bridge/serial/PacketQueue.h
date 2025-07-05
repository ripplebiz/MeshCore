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

#include <Packet.h>
#include <string.h>

#if MESH_PACKET_LOGGING
#include <Arduino.h>
#endif

#ifndef MAX_QUEUE_SIZE
#define MAX_QUEUE_SIZE 8
#endif

namespace bridge {

class PacketQueue {
private:
  struct {
    size_t len;
    uint8_t bytes[MAX_TRANS_UNIT];
  } buffer[MAX_QUEUE_SIZE];

protected:
  uint16_t head = 0, tail = 0;

public:
  size_t available() const { return (tail - head + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE; }

  size_t enqueue(const uint8_t *bytes, const uint8_t len) {
    if (len == 0 || len > MAX_TRANS_UNIT) {
#if BRIDGE_DEBUG
      Serial.printf("BRIDGE: enqueue() failed len=%d\n", len);
#endif
      return 0;
    }

    if ((tail + 1) % MAX_QUEUE_SIZE == head) { // Buffer full
      head = (head + 1) % MAX_QUEUE_SIZE;      // Overwrite oldest packet
    }

    buffer[tail].len = len;
    memcpy(buffer[tail].bytes, bytes, len);

#if MESH_PACKET_LOGGING
    Serial.printf("BRIDGE: enqueue() len=%d payload[5]=", len);
    for (size_t i = 0; i < len && i < 5; ++i) {
      Serial.printf("0x%02x ", buffer[tail].bytes[i]);
    }
    Serial.printf("\n");
#endif

    tail = (tail + 1) % MAX_QUEUE_SIZE;
    return len;
  }

  size_t enqueue(const mesh::Packet *pkt) {
    uint8_t bytes[MAX_TRANS_UNIT];
    const size_t len = pkt->writeTo(bytes);
    return enqueue(bytes, len);
  }

  size_t dequeue(uint8_t *bytes) {
    if (head == tail) { // Buffer empty
      return 0;
    }

    const size_t len = buffer[head].len;
    memcpy(bytes, buffer[head].bytes, len);
    head = (head + 1) % MAX_QUEUE_SIZE;

#if MESH_PACKET_LOGGING
    Serial.printf("BRIDGE: dequeue() payload[5]=");
    for (size_t i = 0; i < len && i < 5; ++i) {
      Serial.printf("0x%02x ", bytes[i]);
    }
    Serial.printf("\n");
#endif

    return len;
  }
};

} // namespace bridge
