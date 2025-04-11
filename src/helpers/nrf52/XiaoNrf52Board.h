#pragma once

#include <MeshCore.h>
#include <Arduino.h>

#ifdef XIAO_NRF52

// LoRa radio module pins for Seeed Xiao-nrf52
#ifdef SX1262_XIAO_S3_VARIANT
  #define  P_LORA_DIO_1       D0
  #define  P_LORA_BUSY        D1
  #define  P_LORA_RESET       D2
  #define  P_LORA_NSS         D3
  #define  LORA_TX_BOOST_PIN  D4
#else
  #define  P_LORA_DIO_1       D1
  #define  P_LORA_BUSY        D3
  #define  P_LORA_RESET       D2
  #define  P_LORA_NSS         D4
  #define  LORA_TX_BOOST_PIN  D5
#endif
#define  P_LORA_SCLK        PIN_SPI_SCK
#define  P_LORA_MISO        PIN_SPI_MISO
#define  P_LORA_MOSI        PIN_SPI_MOSI
//#define  SX126X_POWER_EN  37

#define SX126X_DIO2_AS_RF_SWITCH  true
#define SX126X_DIO3_TCXO_VOLTAGE   1.8


class XiaoNrf52Board : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin();
  uint8_t getStartupReason() const override { return startup_reason; }

#if defined(P_LORA_TX_LED)
  void onBeforeTransmit() override {
    digitalWrite(P_LORA_TX_LED, LOW);   // turn TX LED on
  }
  void onAfterTransmit() override {
    digitalWrite(P_LORA_TX_LED, HIGH);   // turn TX LED off
  }
#endif

  uint16_t getBattMilliVolts() override {
    // Please read befor going further ;)
    // https://wiki.seeedstudio.com/XIAO_BLE#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging

    pinMode(BAT_NOT_CHARGING, INPUT);
    if (digitalRead(BAT_NOT_CHARGING) == HIGH) {
      int adcvalue = 0;
      analogReadResolution(12);
      analogReference(AR_INTERNAL_3_0);  
      digitalWrite(VBAT_ENABLE, LOW);
      delay(10);
      adcvalue = analogRead(PIN_VBAT);
      digitalWrite(VBAT_ENABLE, HIGH);
      return (adcvalue * ADC_MULTIPLIER * AREF_VOLTAGE) / 4.096;
    } else {
      digitalWrite(VBAT_ENABLE, HIGH); // ensures high !
      return 4200; // charging value
    }
  }

  const char* getManufacturerName() const override {
    return "Seeed Xiao-nrf52";
  }

  void reboot() override {
    NVIC_SystemReset();
  }

  bool startOTAUpdate(const char* id, char reply[]) override;
};

#endif