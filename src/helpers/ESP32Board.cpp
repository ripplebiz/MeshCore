#ifdef ESP_PLATFORM

#include "ESP32Board.h"

#if defined(ADMIN_PASSWORD) && !defined(DISABLE_WIFI_OTA)   // Repeater or Room Server only
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <SPIFFS.h>

bool ESP32Board::startOTAUpdate(const char* id, char reply[]) {
  WiFi.softAP("MeshCore-OTA", NULL);

  sprintf(reply, "Started: http://%s/update", WiFi.softAPIP().toString().c_str());
  MESH_DEBUG_PRINTLN("startOTAUpdate: %s", reply);

  static char id_buf[60];
  sprintf(id_buf, "%s (%s)", id, getManufacturerName());
  static char home_buf[90];
  sprintf(home_buf, "<H2>Hi! I am a MeshCore Repeater. ID: %s</H2>", id);

  AsyncWebServer* server = new AsyncWebServer(80);

  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", home_buf);
  });
  server->on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/packet_log", "text/plain");
  });

  AsyncElegantOTA.setID(id_buf);
  AsyncElegantOTA.begin(server);    // Start ElegantOTA
  server->begin();

  return true;
}

#else
bool ESP32Board::startOTAUpdate(const char* id, char reply[]) {
  return false; // not supported
}
#endif

uint8_t ESP32Board::scanI2CDevices(TwoWire *w)
{
    uint8_t err, addr;
    int nDevices = 0;
    uint32_t start = 0;

    MESH_DEBUG_PRINTLN("Scanning I2C for Devices");
    for (addr = 1; addr < 127; addr++) {
        start = millis();
        w->beginTransmission(addr); delay(2);
        err = w->endTransmission();
        if (err == 0) {
            nDevices++;
            switch (addr) {
            case 0x77:
            case 0x76:
                MESH_DEBUG_PRINTLN("\tFound BME280 Sensor");
                _deviceOnline |= BME280_ONLINE;
                break;
            case 0x34:
                MESH_DEBUG_PRINTLN("\tFound AXP192/AXP2101 PMU");
                _deviceOnline |= POWERMANAGE_ONLINE;
                break;
            case 0x3C:
                MESH_DEBUG_PRINTLN("\tFound SSD1306/SH1106 dispaly");
                _deviceOnline |= DISPLAY_ONLINE;
                break;
            case 0x51:
                MESH_DEBUG_PRINTLN("\tFound PCF8563 RTC");
                _deviceOnline |= PCF8563_ONLINE;
                break;
            case 0x1C:
                MESH_DEBUG_PRINTLN("\tFound QMC6310 MAG Sensor");
                _deviceOnline |= QMC6310_ONLINE;
                break;
            default:
                MESH_DEBUG_PRINTLN("\tI2C device found at address 0x%02x!", addr);
                break;
            }

        } else if (err == 4) {
            MESH_DEBUG_PRINTLN("Unknown error at address 0x%02x!", addr);
        }
    }
    if (nDevices == 0)
        MESH_DEBUG_PRINTLN("No I2C devices found\n");

    MESH_DEBUG_PRINTLN("Scan for devices is complete.");
    return _deviceOnline;
}

#endif
