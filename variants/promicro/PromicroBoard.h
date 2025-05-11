#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include "variant.h"

#define  ADC_MULTIPLIER   (1.815f) // dependent on voltage divider resistors. TODO: more accurate battery tracking

class PromicroBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;
  uint8_t btn_prev_state;

public:
  void begin();

  uint8_t getStartupReason() const override { return startup_reason; }

  #define BATTERY_SAMPLES 8

  uint16_t getBattMilliVolts() override {
    analogReadResolution(12);

    uint32_t raw = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / BATTERY_SAMPLES;
    return (ADC_MULTIPLIER * raw);
  }

  const char* getManufacturerName() const override {
    return "ProMicro DIY";
  }

  int buttonStateChanged() {
    #ifdef BUTTON_PIN
      uint8_t v = digitalRead(BUTTON_PIN);
      if (v != btn_prev_state) {
        btn_prev_state = v;
        return (v == LOW) ? 1 : -1;
      }
    #endif
      return 0;
  }

  void reboot() override {
    NVIC_SystemReset();
  }

  bool startOTAUpdate(const char* id, char reply[]) override;
};
