#pragma once

#include <Arduino.h>
#include <MeshCore.h>

// LoRa radio module pins for Waveshare RP2040-LoRa-HF/LF
// https://files.waveshare.com/wiki/RP2040-LoRa/Rp2040-lora-sch.pdf

#define P_LORA_DIO_1             16
#define P_LORA_NSS               13 // CS
#define P_LORA_RESET             23
#define P_LORA_BUSY              18
#define P_LORA_SCLK              14
#define P_LORA_MISO              24
#define P_LORA_MOSI              15
#define P_LORA_TX_LED            25

#define SX126X_DIO2_AS_RF_SWITCH true
#define SX126X_DIO3_TCXO_VOLTAGE 0

// This board has no built-in way to read battery voltage
// #define PIN_VBAT_READ    26
// #define BATTERY_SAMPLES  8
// #define ADC_MULTIPLIER   (3.1 * 3.3 * 1000) // MT Uses 3.1

class WaveshareBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin();
  uint8_t getStartupReason() const override { return startup_reason; }

#ifdef P_LORA_TX_LED
  void onBeforeTransmit() override { digitalWrite(P_LORA_TX_LED, HIGH); }
  void onAfterTransmit() override { digitalWrite(P_LORA_TX_LED, LOW); }
#endif

  uint16_t getBattMilliVolts() override {
#if defined(PIN_VBAT_READ) && defined(ADC_MULTIPLIER)
    analogReadResolution(12);

    uint32_t raw = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / BATTERY_SAMPLES;

    return (ADC_MULTIPLIER * raw) / 4096;
#else
    return 0;
#endif
  }

  const char *getManufacturerName() const override { return "Waveshare RP2040-LoRa"; }

  void reboot() override { rp2040.reboot(); }

  bool startOTAUpdate(const char *id, char reply[]) override;
};
