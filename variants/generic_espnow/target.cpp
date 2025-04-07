#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>

ESP32Board board;

ESPNOWRadio radio_driver;

ESP32RTCClock rtc_clock;

bool radio_init() {
  rtc_clock.begin();

  // NOTE: radio_driver.begin() is called by Dispatcher::begin(), so not needed here
  return true;  // success
}

uint32_t radio_get_rng_seed() {
  return millis() + radio_driver.intID();  // TODO: where to get some entropy?
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  // no-op
}

void radio_set_tx_power(uint8_t dbm) {
  radio_driver.setTxPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  StdRNG  rng;  // TODO: need stronger True-RNG here
  return mesh::LocalIdentity(&rng);  // create new random identity
}
