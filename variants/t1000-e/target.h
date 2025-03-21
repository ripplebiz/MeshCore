#pragma once

#include <helpers/nrf52/T1000eBoard.h>
#include <helpers/CustomLR1110Wrapper.h>

extern T1000eBoard board;
extern RADIO_CLASS radio;

bool radio_init();
