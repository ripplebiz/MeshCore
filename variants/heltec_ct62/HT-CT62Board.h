#pragma once

#include <MeshCore.h>
#include <Arduino.h>

#if defined(ESP_PLATFORM)

#include <helpers/ESP32Board.h>

class Heltec_CT62_Board : public ESP32Board {
public:

uint16_t getBattMilliVolts() override {
  #ifdef PIN_VBAT_READ
    analogReadResolution(12);         // ESP32-C3 ADC is 12-bit - 3.3/4096 (ref voltage/max counts)
    uint32_t raw = 0;
    for (int i = 0; i < 8; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / 8;

    return ((6.52 * raw) / 1024.0) * 1000;
  #else
    return 0;  // not supported
  #endif
  }

  const char* getManufacturerName() const override {
    return "Heltec CT62";
  }
};

#endif
