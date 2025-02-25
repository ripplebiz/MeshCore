#pragma once

#include <MeshCore.h>
#include <Arduino.h>

// LoRa and SPI pins
#define  P_LORA_DIO_1   (32 + 1)  // P1.1
#define  P_LORA_NSS     (0 + 12)  // P0.12
#define  P_LORA_RESET   (32 + 10) // P1.10
#define  P_LORA_BUSY    (0 + 7)   // P0.7
#define  P_LORA_SCLK    (0 + 11)  // P0.11
#define  P_LORA_MISO    (32 + 8)  // P1.8
#define  P_LORA_MOSI    (32 + 9)  // P0.9
#define  LR1110_POWER_EN  37
 
#define LR11X0_DIO_AS_RF_SWITCH  true
#define LR11X0_DIO3_TCXO_VOLTAGE   1.6

// built-ins
//#define  PIN_VBAT_READ    5
//#define  ADC_MULTIPLIER   (3 * 1.73 * 1000)

class T1000eBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin();

  uint16_t getBattMilliVolts() override {
    return 0;
  }

  uint8_t getStartupReason() const override { return startup_reason; }

  const char* getManufacturerName() const override {
    return "Seeed Tracker T1000-e";
  }

  void reboot() override {
    NVIC_SystemReset();
  }

//  bool startOTAUpdate() override;
};
