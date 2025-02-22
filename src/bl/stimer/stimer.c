#include "stimer.h"
#include "libopencm3/stm32/rcc.h"
#include "libopencm3/cm3/systick.h"
#include "libopencm3/cm3/vector.h"

volatile uint32_t ticks;

void sys_tick_handler(void) {
    ticks++;
}

// Get current tick value
static uint32_t systick_get_ticks(void) {
    return ticks;
}

// Initialize systick peripheral 
void stimer_systick_init(void) {
    systick_set_frequency(1000, rcc_ahb_frequency);
    systick_counter_enable();
    systick_interrupt_enable();
}

// Setup and start a software timer
void stimer_start(Stimer_t *timer, uint32_t waitTime, bool autoReset) {
    timer->autoReset = autoReset;
    timer->waitTime = waitTime;
    timer->targetTime = systick_get_ticks() + waitTime;
    timer->hasElapsed = false;
}

// Reset timer counter to zero
void stimer_reload(Stimer_t *timer) {
    if (!timer->hasElapsed) timer->targetTime = systick_get_ticks() + timer->waitTime;
}

// Returns true of timer has elapsed
bool stimer_ping(Stimer_t *timer) {
    if (timer->hasElapsed) return false;
    uint32_t now = systick_get_ticks();
    bool hasElapsed = now >= timer->targetTime;
    if (hasElapsed) {
        if (timer->autoReset) {
            uint32_t drift = now - timer->targetTime;
            timer->targetTime = (now + timer->waitTime) - drift;
        } else {
            timer->hasElapsed = true;
        }
    }
    return hasElapsed;
}

// Resets software timers counter
void stimer_reset(Stimer_t *timer) {
    stimer_start(timer, timer->waitTime, timer->autoReset);
}

// Blocking delay
void stimer_delay(uint32_t delay) {
    Stimer_t timer;
    stimer_start(&timer, delay, false);
    while(!stimer_ping(&timer));
}
