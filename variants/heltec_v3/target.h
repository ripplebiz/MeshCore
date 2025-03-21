#pragma once

#include <helpers/HeltecV3Board.h>
#include <helpers/CustomSX1262Wrapper.h>

extern HeltecV3Board board;
extern RADIO_CLASS radio;

bool radio_init();
