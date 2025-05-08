#pragma once

#include <RadioLib.h>

#define LR1110_IRQ_HAS_PREAMBLE                     0b0000000100  //  4     4     valid LoRa header received
#define LR1110_IRQ_HEADER_VALID                     0b0000010000  //  4     4     valid LoRa header received

class CustomLR1110 : public LR1110 {
  public:
    CustomLR1110(Module *mod) : LR1110(mod) { }

    bool isReceiving() {
      uint16_t irq = getIrqStatus();
      bool detected = ((irq & LR1110_IRQ_HEADER_VALID) || (irq & LR1110_IRQ_HAS_PREAMBLE));
      return detected;
    }
};