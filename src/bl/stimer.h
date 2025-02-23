#ifndef __STIMER_H__
#define __STIMER_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool autoReset;
    bool hasElapsed;
    uint32_t waitTime;
    uint32_t targetTime;
} Stimer_t;

void stimer_systick_init(void);
void stimer_start(Stimer_t *timer, const uint32_t waitTime, const bool autoReset);
bool stimer_ping(Stimer_t *timer);
void stimer_reset(Stimer_t *timer);
void stimer_delay(const uint32_t delay);
void stimer_reload(Stimer_t *timer);
#endif // __STIMER_H__
