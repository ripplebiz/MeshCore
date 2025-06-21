#include <Arduino.h>
#include "target.h"
#include <helpers/sensors/MicroNMEALocationProvider.h>

TBeamS3SupremeBoard board;

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
#endif

bool pmuIntFlag;

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1);
TbeamSupSensorManager sensors = TbeamSupSensorManager(nmea);

static void setPMUIntFlag(){
  pmuIntFlag = true;
}

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
            case 0x77:
            case 0x76:
                Serial.println("\tFound BME280 Sensor");
                deviceOnline |= BME280_ONLINE;
                break;
            case 0x34:
                Serial.println("\tFound AXP192/AXP2101 PMU");
                deviceOnline |= POWERMANAGE_ONLINE;
                break;
            case 0x3C:
                Serial.println("\tFound SSD1306/SH1106 dispaly");
                deviceOnline |= DISPLAY_ONLINE;
                break;
            case 0x51:
                Serial.println("\tFound PCF8563 RTC");
                deviceOnline |= PCF8563_ONLINE;
                break;
            case 0x1C:
                Serial.println("\tFound QMC6310 MAG Sensor");
                deviceOnline |= QMC6310_ONLINE;
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
void TBeamS3SupremeBoard::printPMU()
{
    Serial.print("isCharging:"); Serial.println(PMU.isCharging() ? "YES" : "NO");
    Serial.print("isDischarge:"); Serial.println(PMU.isDischarge() ? "YES" : "NO");
    Serial.print("isVbusIn:"); Serial.println(PMU.isVbusIn() ? "YES" : "NO");
    Serial.print("getBattVoltage:"); Serial.print(PMU.getBattVoltage()); Serial.println("mV");
    Serial.print("getVbusVoltage:"); Serial.print(PMU.getVbusVoltage()); Serial.println("mV");
    Serial.print("getSystemVoltage:"); Serial.print(PMU.getSystemVoltage()); Serial.println("mV");

    // The battery percentage may be inaccurate at first use, the PMU will automatically
    // learn the battery curve and will automatically calibrate the battery percentage
    // after a charge and discharge cycle
    if (PMU.isBatteryConnect()) {
        Serial.print("getBatteryPercent:"); Serial.print(PMU.getBatteryPercent()); Serial.println("%");
    }

    Serial.println();
}
void TbeamSupSensorManager::printBMEValues() {  
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");

  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}
#endif

bool TBeamS3SupremeBoard::power_init()
{
  bool result = PMU.begin(PMU_WIRE_PORT, I2C_PMU_ADD, PIN_BOARD_SDA1, PIN_BOARD_SCL1);
  if (result == false) {
    MESH_DEBUG_PRINTLN("power is not online..."); while (1)delay(50);
  }
  MESH_DEBUG_PRINTLN("Setting charge led");
  PMU.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

  // Set up PMU interrupts
  MESH_DEBUG_PRINTLN("Setting up PMU interrupts");
  pinMode(PIN_PMU_IRQ, INPUT_PULLUP);
  attachInterrupt(PIN_PMU_IRQ, setPMUIntFlag, FALLING);

  // GPS
  MESH_DEBUG_PRINTLN("Setting and enabling a-ldo4 for GPS");
  PMU.setALDO4Voltage(3300);
  PMU.enableALDO4(); // disable to save power

  // Lora
  MESH_DEBUG_PRINTLN("Setting and enabling a-ldo3 for LoRa");
  PMU.setALDO3Voltage(3300);
  PMU.enableALDO3();

  // To avoid SPI bus issues during power up, reset OLED, sensor, and SD card supplies
  // MESH_DEBUG_PRINTLN("Reset a-ldo1&2 and b-ldo1");
  // if (ESP_SLEEP_WAKEUP_UNDEFINED == esp_sleep_get_wakeup_cause())
  // {
  //   PMU.disableALDO1();
  //   PMU.disableALDO2();
  //   PMU.disableBLDO1();
  //   delay(250);
  // }

  // m.2 interface
  MESH_DEBUG_PRINTLN("Setting and enabling dcdc3 for m.2 interface");
  PMU.setDC3Voltage(3300); // doesn't go anywhere in the schematic??
  PMU.enableDC3();

  // QMC6310U
  MESH_DEBUG_PRINTLN("Setting and enabling a-ldo2 for QMC");
  PMU.setALDO2Voltage(3300);
  PMU.enableALDO2(); // disable to save power

  // BME280 and OLED
  MESH_DEBUG_PRINTLN("Setting and enabling a-ldo1 for oled");
  PMU.setALDO1Voltage(3300);
  PMU.enableALDO1();

  // SD card
  MESH_DEBUG_PRINTLN("Setting and enabling b-ldo2 for SD card");
  PMU.setBLDO1Voltage(3300);
  PMU.enableBLDO1();

  // Out to header pins
  MESH_DEBUG_PRINTLN("Setting and enabling b-ldo2 for output to header");
  PMU.setBLDO2Voltage(3300);
  PMU.enableBLDO2();

  MESH_DEBUG_PRINTLN("Setting and enabling dcdc4 for output to header");
  PMU.setDC4Voltage(XPOWERS_AXP2101_DCDC4_VOL2_MAX); // 1.8V
  PMU.enableDC4();

  MESH_DEBUG_PRINTLN("Setting and enabling dcdc5 for output to header");
  PMU.setDC5Voltage(3300);
  PMU.enableDC5();

  // Unused power rails
  MESH_DEBUG_PRINTLN("Disabling unused supplies dcdc2, dcdc5, dldo1 and dldo2");
  PMU.disableDC2();
  //PMU.disableDC5();
  PMU.disableDLDO1();
  PMU.disableDLDO2();

  PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

  // Set charge current to 500mA
  MESH_DEBUG_PRINTLN("Setting battery charge current limit and voltage");
  PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

  PMU.clearIrqStatus();
  PMU.disableTSPinMeasure();

  // enable battery voltage measurement
  MESH_DEBUG_PRINTLN("Enabling battery measurement");
  PMU.enableBattVoltageMeasure();
  PMU.enableVbusVoltageMeasure();

  // Reset and re-enable PMU interrupts
  MESH_DEBUG_PRINTLN("Re-enable interrupts");
  PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  PMU.clearIrqStatus();
  PMU.enableIRQ(
      XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |    // Battery interrupts
      XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS interrupts
      XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // Power Key interrupts
      XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // Charging interrupts
  );
#ifdef MESH_DEBUG
  scanDevices(&Wire); 
  scanDevices(&Wire1);
  printPMU();
#endif

  // Set the power key off press time
  PMU.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  return true;
}

static bool readStringUntil(Stream& s, char dest[], size_t max_len, char term, unsigned int timeout_millis) {
  unsigned long timeout = millis() + timeout_millis;
  char *dp = dest;
  while (millis() < timeout && dp - dest < max_len - 1) {
    if (s.available()) {
      char c = s.read();
      if (c == term) break;
      *dp++ = c;  // append to dest[]
    } else {
      delay(1);
    }
  }
  *dp = 0;  // null terminator
  return millis() < timeout;   // false, if timed out
}

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire1);
  return radio.std_init(&spi);
#endif
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

void TbeamSupSensorManager::start_gps()
{
  gps_active = true;
  pinMode(P_GPS_WAKE, OUTPUT);
  digitalWrite(P_GPS_WAKE, HIGH);
}

void TbeamSupSensorManager::sleep_gps() {
  gps_active = false;
  pinMode(P_GPS_WAKE, OUTPUT);
  digitalWrite(P_GPS_WAKE, LOW);
}

bool TbeamSupSensorManager::begin() {
  //init BME280
    if (! bme.begin(0x77, &Wire)) {
        MESH_DEBUG_PRINTLN("Could not find a valid BME280 sensor");
        bme_active = false;
    }
    else
      MESH_DEBUG_PRINTLN("BME280 found and init!");
      bme_active = true;
  
  // init GPS port
  Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, P_GPS_RX, P_GPS_TX);

  MESH_DEBUG_PRINTLN("Sleeping GPS for initial state");
  sleep_gps();
  return true;
}

bool TbeamSupSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  if (requester_permissions & TELEM_PERM_LOCATION && gps_active) {   // does requester have permission?
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, node_altitude);
  }
  if (requester_permissions & TELEM_PERM_ENVIRONMENT && bme_active) {   // does requester have permission?
    telemetry.addTemperature(TELEM_CHANNEL_SELF, node_temp);
    telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, node_hum);
    telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, node_pres);
    //telemetry.addAltitude(TELEM_CHANNEL_SELF, node_alt);
  }
  return true;
}

void TbeamSupSensorManager::loop() {
  static long next_update = 0;

  _nmea->loop();

  if (millis() > next_update) {
    if (_nmea->isValid() && gps_active) {
      node_lat = ((double)_nmea->getLatitude())/1000000.;
      node_lon = ((double)_nmea->getLongitude())/1000000.;
      node_altitude = ((double)_nmea->getAltitude()) / 1000.0;
      MESH_DEBUG_PRINT("lat %f lon %f alt %f\r\n", node_lat, node_lon, node_altitude);
    }

    //read BME280 values
    if(bme_active){
      //node_alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
      node_temp = bme.readTemperature();
      node_hum = bme.readHumidity();
      node_pres = (bme.readPressure() / 100.0F);
    
      #ifdef MESH_DEBUG
      // Serial.print("Temperature = ");
      // Serial.print(node_temp);
      // Serial.println(" *C");

      // Serial.print("Humidity = ");
      // Serial.print(node_hum);
      // Serial.println(" %");

      // Serial.print("Pressure = ");
      // Serial.print(node_pres);
      // Serial.println(" hPa");

      // Serial.print("Approx. Altitude = ");
      // Serial.print(node_alt);
      // Serial.println(" m");
      #endif
    }

    next_update = millis() + 1000;
  }
}

int TbeamSupSensorManager::getNumSettings() const {
  return 1; 
}

const char* TbeamSupSensorManager::getSettingName(int i) const {
  switch(i){
    case 0: return "gps";
    default: NULL;
  }
}

const char* TbeamSupSensorManager::getSettingValue(int i) const {
  switch(i){
    case 0: return gps_active == true ? "1" : "0";
    default: NULL;
  }
}

bool TbeamSupSensorManager::setSettingValue(const char* name, const char* value) {
  if (strcmp(name, "gps") == 0) {
    if (strcmp(value, "0") == 0) {
      sleep_gps();
    } else {
      start_gps();
    }
    return true;
  }
  return false;  // not supported
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
