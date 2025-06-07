#pragma once

#include "../BaseSerialInterface.h"
#include <bluefruit.h>

class SerialBLEInterface : public BaseSerialInterface {
  BLEUart bleuart;
  bool deviceConnected;
  bool oldDeviceConnected;
  bool checkAdvRestart;
  bool _isEnabled;
  unsigned long _last_write;

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define FRAME_QUEUE_SIZE  4
  int send_queue_len;
  Frame send_queue[FRAME_QUEUE_SIZE];

  void clearBuffers() { send_queue_len = 0; }
  void startAdv();

  static SerialBLEInterface* _instance;

public:
  SerialBLEInterface() {
    deviceConnected = false;
    oldDeviceConnected = false;
    checkAdvRestart = false;
    _isEnabled = false;
    _last_write = 0;
    send_queue_len = 0;
    _instance = this;
  }

  static SerialBLEInterface* getInstance() {
    return _instance;
  }

  void begin(const char* device_name, uint32_t pin_code);

  // BaseSerialInterface methods
  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }

  bool isConnected() const override;

  bool isWriteBusy() const override;
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;

  friend void connect_callback(uint16_t conn_handle);
  friend void disconnect_callback(uint16_t conn_handle, uint8_t reason);
};

#if BLE_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define BLE_DEBUG_PRINT(F, ...) Serial.printf("BLE: " F, ##__VA_ARGS__)
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINT(...) {}
  #define BLE_DEBUG_PRINTLN(...) {}
#endif
