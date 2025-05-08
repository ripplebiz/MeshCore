#pragma once

#include "CustomLR1110.h"
#include "RadioLibWrappers.h"

class CustomLR1110Wrapper : public RadioLibWrapper {
public:
  CustomLR1110Wrapper(CustomLR1110& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }
  bool isReceiving() override { 
    if (((CustomLR1110 *)_radio)->isReceiving()) return true;

    idle();  // put sx126x into standby
    // do some basic CAD (blocks for ~12780 micros (on SF 10)!)
    bool activity = (((CustomLR1110 *)_radio)->scanChannel() == RADIOLIB_LORA_DETECTED);
    if (activity) {
      startRecv();
    } else {
      idle();
    }
    return activity;
  }

  void onSendFinished() override {
    RadioLibWrapper::onSendFinished();
    _radio->setPreambleLength(8); // overcomes weird issues with small and big pkts
  }

  float getLastRSSI() const override { return ((CustomLR1110 *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomLR1110 *)_radio)->getSNR(); }
  int16_t setRxBoostedGainMode(bool en) { return ((CustomLR1110 *)_radio)->setRxBoostedGainMode(en); };
};
