#pragma once

#include <MeshCore.h>
#include <Arduino.h>

class STM32Board : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin() {
    startup_reason = BD_STARTUP_NORMAL;
  }

  uint8_t getStartupReason() const override { return startup_reason; }

  uint16_t getBattMilliVolts() override {
    return 0;  // not supported
  }

  const char* getManufacturerName() const override {
    return "Generic STM32";
  }

  void reboot() override {
  }

  bool startOTAUpdate(const char* id, char reply[]) override { return false; };
};