
#ifndef EDUOS_OS_TIME_H
#define EDUOS_OS_TIME_H

#include "third-party/queue.h"

struct timer {
    TAILQ_ENTRY(timer) link;
    long usec_left;
    struct sched_task *task;
};

extern long get_uptime(void);

extern struct timer *new_timer(int seconds, struct sched_task *task, struct timer *tmr);

extern void time_init(void);

#endif /* EDUOS_OS_TIME_H */


