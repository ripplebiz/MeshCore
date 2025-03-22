#pragma once

#include <helpers/nrf52/FaketecBoard.h>
#include <helpers/CustomSX1262Wrapper.h>
#include <helpers/CustomLLCC68Wrapper.h>

extern FaketecBoard board;
extern RADIO_CLASS radio;

bool radio_init();
