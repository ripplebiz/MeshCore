#pragma once
#include <cstdint> // For uint8_t, uint32_t

#define TELEM_MODE_DENY            0
#define TELEM_MODE_ALLOW_FLAGS     1     // use contact.flags
#define TELEM_MODE_ALLOW_ALL       2

#define ADVERT_LOC_NONE       0
#define ADVERT_LOC_SHARE      1

#define WIFI_STATUS_DISABLED     0
#define WIFI_STATUS_ENABLED      1
#define WIFI_STATUS_CONNECTING   2
#define WIFI_STATUS_CONNECTED    3
#define WIFI_STATUS_FAILED       4

struct NodePrefs {  // persisted to file
  float airtime_factor;
  char node_name[32];
  float freq;
  uint8_t sf;
  uint8_t cr;
  uint8_t reserved1;
  uint8_t manual_add_contacts;
  float bw;
  uint8_t tx_power_dbm;
  uint8_t telemetry_mode_base;
  uint8_t telemetry_mode_loc;
  uint8_t telemetry_mode_env;
  float rx_delay_base;
  uint32_t ble_pin;
  uint8_t  advert_loc_policy;
  
  // WiFi configuration
  char wifi_ssid[64];          // WiFi network name
  char wifi_password[64];      // WiFi password
  uint16_t wifi_tcp_port;      // TCP port for WiFi interface (default: 5000)
  uint8_t wifi_enabled;        // WiFi enable/disable flag
  uint8_t wifi_auto_connect;   // Auto-connect on startup
  uint8_t reserved_wifi[2];    // Reserved for future WiFi settings
};