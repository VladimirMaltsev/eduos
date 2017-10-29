
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

#include "os/time.h"
#include "os/sched.h"
#include "os.h"

#define USEC_IN_SEC 1000000

#define INTERVAL_SEC 1
#define INTERVAL_USEC 0

struct {
	TAILQ_HEAD(timerlist, timer) head;
} timers;

extern void sched_tramp(void);

long uptime;

long timeval_to_usec(struct timeval t) {
	return (long) (t.tv_sec*USEC_IN_SEC + t.tv_usec);
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

void decrement_timers_and_notify(long delta) {
	struct timer *cur, *tmp;
	TAILQ_FOREACH_SAFE(cur, &timers.head, link, tmp) {
		assert(cur->usec_left > 0);
		cur->usec_left -= delta;
		if(cur->usec_left <= 0 && cur->task->state == SCHED_SLEEP) {
			sched_notify(cur->task);
			TAILQ_REMOVE(&timers.head, cur, link); //mb do removals after sigalrmhnd
		}
	}
}



void handle_timers(long time_delta) {
	decrement_timers_and_notify(time_delta);
	if(TAILQ_EMPTY(&timers.head)) 
		set_timer_repeat(INTERVAL_SEC, INTERVAL_USEC);
	else {
		long smallest_time = TAILQ_FIRST(&timers.head)->usec_left;
		set_timer_once(smallest_time / USEC_IN_SEC, smallest_time % USEC_IN_SEC);
	}
}

static void os_sigalrmhnd(int signal, siginfo_t *info, void *ctx) {
	long time_delta;
	if(TAILQ_EMPTY(&timers.head))
		time_delta = INTERVAL_SEC*USEC_IN_SEC + INTERVAL_USEC;
	else
		time_delta = TAILQ_FIRST(&timers.head)->usec_left;
	uptime += time_delta;
	handle_timers(time_delta);

	ucontext_t *uc = (ucontext_t *) ctx;
	greg_t *regs = uc->uc_mcontext.gregs;

	regs[REG_RSP] -= 8;
	*(unsigned long*) regs[REG_RSP] = regs[REG_RIP];
	regs[REG_RIP] = (greg_t) sched_tramp;
}

void insert_sorted(struct timer *tmr) {
	struct timer *cur;
	TAILQ_FOREACH(cur, &timers.head, link) {
		if(tmr->usec_left < cur->usec_left) {
			TAILQ_INSERT_BEFORE(cur, tmr, link);
			return;
		}
	}
	TAILQ_INSERT_TAIL(&timers.head, tmr, link);
}

struct timer *new_timer(int seconds, struct sched_task *task, struct timer *tmr) {
	*tmr = (struct timer) {.usec_left = seconds*USEC_IN_SEC, .task = task};
	if(TAILQ_EMPTY(&timers.head) || (tmr->usec_left < TAILQ_FIRST(&timers.head)->usec_left)) {
		uptime += time_since_last_irq();
		decrement_timers_and_notify(time_since_last_irq());
		TAILQ_INSERT_HEAD(&timers.head, tmr, link);
		set_timer_once(tmr->usec_left / USEC_IN_SEC, tmr->usec_left % USEC_IN_SEC);
	} else {
		insert_sorted(tmr);
	}

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
	struct timeval value = {.tv_sec = sec, .tv_usec = usec};
	struct timeval interval = {0, 0};
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
	set_timer_repeat(INTERVAL_SEC, INTERVAL_USEC);
}
