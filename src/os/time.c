
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#include "os/time.h"
#include "os/sched.h"
#include "os.h"

struct timer *single;

static void os_sigalrmhnd(int signal, siginfo_t *info, void *ctx) {
	struct itimerval cur_it;
	getitimer(ITIMER_REAL, &cur_it);
	//TODO: remove timer

	//TODO: move into function
	single->sec_left = 0;
	if(single->task != NULL && single->task->state == SCHED_SLEEP) {
		sched_notify(single->task);
		sched();
	}
	//TODO: go through all list of timers and decrement.
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

	set_timer(0);
}
