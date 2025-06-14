#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/RefCountedDigitalPin.h>

// LoRa radio module pins for Heltec V3
// Also for Heltec Wireless Tracker
#define  P_LORA_DIO_1   14
#define  P_LORA_NSS      8
#define  P_LORA_RESET   RADIOLIB_NC
#define  P_LORA_BUSY    13
#define  P_LORA_SCLK     9
#define  P_LORA_MISO    11
#define  P_LORA_MOSI    10

// built-ins    
#define  PIN_VBAT_READ    1
#ifndef PIN_ADC_CTRL              // set in platformio.ini for Heltec Wireless Tracker (2)
  #define  PIN_ADC_CTRL    37
#endif
#define  PIN_ADC_CTRL_ACTIVE    LOW
#define  PIN_ADC_CTRL_INACTIVE  HIGH
//#define  PIN_LED_BUILTIN 35

class NativeBoard : public mesh::MainBoard {
private:
  bool adc_active_state;

public:
  RefCountedDigitalPin periph_power;

  NativeBoard() : periph_power(PIN_VEXT_EN) { }

  void begin() {
    NativeBoard::begin();

    // Auto-detect correct ADC_CTRL pin polarity (different for boards >3.2)
    pinMode(PIN_ADC_CTRL, INPUT);
    adc_active_state = !digitalRead(PIN_ADC_CTRL);
    
    pinMode(PIN_ADC_CTRL, OUTPUT);
    digitalWrite(PIN_ADC_CTRL, !adc_active_state); // Initially inactive

    periph_power.begin();
  }

  void enterDeepSleep(uint32_t secs, int pin_wake_btn = -1) {
    // Not implemented for native Linux
  }

  void powerOff() override {
    // Not implemented for native Linux
  }

  uint16_t getBattMilliVolts() override {
    // Simulate battery voltage for native Linux
    return 3700; // Return 3.7V as a default value
  }

  const char* getManufacturerName() const override {
    return "Native";
  }

  void reboot() override {
    // Not implemented for native Linux
  }

  uint8_t getStartupReason() const override {
    // Not implemented for native Linux
    return 0;
  }
};
