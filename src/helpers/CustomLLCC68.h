#pragma once

#include <RadioLib.h>

#define SX126X_IRQ_HEADER_VALID                     0b0000010000  //  4     4     valid LoRa header received

class CustomLLCC68 : public LLCC68 {
  public:
    CustomLLCC68(Module *mod) : LLCC68(mod) { }

    bool isReceiving() {
      uint16_t irq = getIrqFlags();
      bool hasPreamble = (irq & SX126X_IRQ_HEADER_VALID);
      return hasPreamble;
    }
};