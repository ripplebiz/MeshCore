#pragma once

#include "BaseSerialInterface.h"
#include <Arduino.h>

class ArduinoSerialInterface : public BaseSerialInterface {
  bool _isEnabled;
  uint8_t _state;
  uint16_t _frame_len;
  uint16_t rx_len;
#ifdef LILYGO_T3S3
  HWCDC* _serial;
#elif defined(NRF52_PLATFORM)
  Adafruit_USBD_CDC* _serial;
#else
  HardwareSerial* _serial;
#endif
  uint8_t rx_buf[MAX_FRAME_SIZE];

public:
  ArduinoSerialInterface() { _isEnabled = false; _state = 0; }

#ifdef LILYGO_T3S3
  void begin(HWCDC& serial) { _serial = &serial; }
#elif defined(NRF52_PLATFORM)
  void begin(Adafruit_USBD_CDC& serial) {
    _serial = &serial;
  #ifdef RAK_4631
    pinMode(WB_IO2, OUTPUT);
  #endif
  }
#else
  void begin(HardwareSerial& serial) { _serial = &serial; }
#endif

  // BaseSerialInterface methods
  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }

  bool isConnected() const override;

  bool isWriteBusy() const override;
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;
};