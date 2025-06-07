#include <Arduino.h>
#include "target.h"

TBeamBoard board;

// Using PMU AXP2102
#define PMU_WIRE_PORT               Wire

bool pmuIntFlag = false;
static void setPMUIntFlag(){
  pmuIntFlag = true;
}

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RESET, P_LORA_DIO_1, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RESET, P_LORA_DIO_1);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
#endif

#ifndef LORA_CR
  #define LORA_CR      5
#endif

bool TBeamBoard::power_init()
{
  if (!PMU)
  {
    PMU = new XPowersAXP2101(PMU_WIRE_PORT);
    if (!PMU->init())
    {
      // Serial.println("Warning: Failed to find AXP2101 power management");
      delete PMU;
      PMU = NULL;
    }
    else
    {
      // Serial.println("AXP2101 PMU init succeeded, using AXP2101 PMU");
    }
  }
  if (!PMU)
  {
    PMU = new XPowersAXP192(PMU_WIRE_PORT);
    if (!PMU->init())
    {
      // Serial.println("Warning: Failed to find AXP192 power management");
      delete PMU;
      PMU = NULL;
    }
    else
    {
      // Serial.println("AXP192 PMU init succeeded, using AXP192 PMU");
    }
  }

  if (!PMU)
  {
    MESH_DEBUG_PRINTLN("PMU init failed.");
    return false;
  }

  // Serial.printf("PMU  ID:0x%x\n", PMU->getChipID());
  // printPMU();
  if (PMU->getChipModel() == XPOWERS_AXP192)
  {
    // lora radio power channel
    PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
    PMU->enablePowerOutput(XPOWERS_LDO2);
    // oled module power channel,
    // disable it will cause abnormal communication between boot and AXP power supply,
    // do not turn it off
    PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
    // enable oled power
    PMU->enablePowerOutput(XPOWERS_DCDC1);
    // gnss module power channel
    PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
    // power->enablePowerOutput(XPOWERS_LDO3);
    // protected oled power source
    PMU->setProtectedChannel(XPOWERS_DCDC1);
    // protected esp32 power source
    PMU->setProtectedChannel(XPOWERS_DCDC3);
    // disable not use channel
    PMU->disablePowerOutput(XPOWERS_DCDC2);
    // disable all axp chip interrupt
    PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);
    PMU->setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_550MA);
  }
  else if (PMU->getChipModel() == XPOWERS_AXP2101)
  {
    // gnss module power channel
    PMU->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO4);
    // lora radio power channel
    PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO3);
    // m.2 interface
    PMU->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
    PMU->enablePowerOutput(XPOWERS_DCDC3);
    // power->setPowerChannelVoltage(XPOWERS_DCDC4, 3300);
    // power->enablePowerOutput(XPOWERS_DCDC4);
    // not use channel
    PMU->disablePowerOutput(XPOWERS_DCDC2); // not elicited
    PMU->disablePowerOutput(XPOWERS_DCDC5); // not elicited
    PMU->disablePowerOutput(XPOWERS_DLDO1); // Invalid power channel, it does not exist
    PMU->disablePowerOutput(XPOWERS_DLDO2); // Invalid power channel, it does not exist
    PMU->disablePowerOutput(XPOWERS_VBACKUP);
    // disable all axp chip interrupt
    PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);

    // Set up PMU interrupts
    // Serial.println("Setting up PMU interrupts");
    pinMode(PIN_PMU_IRQ, INPUT_PULLUP);
    attachInterrupt(PIN_PMU_IRQ, setPMUIntFlag, FALLING);

    // Reset and re-enable PMU interrupts
    // Serial.println("Re-enable interrupts");
    PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    PMU->clearIrqStatus();
    PMU->enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |    // Battery interrupts
        XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS interrupts
        XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // Power Key interrupts
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // Charging interrupts
    );
  }

  return true;
}

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);

#if defined(P_LORA_SCLK)
  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
#endif
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;  // fail
  }

#ifdef SX127X_CURRENT_LIMIT
  radio.setCurrentLimit(SX127X_CURRENT_LIMIT);
#endif

  radio.setCRC(1);

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

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}

#ifdef MESH_DEBUG
void TBeamBoard::printPMU()
{
    Serial.print("isCharging:"); Serial.println(PMU->isCharging() ? "YES" : "NO");
    Serial.print("isDischarge:"); Serial.println(PMU->isDischarge() ? "YES" : "NO");
    Serial.print("isVbusIn:"); Serial.println(PMU->isVbusIn() ? "YES" : "NO");
    Serial.print("getBattVoltage:"); Serial.print(PMU->getBattVoltage()); Serial.println("mV");
    Serial.print("getVbusVoltage:"); Serial.print(PMU->getVbusVoltage()); Serial.println("mV");
    Serial.print("getSystemVoltage:"); Serial.print(PMU->getSystemVoltage()); Serial.println("mV");

    // The battery percentage may be inaccurate at first use, the PMU will automatically
    // learn the battery curve and will automatically calibrate the battery percentage
    // after a charge and discharge cycle
    if (PMU->isBatteryConnect()) {
        Serial.print("getBatteryPercent:"); Serial.print(PMU->getBatteryPercent()); Serial.println("%");
    }

    Serial.println();
}
#endif