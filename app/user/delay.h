#ifndef _delay_h
#define _delay_h

#include "scr1_timer.h"
#include "mik32_memory_map.h"

#define SCR1_TIMER_GET_TIME()                                                  \
  (((uint64_t)(SCR1_TIMER->MTIMEH) << 32) | (SCR1_TIMER->MTIME))

#define MCU_CLOCK 32000000U

#define CYCLES_PER_MILLISECOND ((MCU_CLOCK) / 1000U)
#define CYCLES_PER_MICROSECOND ((MCU_CLOCK) / 1000000U)

void delayMicroseconds(uint32_t microseconds);

#endif