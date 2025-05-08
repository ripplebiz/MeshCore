#pragma once

#include "CustomLLCC68.h"
#include "RadioLibWrappers.h"

class CustomLLCC68Wrapper : public RadioLibWrapper {
public:
  CustomLLCC68Wrapper(CustomLLCC68& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }
  bool isReceiving() override { 
    if (((CustomLLCC68 *)_radio)->isReceiving()) return true;

    idle();  // put sx126x into standby
    // do some basic CAD (blocks for ~12780 micros (on SF 10)!)
    bool activity = (((CustomLLCC68 *)_radio)->scanChannel() == RADIOLIB_LORA_DETECTED);
    if (activity) {
      startRecv();
    } else {
      idle();
    }
    return activity;
  }
  float getLastRSSI() const override { return ((CustomLLCC68 *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomLLCC68 *)_radio)->getSNR(); }

  float packetScore(float snr, int packet_len) override {
    int sf = ((CustomLLCC68 *)_radio)->spreadingFactor;
    return packetScoreInt(snr, sf, packet_len);
  }
};
