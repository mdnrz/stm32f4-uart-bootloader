#ifndef __RES_TIMER_H__
#define __RES_TIMER_H__

#include <libopencm3/stm32/timer.h>

#define capTimLo TIM1
#define capTimHi TIM2

void res_captureTimer_setup(void);
void res_timers_reconfig(void);
#endif
