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

#if ENV_INCLUDE_BME680
#ifndef TELEM_BME680_ADDRESS
#define TELEM_BME680_ADDRESS    0x76      // BME680 environmental sensor I2C address
#endif
#include <bsec2.h>
static Bsec2 BME680;
static float rawPressure = 0;
static float rawTemperature = 0;
static float compTemperature = 0;
static float rawHumidity = 0;
static float compHumidity = 0;
static float readIAQ = 0;
static float readStaticIAQ = 0;
static float readCO2 = 0;
#endif

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
#endif

#ifdef MESH_DEBUG
uint32_t deviceOnline = 0x00;
static void scanDevices(TwoWire *w)
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
            case 0x76:
                Serial.println("\tFound RAK1906 Environment Sensor");
                deviceOnline |= BME680_ONLINE;
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
  return radio.std_init(&SPI);
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
    gps_detected = true;
    return true;
  }
  else
  MESH_DEBUG_PRINTLN("GPS failed to init on this IO pin... try the next");
  //digitalWrite(ioPin,pinInitialState); //reset the IO pin to initial state
  return false;
}
#endif

#if ENV_INCLUDE_BME680
static void checkBMEStatus(Bsec2 bsec) {
  if (bsec.status < BSEC_OK)
  {
    MESH_DEBUG_PRINTLN("BSEC error code : %f", float(bsec.status));
  }
  else if (bsec.status > BSEC_OK)
  {
    MESH_DEBUG_PRINTLN("BSEC warning code : %f", float(bsec.status));
  }

  if (bsec.sensor.status < BME68X_OK)
  {
    MESH_DEBUG_PRINTLN("BME68X error code : %f", bsec.sensor.status);
  }
  else if (bsec.sensor.status > BME68X_OK)
  {
    MESH_DEBUG_PRINTLN("BME68X warning code : %f", bsec.sensor.status);
  }
}

static void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec) {
    if (!outputs.nOutputs) {
      MESH_DEBUG_PRINTLN("No new data to report out");
      return;
    }

    MESH_DEBUG_PRINTLN("BSEC outputs:\n\tTime stamp = %f", (int) (outputs.output[0].time_stamp / INT64_C(1000000)));
    for (uint8_t i = 0; i < outputs.nOutputs; i++) {
        const bsecData output  = outputs.output[i];
        switch (output.sensor_id)
        {
          case BSEC_OUTPUT_IAQ:
              readIAQ = output.signal;
              MESH_DEBUG_PRINTLN("\tIAQ = %f", output.signal);
              MESH_DEBUG_PRINTLN("\tIAQ accuracy = %f", output.accuracy);
              break;
          case BSEC_OUTPUT_RAW_TEMPERATURE:
              rawTemperature = output.signal;
              MESH_DEBUG_PRINTLN("\tTemperature = %f", output.signal);
              break;
          case BSEC_OUTPUT_RAW_PRESSURE:
              rawPressure = output.signal;
              MESH_DEBUG_PRINTLN("\tPressure = %f", output.signal);
              break;
          case BSEC_OUTPUT_RAW_HUMIDITY:
              rawHumidity = output.signal;
              MESH_DEBUG_PRINTLN("\tHumidity = %f", output.signal);
              break;
          case BSEC_OUTPUT_RAW_GAS:
              MESH_DEBUG_PRINTLN("\tGas resistance = %f", output.signal);
              break;
          case BSEC_OUTPUT_STABILIZATION_STATUS:
              MESH_DEBUG_PRINTLN("\tStabilization status = %f", output.signal);
              break;
          case BSEC_OUTPUT_RUN_IN_STATUS:
              MESH_DEBUG_PRINTLN("\tRun in status = %f", output.signal);
              break;
          case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
              compTemperature = output.signal;
              MESH_DEBUG_PRINTLN("\tCompensated temperature = %f", output.signal);
              break;
          case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
              compHumidity = output.signal;
              MESH_DEBUG_PRINTLN("\tCompensated humidity = %f", output.signal);
              break;
          case BSEC_OUTPUT_STATIC_IAQ:
              readStaticIAQ = output.signal;
              MESH_DEBUG_PRINTLN("\tStatic IAQ = %f", output.signal);
              break;
          case BSEC_OUTPUT_CO2_EQUIVALENT:
              readCO2 = output.signal;
              MESH_DEBUG_PRINTLN("\tCO2 Equivalent = %f", output.signal);
              break;
          case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
              MESH_DEBUG_PRINTLN("\tbVOC equivalent = %f", output.signal);
              break;
          case BSEC_OUTPUT_GAS_PERCENTAGE:
              MESH_DEBUG_PRINTLN("\tGas percentage = %f", output.signal);
              break;
          case BSEC_OUTPUT_COMPENSATED_GAS:
              MESH_DEBUG_PRINTLN("\tCompensated gas = %f", output.signal);
              break;
          default:
              break;
        }
    }
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
      gps_detected = false;
      return false;
    }

    #ifndef FORCE_GPS_ALIVE
    //Now that GPS is found and set up, set to sleep for initial state
    stop_gps();
    #endif
  #endif

  #if ENV_INCLUDE_BME680

    bsecSensor sensorList[5] = {
      BSEC_OUTPUT_IAQ,
    //  BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_PRESSURE,
    //  BSEC_OUTPUT_RAW_HUMIDITY,
    //  BSEC_OUTPUT_RAW_GAS,
    //  BSEC_OUTPUT_STABILIZATION_STATUS,
    //  BSEC_OUTPUT_RUN_IN_STATUS,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
      BSEC_OUTPUT_STATIC_IAQ,
    //  BSEC_OUTPUT_CO2_EQUIVALENT,
    //  BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    //  BSEC_OUTPUT_GAS_PERCENTAGE,
    //  BSEC_OUTPUT_COMPENSATED_GAS
    };

    if(!BME680.begin(TELEM_BME680_ADDRESS, Wire)){
      checkBMEStatus(BME680);
      bme680_present = false;
      bme680_active = false;
      return false;
    }
    
    MESH_DEBUG_PRINTLN("Found BME680 at address: %02X", TELEM_BME680_ADDRESS);
    bme680_present = true;
    bme680_active = true;

    if (SAMPLING_RATE == BSEC_SAMPLE_RATE_ULP)
	  {
		  BME680.setTemperatureOffset(BSEC_SAMPLE_RATE_ULP);
	  }
	  else if (SAMPLING_RATE == BSEC_SAMPLE_RATE_LP)
	  {
		  BME680.setTemperatureOffset(TEMP_OFFSET_LP);
	  }

    if (!BME680.updateSubscription(sensorList, ARRAY_LEN(sensorList), SAMPLING_RATE))
    {
      checkBMEStatus(BME680);
    }

    BME680.attachCallback(newDataCallback);

  #endif
}

bool RAK4631SensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  #ifdef ENV_INCLUDE_GPS
  if (requester_permissions & TELEM_PERM_LOCATION && gps_active) {   // does requester have permission?
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, node_altitude);
  }
  #endif

  if (requester_permissions & TELEM_PERM_ENVIRONMENT) {

    #if ENV_INCLUDE_BME680
    if (bme680_active) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, compTemperature);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, compHumidity);
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, rawPressure);
      telemetry.addTemperature(TELEM_CHANNEL_SELF+1, readIAQ);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF+1, readStaticIAQ);
    }
    #endif
  }
  return true;
}

void RAK4631SensorManager::loop() {
  static long next_update = 0;
  
  #ifdef ENV_INCLUDE_GPS
  _nmea->loop();
  #endif

  if (millis() > next_update) {
    
    #ifdef ENV_INCLUDE_GPS
    if(gps_active){
      node_lat = (double)ublox_GNSS.getLatitude()/10000000.;
      node_lon = (double)ublox_GNSS.getLongitude()/10000000.;
      node_altitude = (double)ublox_GNSS.getAltitude()/1000.;
      MESH_DEBUG_PRINT("lat %f lon %f alt %f\r\n", node_lat, node_lon, node_altitude);
    }
    #endif

    #ifdef ENV_INCLUDE_BME680
    if(bme680_active){
      if (!BME680.run()){
        checkBMEStatus(BME680);
      }
    }
    #endif
    next_update = millis() + 1000;
  }

}

int RAK4631SensorManager::getNumSettings() const { 
  #if ENV_INCLUDE_GPS
    return gps_detected ? 1 : 0;  // only show GPS setting if GPS is detected
  #else
    return 0;
  #endif
}

const char* RAK4631SensorManager::getSettingName(int i) const {
  #if ENV_INCLUDE_GPS
    return (gps_detected && i == 0) ? "gps" : NULL;
  #else  
    return NULL;
  #endif
}

const char* RAK4631SensorManager::getSettingValue(int i) const {
  #if ENV_INCLUDE_GPS
  if (gps_detected && i == 0) {
    return gps_active ? "1" : "0";
  }
  #endif
  return NULL;
}

bool RAK4631SensorManager::setSettingValue(const char* name, const char* value) {
  #if ENV_INCLUDE_GPS
  if (gps_detected && strcmp(name, "gps") == 0) {
    if (strcmp(value, "0") == 0) {
      stop_gps();
    } else {
      start_gps();
    }
    return true;
  }
  #endif
  return false;  // not supported
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
