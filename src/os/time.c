
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#include "os/time.h"
#include "os/sched.h"
#include "os.h"

extern void sched_tramp(void);

struct timer *single;
long init_time;

long timeval_to_usec(struct timeval t) {
	return (long) (t.tv_sec*1000000 + t.tv_usec);
}

long get_init_time() {
	return init_time;
}

long get_current_time() {
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	return timeval_to_usec(cur_time);
}

static void handle_timers() {
	//TODO: go through all list of timers and decrement.
	//TODO: remove timer and notify task
	single->sec_left = 0;
	if(single->task != NULL && single->task->state == SCHED_SLEEP) {
		sched_notify(single->task);
	}
}

static void os_sigalrmhnd(int signal, siginfo_t *info, void *ctx) {
	handle_timers();

	ucontext_t *uc = (ucontext_t *) ctx;
	greg_t *regs = uc->uc_mcontext.gregs;

	regs[REG_RSP] -= 8;
	*(unsigned long*) regs[REG_RSP] = regs[REG_RIP];
	regs[REG_RIP] = (greg_t) sched_tramp;
}

struct timer *new_timer(int seconds, struct sched_task *task, struct timer *tmr) {
	//TODO: insert into list, if the lowest time, then set timer accordingly
	*tmr = (struct timer) {.sec_left = seconds, .task = task};
	single = tmr;

	set_timer(seconds);
	return single;
}

int set_timer(int seconds) {
	const struct itimerval setup_it = {
		.it_value    = { seconds /*sec*/, 0 /*usec*/},
	};

	if (-1 == setitimer(ITIMER_REAL, &setup_it, NULL)) {
		perror("SIGALRM set failed");
		exit(1);
	}

	return 0;
}


void time_init(void) {
	struct sigaction alrmact = {
		.sa_sigaction = os_sigalrmhnd,
	};
	sigemptyset(&alrmact.sa_mask);
	if (-1 == sigaction(SIGALRM, &alrmact, NULL)) {
		perror("SIGALRM set failed");
		exit(1);
	}
	init_time = get_current_time();
	set_timer(0);
}
