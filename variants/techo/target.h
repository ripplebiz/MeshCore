#pragma once

#include <helpers/nrf52/TechoBoard.h>
#include <helpers/CustomSX1262Wrapper.h>

extern TechoBoard board;
extern RADIO_CLASS radio;

bool radio_init();
