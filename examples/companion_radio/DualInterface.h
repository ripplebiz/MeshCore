#pragma once

#include <helpers/BaseSerialInterface.h>

#ifdef ESP32
#include <helpers/esp32/SerialWifiInterface.h>
#include <helpers/esp32/SerialBLEInterface.h>

class DualInterface : public BaseSerialInterface {
public:
  DualInterface(const char* ble_name, uint32_t ble_pin, uint16_t wifi_port = 5000);
  
  void begin();
  void begin(const char* ble_name, uint32_t ble_pin, uint16_t wifi_port);
  void begin(uint16_t wifi_port);
  
  bool isEnabled() const override;
  void enable() override;
  void disable() override;
  
  bool isConnected() const override;
  bool isWriteBusy() const override;
  
  size_t checkRecvFrame(uint8_t* buffer) override;
  size_t writeFrame(const uint8_t* buffer, size_t len) override;
  
  // WiFi management
  void enableWiFi(const char* ssid, const char* password);
  void disableWiFi();
  bool isWiFiEnabled() const { return _wifi_enabled; }
  bool isWiFiConnected() const;
  
  // BLE management
  void enableBLE(const char* name, uint32_t pin);
  void disableBLE();
  bool isBLEEnabled() const { return _ble_enabled; }
  bool isBLEConnected() const;
  
  // Client management 
  void disconnectWiFiClients();
  void disconnectBLEClients();

private:
  SerialWifiInterface _wifi_interface;
  SerialBLEInterface _ble_interface;
  
  bool _wifi_enabled;
  bool _ble_enabled;
  bool _wifi_initialized;
  bool _ble_initialized;
  
  // Connection state tracking 
  bool _wifi_was_connected;
  bool _ble_was_connected;
  
  char _ble_name[48];
  uint32_t _ble_pin;
  uint16_t _wifi_port;
  uint8_t _check_interface_index;
};

#endif 