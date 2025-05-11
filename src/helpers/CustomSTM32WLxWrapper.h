#pragma once

#include "CustomSTM32WLx.h"
#include "RadioLibWrappers.h"
#include <math.h>

class CustomSTM32WLxWrapper : public RadioLibWrapper {
public:
  CustomSTM32WLxWrapper(CustomSTM32WLx& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }
  bool isReceiving() override { 
    if (((CustomSTM32WLx *)_radio)->isReceiving()) return true;

    idle();  // put sx126x into standby
    // do some basic CAD (blocks for ~12780 micros (on SF 10)!)
    bool activity = (((CustomSTM32WLx *)_radio)->scanChannel() == RADIOLIB_LORA_DETECTED);
    if (activity) {
      startRecv();
    } else {
      idle();
    }
    return activity;
  }
  float getLastRSSI() const override { return ((CustomSTM32WLx *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomSTM32WLx *)_radio)->getSNR(); }

  float packetScore(float snr, int packet_len) override {
    int sf = ((CustomSTM32WLx *)_radio)->spreadingFactor;
    return packetScoreInt(snr, sf, packet_len);
  }
};
