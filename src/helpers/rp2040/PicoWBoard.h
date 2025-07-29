#pragma once

#include <MeshCore.h>
#include <Arduino.h>

// LoRa radio module pins for PicoW
#define  P_LORA_DIO_1   20
#define  P_LORA_NSS     3
#define  P_LORA_RESET   15
#define  P_LORA_BUSY    2
#define  P_LORA_SCLK    10
#define  P_LORA_MISO    12
#define  P_LORA_MOSI    11
//#define  SX126X_POWER_EN  ??? // Not Sure

#define SX126X_DIO2_AS_RF_SWITCH  true
#define SX126X_DIO3_TCXO_VOLTAGE   1.8

// built-ins
#define  PIN_VBAT_READ    26
#define  ADC_MULTIPLIER   (3.1 * 3.3 * 1000) // MT Uses 3.1
#define  PIN_LED_BUILTIN  LED_BUILTIN

class PicoWBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin();
  uint8_t getStartupReason() const override { return startup_reason; }

  void onBeforeTransmit() override {
    digitalWrite(LED_BUILTIN, HIGH);   // turn TX LED on
  }

  void onAfterTransmit() override {
    digitalWrite(LED_BUILTIN, LOW);   // turn TX LED off
  }

  #define BATTERY_SAMPLES 8

  uint16_t getBattMilliVolts() override {
    analogReadResolution(12);

    uint32_t raw = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / BATTERY_SAMPLES;

    return (ADC_MULTIPLIER * raw) / 4096;
  }

  const char* getManufacturerName() const override {
    return "Pico W";
  }

  void reboot() override {
    //NVIC_SystemReset();
    rp2040.reboot();
  }

  bool startOTAUpdate(const char* id, char reply[]) override;
};
