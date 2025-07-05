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
#include "SerialBridge.h"

#include "MeshCore.h"
#include "Packet.h"

// Alternative <HardwareSerial.h> for ESP32
// Alternative <SerialUART.h> for RP2040
#include <Arduino.h>

namespace bridge {
#ifdef BRIDGE_OVER_SERIAL

#if !defined(BRIDGE_OVER_SERIAL_RX) || !defined(BRIDGE_OVER_SERIAL_TX)
#error You must define RX and TX pins
#endif

void SerialBridge::setup() {
#if defined(ESP32)
  BRIDGE_OVER_SERIAL.setPins(BRIDGE_OVER_SERIAL_RX, BRIDGE_OVER_SERIAL_TX);
#elif defined(RP2040_PLATFORM)
  BRIDGE_OVER_SERIAL.setPinout(BRIDGE_OVER_SERIAL_TX, BRIDGE_OVER_SERIAL_RX);
#else
#error SerialBridge was not tested on the current platform
#endif
  BRIDGE_OVER_SERIAL.begin(115200);
  Serial.printf("Bridge over serial: enabled\n");
}

void SerialBridge::loop() {
  readFromSerial();
  writeToSerial();
}

bool SerialBridge::shouldRetransmit(const mesh::Packet *pkt) {
  if (pkt->isMarkedDoNotRetransmit()) {
    return false;
  }
}

size_t SerialBridge::getPacket(uint8_t *bytes) {
  return rx_queue.dequeue(bytes);
}

size_t SerialBridge::sendPacket(const mesh::Packet *pkt) {
  if (shouldRetransmit(pkt)) return 0;
  const size_t len = tx_queue.enqueue(pkt);
  return len;
}

void SerialBridge::readFromSerial() {
  static constexpr uint16_t size = sizeof(bridge::Packet) + 1;
  static uint8_t buffer[size];
  static uint16_t tail = 0;

  while (BRIDGE_OVER_SERIAL.available()) {
    buffer[tail] = (uint8_t)BRIDGE_OVER_SERIAL.read();
    tail = (tail + 1) % size;

#if BRIDGE_OVER_SERIAL_DEBUG
    Serial.printf("%02x ", buffer[(tail - 1 + size) % size]);
#endif

    // Check for complete packet by looking back to where the magic number should be
    uint16_t head = (tail - sizeof(bridge::Packet) + size) % size;
    const uint16_t magic = buffer[head] | (buffer[(head + 1) % size] << 8);

    if (magic == SERIAL_PKT_MAGIC) {
      const uint16_t len = buffer[(head + 2) % size] | (buffer[(head + 3) % size] << 8);
      const uint16_t crc = buffer[(head + 4) % size] | (buffer[(head + 5) % size] << 8);

      uint8_t payload[MAX_TRANS_UNIT];
      for (size_t i = 0; i < len; i++) {
        payload[i] = buffer[(head + 6 + i) % size];
      }

      const bool valid = verifyCRC(payload, len, crc);

#if MESH_PACKET_LOGGING
      Serial.printf("BRIDGE: Read from serial len=%d crc=0x%04x\n", len, crc);
#endif

      if (verifyCRC(payload, len, crc)) {
#if MESH_PACKET_LOGGING
        Serial.printf("BRIDGE: Received packet was validated\n");
#endif
        rx_queue.enqueue(payload, len);
      }
    }
  }
}

void SerialBridge::writeToSerial() {
  bridge::Packet pkt;
  if (!tx_queue.available()) return;
  pkt.len = tx_queue.dequeue(pkt.payload);

  if (pkt.len > 0) {
    pkt.crc = SerialBridge::calculateCRC(pkt.payload, pkt.len);
    BRIDGE_OVER_SERIAL.write((uint8_t *)&pkt, sizeof(bridge::Packet));
#if MESH_PACKET_LOGGING
    Serial.printf("BRIDGE: Write to serial len=%d crc=0x%04x\n", pkt.len, pkt.crc);
#endif
  }
}

#endif
} // namespace bridge
