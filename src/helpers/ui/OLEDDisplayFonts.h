#ifndef OLEDDISPLAYFONTS_h
#define OLEDDISPLAYFONTS_h

#ifdef ARDUINO
#include <Arduino.h>
#elif __MBED__
#define PROGMEM
#endif

extern const uint8_t ArialMT_Plain_10[] PROGMEM;
extern const uint8_t ArialMT_Plain_16[] PROGMEM;
extern const uint8_t ArialMT_Plain_24[] PROGMEM;
#endif
