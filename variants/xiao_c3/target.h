#pragma once

#include <helpers/XiaoC3Board.h>
#include <helpers/CustomSX1262Wrapper.h>
#include <helpers/CustomSX1268Wrapper.h>

extern XiaoC3Board board;
extern RADIO_CLASS radio;

bool radio_init();
