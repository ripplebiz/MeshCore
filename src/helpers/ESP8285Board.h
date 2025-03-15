#pragma once

#include <MeshCore.h>
#include <Arduino.h>

#if defined(ESP8266)

#include <ESP8266WiFi.h>  // For ESP.getResetReason() and ESP.restart()
#include <sys/time.h>

#ifndef BD_STARTUP_UNKNOWN
  // For ESP8266/ESP8285, if no proper startup reason is available, fall back to a known value.
  #define BD_STARTUP_UNKNOWN BD_STARTUP_NORMAL
#endif


class ESP8285Board : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin() {
    // Determine startup reason using ESP8266 API
    String reason = ESP.getResetReason();
    if (reason == "Power on") {
      startup_reason = BD_STARTUP_NORMAL;
    } else {
      // Add more mappings as needed for your enum
      startup_reason = BD_STARTUP_UNKNOWN;  // Assuming this exists in MainBoard
    }

  #ifdef PIN_VBAT_READ
    // battery read support
    pinMode(PIN_VBAT_READ, INPUT);
    adcAttachPin(PIN_VBAT_READ);
  #endif

  #ifdef PIN_LED_BUILTIN
    pinMode(PIN_LED_BUILTIN, OUTPUT);
    digitalWrite(PIN_LED_BUILTIN, LOW);
  #endif

  #if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
    Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL);
  #else
    Wire.begin();
  #endif
  }

  uint8_t getStartupReason() const override { return startup_reason; }

#if defined(PIN_LED_BUILTIN)
  void onBeforeTransmit() override {
    digitalWrite(PIN_LED_BUILTIN, HIGH);   // turn TX LED on
  }
  void onAfterTransmit() override {
    digitalWrite(PIN_LED_BUILTIN, LOW);   // turn TX LED off
  }
#endif

  uint16_t getBattMilliVolts() override {
    return 0;  // not supported
  }

  const char* getManufacturerName() const override {
    return "Generic ESP8285 ELRS Receiver";
  }

  void reboot() override {
    ESP.restart();  // ESP8266 reboot function
  }
};

class ESP8285RTCClock : public mesh::RTCClock {
public:
  ESP8285RTCClock() { }
  void begin() {
    String reason = ESP.getResetReason();
    if (reason == "Power on") {
      // Initialize with a default time on power-on
      struct timeval tv;
      tv.tv_sec = 1715770351;  // 15 May 2024, 8:50pm
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
    }
    // On other resets, assume time persists unless synced externally
  }

  uint32_t getCurrentTime() override {
    time_t now;
    time(&now);
    return now;
  }

  void setCurrentTime(uint32_t time) override {
    struct timeval tv;
    tv.tv_sec = time;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
  }
};

#endif
