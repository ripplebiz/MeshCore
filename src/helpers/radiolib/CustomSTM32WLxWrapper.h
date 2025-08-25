#pragma once

#include "CustomSTM32WLx.h"
#include "RadioLibWrappers.h"
#include <math.h>

class CustomSTM32WLxWrapper : public RadioLibWrapper {
public:
  CustomSTM32WLxWrapper(CustomSTM32WLx& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }
  bool isReceivingPacket() override { 
    return ((CustomSTM32WLx *)_radio)->isReceiving();
  }
  float getCurrentRSSI() override {
    return ((CustomSTM32WLx *)_radio)->getRSSI(false);
  }
  float getLastRSSI() const override { return ((CustomSTM32WLx *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomSTM32WLx *)_radio)->getSNR(); }
  int getCurrentSF() const override { return ((CustomSTM32WLx *)_radio)->spreadingFactor; }
};
