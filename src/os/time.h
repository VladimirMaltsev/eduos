
#ifndef EDUOS_OS_TIME_H
#define EDUOS_OS_TIME_H

struct timer {
    int sec_left;
    struct sched_task *task;
};

int set_timer(int seconds);
extern struct timer *new_timer(int seconds, struct sched_task *task, struct timer *tmr);

extern void time_init(void);

#endif /* EDUOS_OS_TIME_H */


