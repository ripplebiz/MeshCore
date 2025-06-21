#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>
#include <helpers/sensors/MicroNMEALocationProvider.h>

NanoG2Ultra board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);

WRAPPER_CLASS radio_driver(radio, board);

VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1);
NanoG2UltraSensorManager sensors = NanoG2UltraSensorManager(nmea);

#ifdef DISPLAY_CLASS
DISPLAY_CLASS display;
#endif

bool radio_init()
{
  rtc_clock.begin(Wire);
  SPI.begin();
  return radio.std_init(&SPI);
}

uint32_t radio_get_rng_seed()
{
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr)
{
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(uint8_t dbm)
{
  radio.setOutputPower(dbm);
}

void NanoG2UltraSensorManager::start_gps()
{
  if (!gps_active)
  {
    MESH_DEBUG_PRINTLN("starting GPS");
    digitalWrite(PIN_GPS_STANDBY, HIGH);
    gps_active = true;
  }
}

void NanoG2UltraSensorManager::stop_gps()
{
  if (gps_active)
  {
    MESH_DEBUG_PRINTLN("stopping GPS");
    digitalWrite(PIN_GPS_STANDBY, LOW);
    gps_active = false;
  }
}

bool NanoG2UltraSensorManager::begin()
{
  Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX); // be sure to tx into rx and rx into tx
  Serial1.begin(115200);

  pinMode(PIN_GPS_STANDBY, OUTPUT);
  digitalWrite(PIN_GPS_STANDBY, HIGH); // Wake GPS from standby
  delay(500);

  // We'll consider GPS detected if we see any data on Serial1
  if (Serial1.available() > 0)
  {
    MESH_DEBUG_PRINTLN("GPS detected");
  }
  else
  {
    MESH_DEBUG_PRINTLN("No GPS detected");
  }
  digitalWrite(GPS_EN, LOW); // Put GPS back into standby mode
  return true;
}

bool NanoG2UltraSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP &telemetry)
{
  if (requester_permissions & TELEM_PERM_LOCATION)
  { // does requester have permission?
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, node_altitude);
  }
  return true;
}

void NanoG2UltraSensorManager::loop()
{
  static long next_gps_update = 0;
  _location->loop();
  if (millis() > next_gps_update && gps_active) // don't bother if gps position is not enabled
  {
    if (_location->isValid())
    {
      node_lat = ((double)_location->getLatitude()) / 1000000.;
      node_lon = ((double)_location->getLongitude()) / 1000000.;
      node_altitude = ((double)_location->getAltitude()) / 1000.0;
      MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
    }
    next_gps_update = millis() + (1000 * 60); // after initial update, only check every minute TODO: should be configurable
  }
}

int NanoG2UltraSensorManager::getNumSettings() const { return 1; } // just one supported: "gps" (power switch)

const char *NanoG2UltraSensorManager::getSettingName(int i) const
{
  return i == 0 ? "gps" : NULL;
}

const char *NanoG2UltraSensorManager::getSettingValue(int i) const
{
  if (i == 0)
  {
    return gps_active ? "1" : "0";
  }
  return NULL;
}

bool NanoG2UltraSensorManager::setSettingValue(const char *name, const char *value)
{
  if (strcmp(name, "gps") == 0)
  {
    if (strcmp(value, "0") == 0)
    {
      stop_gps();
    }
    else
    {
      start_gps();
    }
    return true;
  }
  return false; // not supported
}

mesh::LocalIdentity radio_new_identity()
{
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng); // create new random identity
}
