#pragma once

#include <RadioLib.h>
#include "RadioLibWrappers.h"

class CustomSX1262 : public SX1262 {
public:
  CustomSX1262(Module* mod) : SX1262(mod) {}
  uint8_t getSpreadingFactor() { return getSpreadingFactor(); }
};

class CustomSX1262Wrapper : public RadioLibWrapper {
public:
  CustomSX1262Wrapper(CustomSX1262& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }
#if defined(PLATFORM_NATIVE)
  float getCurrentRSSI() override { return 0.0f; }
  bool isReceivingPacket() override { return false; }
#endif
  float getLastRSSI() const override { return ((CustomSX1262 *)&_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomSX1262 *)&_radio)->getSNR(); }
  float packetScore(float snr, int packet_len) override {
    int sf = 10; // Use a default value for native
    return packetScoreInt(snr, sf, packet_len);
  }
};
