#include "SimpleHardwareTimer.h"

volatile bool SimpleHardwareTimer::timer_expired = false;

void SimpleHardwareTimer::init() {
  NRF_RTC2->TASKS_STOP = 1;
  NRF_RTC2->TASKS_CLEAR = 1;
  
  // Set prescaler for ~1ms resolution (32768 Hz / 32 = 1024 Hz)
  NRF_RTC2->PRESCALER = 31;
  
  NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Msk;
  
  NVIC_SetPriority(RTC2_IRQn, 7);
  NVIC_ClearPendingIRQ(RTC2_IRQn);
  NVIC_EnableIRQ(RTC2_IRQn);
  
  NRF_RTC2->EVENTS_COMPARE[0] = 0;
  timer_expired = false;
}

void SimpleHardwareTimer::start(uint32_t timeout_ms) {
  timer_expired = false;
  
  NRF_RTC2->TASKS_STOP = 1;
  NRF_RTC2->TASKS_CLEAR = 1;
  
  NRF_RTC2->CC[0] = timeout_ms;
  NRF_RTC2->EVENTS_COMPARE[0] = 0;
  
  NRF_RTC2->TASKS_START = 1;
}

void SimpleHardwareTimer::stop() {
  NRF_RTC2->TASKS_STOP = 1;
  NRF_RTC2->EVENTS_COMPARE[0] = 0;
  timer_expired = false;
}

void SimpleHardwareTimer::rtc_handler() {
  if (NRF_RTC2->EVENTS_COMPARE[0]) {
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
    timer_expired = true;
    NRF_RTC2->TASKS_STOP = 1;
  }
}

extern "C" void RTC2_IRQHandler(void) {
  SimpleHardwareTimer::rtc_handler();
} 