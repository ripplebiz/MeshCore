#pragma once

#include "CustomLLCC68.h"
#include "RadioLibWrappers.h"

class CustomLLCC68Wrapper : public RadioLibWrapper {
public:
  CustomLLCC68Wrapper(CustomLLCC68& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }
  bool isReceivingPacket() override { 
    return ((CustomLLCC68 *)_radio)->isReceiving();
  }
  float getCurrentRSSI() override {
    return ((CustomLLCC68 *)_radio)->getRSSI(false);
  }
  float getLastRSSI() const override { return ((CustomLLCC68 *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomLLCC68 *)_radio)->getSNR(); }
  int getCurrentSF() const override { return ((CustomLLCC68 *)_radio)->spreadingFactor; }
};
