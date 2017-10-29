
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

#include "os/time.h"
#include "os/sched.h"
#include "os.h"

#define INTERVAL_USEC 10000 //alarm rings every 10ms

struct {
	TAILQ_HEAD(timerlist, timer) head;
} timers;

extern void sched_tramp(void);

struct timer *single;
long uptime;

long timeval_to_usec(struct timeval t) {
	return (long) (t.tv_sec*1000000 + t.tv_usec);
}

long time_since_last_irq() {
	struct itimerval cur_it;
	getitimer(ITIMER_REAL, &cur_it);
	return timeval_to_usec(cur_it.it_value);
}

long get_uptime(void) {
	long t = time_since_last_irq();
	return uptime + t;
}

static void handle_timers(long time_delta) {
	//TODO: go through all list of timers and decrement.
	//TODO: remove timer and notify task
	struct timer *cur, *tmp;
	TAILQ_FOREACH_SAFE(cur, &timers.head, link, tmp) {
		assert(cur->usec_left > 0);
		cur->usec_left -= time_delta;
		if(cur->usec_left <= 0 && cur->task->state == SCHED_SLEEP) {
			sched_notify(cur->task);
			TAILQ_REMOVE(&timers.head, cur, link); //mb do removals after sigalrmhnd
		}
	}
}

static void os_sigalrmhnd(int signal, siginfo_t *info, void *ctx) {
	long time_delta = time_since_last_irq();
	uptime += time_delta;
	handle_timers(time_delta);

	ucontext_t *uc = (ucontext_t *) ctx;
	greg_t *regs = uc->uc_mcontext.gregs;

	regs[REG_RSP] -= 8;
	*(unsigned long*) regs[REG_RSP] = regs[REG_RIP];
	regs[REG_RIP] = (greg_t) sched_tramp;
}

struct timer *new_timer(int seconds, struct sched_task *task, struct timer *tmr) {
	//TODO: insert into list, if the lowest time, then set timer accordingly
	*tmr = (struct timer) {.usec_left = seconds*1000000, .task = task};
	TAILQ_INSERT_HEAD(&timers.head, tmr, link);

	return single;
}

int set_timer(struct timeval value, struct timeval interval) {
	const struct itimerval setup_it = {
		.it_value    = value,
		.it_interval = interval
	};

	if (-1 == setitimer(ITIMER_REAL, &setup_it, NULL)) {
		perror("SIGALRM set failed");
		exit(1);
	}

	return 0;
}

int set_timer_once(int sec, int usec) {
	struct timeval interval = {.tv_sec = sec, .tv_usec = usec};
	struct timeval value = {0, 0};
	return set_timer(value, interval);
}

int set_timer_repeat(int sec, int usec) {
	struct timeval t = {.tv_sec = sec, .tv_usec = usec};
	return set_timer(t, t);
}


void time_init(void) {
	TAILQ_INIT(&timers.head);

	struct sigaction alrmact = {
		.sa_sigaction = os_sigalrmhnd,
	};
	sigemptyset(&alrmact.sa_mask);
	if (-1 == sigaction(SIGALRM, &alrmact, NULL)) {
		perror("SIGALRM set failed");
		exit(1);
	}

	uptime = 0;
	set_timer_repeat(0, INTERVAL_USEC);
}
