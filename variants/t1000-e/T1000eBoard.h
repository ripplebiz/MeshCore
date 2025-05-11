#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include "variant.h"

class T1000eBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;
  uint8_t btn_prev_state;

public:
  void begin();

  uint16_t getBattMilliVolts() override {
  #ifdef BATTERY_PIN
   #ifdef PIN_3V3_EN
    digitalWrite(PIN_3V3_EN, HIGH);
   #endif
    analogReference(AR_INTERNAL_3_0);
    analogReadResolution(12);
    delay(10);
    float volts = (analogRead(BATTERY_PIN) * ADC_MULTIPLIER * AREF_VOLTAGE) / 4096;
   #ifdef PIN_3V3_EN
    digitalWrite(PIN_3V3_EN, LOW);
   #endif

    analogReference(AR_DEFAULT);  // put back to default
    analogReadResolution(10);

    return volts * 1000;
  #else
    return 0;
  #endif
  }

  uint8_t getStartupReason() const override { return startup_reason; }

  const char* getManufacturerName() const override {
    return "Seeed Tracker T1000-e";
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

  void powerOff() override {
    #ifdef HAS_GPS
        digitalWrite(GPS_VRTC_EN, LOW);
        digitalWrite(GPS_RESET, LOW);
        digitalWrite(GPS_SLEEP_INT, LOW);
        digitalWrite(GPS_RTC_INT, LOW);
        pinMode(GPS_RESETB, OUTPUT);
        digitalWrite(GPS_RESETB, LOW);
    #endif

    #ifdef BUZZER_EN
        digitalWrite(BUZZER_EN, LOW);
    #endif

    #ifdef PIN_3V3_EN
        digitalWrite(PIN_3V3_EN, LOW);
    #endif

    #ifdef LED_PIN
    digitalWrite(LED_PIN, LOW);
    #endif
    #ifdef BUTTON_PIN
    nrf_gpio_cfg_sense_input(digitalPinToInterrupt(BUTTON_PIN), NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
    #endif
    sd_power_system_off();
  }

  void reboot() override {
    NVIC_SystemReset();
  }

//  bool startOTAUpdate(const char* id, char reply[]) override;
};