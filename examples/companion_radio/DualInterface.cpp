#include "DualInterface.h"

#ifdef ESP32
#include <WiFi.h>

DualInterface::DualInterface(const char* ble_name, uint32_t ble_pin, uint16_t wifi_port) {
  strncpy(_ble_name, ble_name, sizeof(_ble_name) - 1);
  _ble_name[sizeof(_ble_name) - 1] = '\0';
  _ble_pin = ble_pin;
  _wifi_port = wifi_port;
  
  _wifi_enabled = false;
  _ble_enabled = false;
  _wifi_initialized = false;
  _ble_initialized = false;
  _check_interface_index = 0;
  
  // Initialize connection state tracking
  _wifi_was_connected = false;
  _ble_was_connected = false;
}

void DualInterface::begin() {
  // Initialize with BLE by default
  begin(_ble_name, _ble_pin, _wifi_port);
}

void DualInterface::begin(const char* ble_name, uint32_t ble_pin, uint16_t wifi_port) {
  strncpy(_ble_name, ble_name, sizeof(_ble_name) - 1);
  _ble_name[sizeof(_ble_name) - 1] = '\0';
  _ble_pin = ble_pin;
  _wifi_port = wifi_port;
  
  // Always initialize BLE
  enableBLE(_ble_name, _ble_pin);
}

void DualInterface::begin(uint16_t wifi_port) {
  _wifi_port = wifi_port;
  // WiFi will be enabled when WiFi credentials are available
}

bool DualInterface::isEnabled() const {
  return _ble_enabled || _wifi_enabled;
}

void DualInterface::enable() {
  if (_ble_enabled && !_ble_initialized) {
    _ble_interface.begin(_ble_name, _ble_pin);
    _ble_interface.enable();
    _ble_initialized = true;
  }
  
  if (_wifi_enabled && !_wifi_initialized && WiFi.status() == WL_CONNECTED) {
    _wifi_interface.begin(_wifi_port);
    _wifi_interface.enable();
    _wifi_initialized = true;
  }
}

void DualInterface::disable() {
  if (_ble_initialized) {
    _ble_interface.disable();
    _ble_initialized = false;
  }
  
  if (_wifi_initialized) {
    _wifi_interface.disable();
    _wifi_initialized = false;
  }
}

bool DualInterface::isConnected() const {
  bool ble_connected = _ble_initialized && _ble_interface.isConnected();
  bool wifi_connected = _wifi_initialized && _wifi_interface.isConnected();
  return ble_connected || wifi_connected;
}

bool DualInterface::isWriteBusy() const {
  bool ble_busy = _ble_initialized && _ble_interface.isWriteBusy();
  bool wifi_busy = _wifi_initialized && _wifi_interface.isWriteBusy();
  return ble_busy || wifi_busy;
}

size_t DualInterface::checkRecvFrame(uint8_t* buffer) {
  bool wifi_connected = _wifi_initialized && _wifi_interface.isConnected();
  bool ble_connected = _ble_initialized && _ble_interface.isConnected();
  
  if (wifi_connected && !_wifi_was_connected) {
    if (_ble_was_connected) {
      disconnectBLEClients();
    }
  }
  
  if (ble_connected && !_ble_was_connected) {
    if (_wifi_was_connected) {
      disconnectWiFiClients();
    }
  }
  
  _wifi_was_connected = wifi_connected;
  _ble_was_connected = ble_connected;
  
  size_t len = 0;
  
  if (_check_interface_index == 0) {
    if (_ble_initialized) {
      len = _ble_interface.checkRecvFrame(buffer);
    }
    if (len == 0 && _wifi_initialized) {
      len = _wifi_interface.checkRecvFrame(buffer);
    }
  } else {
    if (_wifi_initialized) {
      len = _wifi_interface.checkRecvFrame(buffer);
    }
    if (len == 0 && _ble_initialized) {
      len = _ble_interface.checkRecvFrame(buffer);
    }
  }
  
  _check_interface_index = (_check_interface_index + 1) % 2;
  return len;
}

void DualInterface::disconnectWiFiClients() {
  if (_wifi_initialized) {
    // Use public method instead of direct member access
    _wifi_interface.disable();
    _wifi_interface.enable();
  }
}

void DualInterface::disconnectBLEClients() {
  if (_ble_initialized) {
    // Use public method instead of direct member access
    _ble_interface.disable();
    _ble_interface.enable();
  }
}

size_t DualInterface::writeFrame(const uint8_t* buffer, size_t len) {
  // Send to all connected interfaces
  size_t bytes_written = 0;
  
  if (_ble_initialized && _ble_interface.isConnected()) {
    bytes_written += _ble_interface.writeFrame(buffer, len);
  }
  
  if (_wifi_initialized && _wifi_interface.isConnected()) {
    bytes_written += _wifi_interface.writeFrame(buffer, len);
  }
  
  return bytes_written;
}

void DualInterface::enableWiFi(const char* ssid, const char* password) {
  if (!_wifi_enabled) {
    _wifi_enabled = true;
    
    // Connect to WiFi if not already connected
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
    }
    
    // Enable WiFi interface if WiFi is connected
    if (WiFi.status() == WL_CONNECTED && !_wifi_initialized) {
      _wifi_interface.begin(_wifi_port);
      _wifi_interface.enable();
      _wifi_initialized = true;
    }
  }
}

void DualInterface::disableWiFi() {
  if (_wifi_enabled) {
    _wifi_enabled = false;
    
    if (_wifi_initialized) {
      _wifi_interface.disable();
      _wifi_initialized = false;
    }
  }
}

bool DualInterface::isWiFiConnected() const {
  return _wifi_initialized && _wifi_interface.isConnected();
}

void DualInterface::enableBLE(const char* name, uint32_t pin) {
  if (!_ble_enabled) {
    _ble_enabled = true;
    strncpy(_ble_name, name, sizeof(_ble_name) - 1);
    _ble_name[sizeof(_ble_name) - 1] = '\0';
    _ble_pin = pin;
    
    if (!_ble_initialized) {
      _ble_interface.begin(_ble_name, _ble_pin);
      _ble_interface.enable();
      _ble_initialized = true;
    }
  }
}

void DualInterface::disableBLE() {
  if (_ble_enabled) {
    _ble_enabled = false;
    
    if (_ble_initialized) {
      _ble_interface.disable();
      _ble_initialized = false;
    }
  }
}

bool DualInterface::isBLEConnected() const {
  return _ble_initialized && _ble_interface.isConnected();
}

#endif 