
#include <errno.h>

#include <stdbool.h>
#include <stdlib.h>
#include "os/sched.h"
#include "os/sem.h"
#include "os/irq.h"

struct wait_queue {
	TAILQ_HEAD(w_listhead, sched_task) head;
}

struct sem {
	bool used;
	struct wait_queue wq;
	int semid;
	int val;
	int max_val;
};

struct sem g_sem;

int sem_init(int cnt) {
	if (cnt < 1) return -EINVAL;

	struct sem *s = NULL;
	irqmask_t irq = irq_disable();
	
	if (!g_sem.used) {
		s = &g_sem;
		s->used = true;
	}
	irq_enable(irq);
	if (!s) {
		return -ENOMEM;
	}

	s->val = cnt;
	s->max_val = cnt;

	return 0;
}
int sem_down (int semid) {
	irqmask_t irq = irq_disable();

	if (semid != 0 || !g_sem.used) {
		irq_enable(irq);
		return -EINVAL;
	}
	while (true) {
		if (g_sem.val > 0) {
			g.sem.val --;
			break;
		} else {
			struct sched_task *curr = sched_current();
			cur->wq = &g_sem.wq;
			TAILQ_INSERT_TAIL (g.sem.wq->head, sched_current(), link);
			sched_wait();
			sched();
		}
	}
	irq_enable(irq);
	return 0;
}

int sem_up (int semid) {
	irqmask_t irq = irq_disable();
	
	if (semid != 0 || !g_sem.used) {
		irq_enable(irq);
		return -EINVAL;
	}

	if (g.sem.val < g.sem.max_val) 
		g.sem.val ++;

	if (!TAILQ_EMPTY(&q_sem.wq->head)) {
		struct sched_task *task = TAILQ_FIRST(&g_sem.wq->head);
		TAILQ_REMOVE (&g_sem.wq->head, task, link);
		sched_notify(task);
		
	}
		
	irq_enable(irq);
	return 0;
}

int sem_use(int semid, int add) {
	if (semid != 0 || !g_sem.used) {
		return -EINVAL;
	}

	return 0;
}

int sem_free(int semid) {
	if (semid != 0 || !g_sem.used) {
		return -EINVAL;
	}
	g_sem.used = false;
	return 0;
}

