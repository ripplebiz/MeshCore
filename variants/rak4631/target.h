#pragma once

#include <helpers/nrf52/RAK4631Board.h>
#include <helpers/CustomSX1262Wrapper.h>

extern RAK4631Board board;
extern RADIO_CLASS radio;

bool radio_init();
