#pragma once

#include <RadioLib.h>

#define RH_RF95_MODEM_STATUS_CLEAR               0x10
#define RH_RF95_MODEM_STATUS_HEADER_INFO_VALID   0x08
#define RH_RF95_MODEM_STATUS_RX_ONGOING          0x04
#define RH_RF95_MODEM_STATUS_SIGNAL_SYNCHRONIZED 0x02
#define RH_RF95_MODEM_STATUS_SIGNAL_DETECTED     0x01

class CustomSX1276 : public SX1276 {
  public:
    CustomSX1276(Module *mod) : SX1276(mod) { }

    bool isReceiving() {
      return (getModemStatus() &
         (RH_RF95_MODEM_STATUS_SIGNAL_DETECTED 
        | RH_RF95_MODEM_STATUS_SIGNAL_SYNCHRONIZED 
        | RH_RF95_MODEM_STATUS_HEADER_INFO_VALID)) != 0;
    }

    int tryScanChannel() {
      // start CAD
      int16_t state = startChannelScan();
      RADIOLIB_ASSERT(state);
 
      // wait for channel activity detected or timeout
      unsigned long timeout = millis() + 16;
      while(!this->mod->hal->digitalRead(this->mod->getIrq()) && millis() < timeout) {
        this->mod->hal->yield();
        if(this->mod->hal->digitalRead(this->mod->getGpio())) {
          return(RADIOLIB_PREAMBLE_DETECTED);
        }
      }
      return 0; // timed out
    }
};
