#include "delay.h"

void delayMicroseconds(uint32_t microseconds) {
    uint64_t end_time = SCR1_TIMER_GET_TIME() + (microseconds * CYCLES_PER_MICROSECOND);
    while (SCR1_TIMER_GET_TIME() < end_time)
        ;
}

void delayMs(uint32_t ms) {
  uint64_t end_time = SCR1_TIMER_GET_TIME() + (ms * CYCLES_PER_MILLISECOND);
  while (SCR1_TIMER_GET_TIME() < end_time)
      ;
}