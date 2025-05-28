#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>
#include <helpers/sensors/MicroNMEALocationProvider.h>

RAK4631Board board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);

WRAPPER_CLASS radio_driver(radio, board);

VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

#if ENV_INCLUDE_GPS
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Wire);
RAK4631SensorManager sensors = RAK4631SensorManager(nmea);
#else
RAK4631SensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
#endif

#ifndef LORA_CR
  #define LORA_CR      5
#endif

#ifdef MESH_DEBUG
uint32_t deviceOnline = 0x00;
void scanDevices(TwoWire *w)
{
    uint8_t err, addr;
    int nDevices = 0;
    uint32_t start = 0;

    Serial.println("Scanning I2C for Devices");
    for (addr = 1; addr < 127; addr++) {
        start = millis();
        w->beginTransmission(addr); delay(2);
        err = w->endTransmission();
        if (err == 0) {
            nDevices++;
            switch (addr) {
            case 0x42:
                Serial.println("\tFound RAK12500 GPS Sensor");
                deviceOnline |= RAK12500_ONLINE;
                break;
            default:
                Serial.print("\tI2C device found at address 0x");
                if (addr < 16) {
                    Serial.print("0");
                }
                Serial.print(addr, HEX);
                Serial.println(" !");
                break;
            }

        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");

    Serial.println("Scan for devices is complete.");
    Serial.println("\n");
}
#endif

bool radio_init() {
  rtc_clock.begin(Wire);
  
#ifdef SX126X_DIO3_TCXO_VOLTAGE
  float tcxo = SX126X_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif

  SPI.setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI);
  SPI.begin();
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;  // fail
  }
  
  radio.setCRC(1);
  
#ifdef SX126X_CURRENT_LIMIT
  radio.setCurrentLimit(SX126X_CURRENT_LIMIT);
#endif
#ifdef SX126X_DIO2_AS_RF_SWITCH
  radio.setDio2AsRfSwitch(SX126X_DIO2_AS_RF_SWITCH);
#endif
#ifdef SX126X_RX_BOOSTED_GAIN
  radio.setRxBoostedGainMode(SX126X_RX_BOOSTED_GAIN);
#endif

  return true;  // success
}

uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(uint8_t dbm) {
  radio.setOutputPower(dbm);
}

#if ENV_INCLUDE_GPS
void RAK4631SensorManager::start_gps()
{
  //function currently not used
  gps_active = true;
  pinMode(disStandbyPin, OUTPUT);
  digitalWrite(disStandbyPin, 1);
  MESH_DEBUG_PRINTLN("GPS should be on now");
}

void RAK4631SensorManager::stop_gps()
{
  //function currently not used
  gps_active = false;
  pinMode(disStandbyPin, OUTPUT);
  digitalWrite(disStandbyPin, 0);
  MESH_DEBUG_PRINTLN("GPS should be off now");
}

void RAK4631SensorManager::sleep_gps() {
  gps_active = false;
  ublox_GNSS.powerSaveMode();
  MESH_DEBUG_PRINTLN("GPS should be sleeping now");
}

void RAK4631SensorManager::wake_gps() {
  gps_active = true;
  ublox_GNSS.powerSaveMode(false);
  MESH_DEBUG_PRINTLN("GPS should be waking now");
}

bool RAK4631SensorManager::gpsIsAwake(uint32_t ioPin){
  
  int pinInitialState = 0;

  //set initial waking state
  pinMode(ioPin,OUTPUT);
  digitalWrite(ioPin,0);
  delay(1000);
  digitalWrite(ioPin,1);
  delay(1000);

  if (ublox_GNSS.begin(Wire) == true){
    MESH_DEBUG_PRINTLN("GPS init correctly and GPS is turned on");
    ublox_GNSS.setI2COutput(COM_TYPE_NMEA);
    ublox_GNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
    disStandbyPin = ioPin;
    gps_active = true;
    gps_present = true;
    return true;
  }
  else
  MESH_DEBUG_PRINTLN("GPS failed to init on this IO pin... try the next");
  //digitalWrite(ioPin,pinInitialState); //reset the IO pin to initial state
  return false;
}
#endif

bool RAK4631SensorManager::begin() {
  
  #ifdef MESH_DEBUG
  scanDevices(&Wire);
  #endif

#if ENV_INCLUDE_GPS
  //search for the correct IO standby pin depending on socket used
  if(gpsIsAwake(P_GPS_STANDBY_A)){
    MESH_DEBUG_PRINTLN("GPS is on socket A");
  }
  else if(gpsIsAwake(P_GPS_STANDBY_C)){
    MESH_DEBUG_PRINTLN("GPS is on socket C"); 
  }
  else if(gpsIsAwake(P_GPS_STANDBY_F)){
    MESH_DEBUG_PRINTLN("GPS is on socket F"); 
  }
  else{
    MESH_DEBUG_PRINTLN("Error: No GPS found on sockets A, C or F"); 
    gps_active = false;
    gps_present = false;
    return false;
  }

  //Now that GPS is found and set up, set to sleep for initial state
  stop_gps();
#endif
}

#if ENV_INCLUDE_GPS
bool RAK4631SensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  if (requester_permissions & TELEM_PERM_LOCATION && gps_active) {   // does requester have permission?
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, node_altitude);
  }
  return true;
}

void RAK4631SensorManager::loop() {
  static long next_update = 0;
  
  _nmea->loop();

  if (millis() > next_update && gps_active) {
    node_lat = (double)ublox_GNSS.getLatitude()/10000000.;
    node_lon = (double)ublox_GNSS.getLongitude()/10000000.;
    node_altitude = (double)ublox_GNSS.getAltitude()/1000.;
    MESH_DEBUG_PRINT("lat %f lon %f alt %f\r\n", node_lat, node_lon, node_altitude);
    
    next_update = millis() + 1000;
  }
}

int RAK4631SensorManager::getNumSettings() const { return 1; }  // just one supported: "gps" (power switch)

const char* RAK4631SensorManager::getSettingName(int i) const {
  return i == 0 ? "gps" : NULL;
}

const char* RAK4631SensorManager::getSettingValue(int i) const {
  if (i == 0) {
    return gps_active ? "1" : "0";
  }
  return NULL;
}

bool RAK4631SensorManager::setSettingValue(const char* name, const char* value) {
  if (strcmp(name, "gps") == 0) {
    if (strcmp(value, "0") == 0) {
      stop_gps();
    } else {
      start_gps();
    }
    return true;
  }
  return false;  // not supported
}
#endif

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
