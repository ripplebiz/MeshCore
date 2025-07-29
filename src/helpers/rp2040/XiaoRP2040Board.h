#pragma once

#include <Arduino.h>
#include <MeshCore.h>

// LoRa radio module pins for the Xiao RP2040
// https://wiki.seeedstudio.com/XIAO-RP2040/

#define P_LORA_DIO_1      27 // D1
#define P_LORA_NSS        6  // D4
#define P_LORA_RESET      28 // D2
#define P_LORA_BUSY       29 // D3
#define P_LORA_TX_LED     17

#define SX126X_RXEN       7  // D5
#define SX126X_TXEN       -1

#define SX126X_DIO2_AS_RF_SWITCH true
#define SX126X_DIO3_TCXO_VOLTAGE 1.8

/*
 * This board has no built-in way to read battery voltage.
 * Nevertheless it's very easy to make it work, you only require two 1% resistors.
 * If your using the WIO SX1262 Addon for xaio, make sure you dont connect D0!
 *
 *    BAT+ -----+
 *              |
 *       VSYS --+ -/\/\/\/\- --+
 *                   200k      |
 *                             +-- D0
 *                             |
 *        GND --+ -/\/\/\/\- --+
 *              |    100k
 *    BAT- -----+
 */
#define PIN_VBAT_READ     26 // D0
#define BATTERY_SAMPLES   8
#define ADC_MULTIPLIER    (3.0f * 3.3f * 1000)

class XiaoRP2040Board : public mesh::MainBoard {
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

  const char *getManufacturerName() const override { return "Xiao RP2040"; }

  void reboot() override { rp2040.reboot(); }

  bool startOTAUpdate(const char *id, char reply[]) override;
};
