
#ifndef EDUOS_OS_TIME_H
#define EDUOS_OS_TIME_H

struct timer {
    TAILQ_ENTRY (timers) link;
    int ticks;
}

extern void time_init(void);

#endif /* EDUOS_OS_TIME_H */


