#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "os.h"
#include "os/sched.h"
#include "os/irq.h"
#include "os/sem.h"
#include "os/syscall.h"
#include "os/filesys.h"

typedef long(*sys_call_t)(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest);

#define SYSCALL_X(x) \
	x(write) \
	x(read) \
	x(halt) \
	x(clone) \
	x(waitpid) \
	x(exit) \
	x(wait) \
	x(get_file_descr) \
	x(fclose_by_descr) \
	x(task_id) \
	x(sem_init) \
	x(sem_use) \
	x(sem_free) \


#define ENUM_LIST(name) os_syscall_nr_ ## name,
enum syscalls_num {
	SYSCALL_X(ENUM_LIST)
};
#undef ENUM_LIST

static long errwrap(long res) {
	return res == -1 ? -errno : res;
}

static long sys_write(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	int fd = (int) arg1;
	const char *msg = (const char *) arg2;
	return errwrap(write(fd, msg, strlen(msg)));
}

static void read_irq_hnd(void *arg) {
	sched_notify(arg);
}

static long sys_read(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	int fd = (int) arg1;
	void *buffer = (void *) arg2;
	const int size = (int) arg3;
    
	irqmask_t cur = irq_disable();

	int bytes = errwrap(read(fd, buffer, size));
	while (bytes == -EAGAIN) {
		irq_set_hnd(read_irq_hnd, sched_current());
		sched_wait();
		sched();
		bytes = errwrap(read(STDIN_FILENO, buffer, size));
	}

	irq_enable(cur);
	return bytes;
}

static long sys_clone(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {

	irqmask_t cur = irq_disable();

	sched_task_entry_t entry = (sched_task_entry_t) arg1;
	void *arg = (void *) arg2;

	struct sched_task *task = sched_add(entry, arg);
	task->parent = sched_current();

	irq_enable(cur);

	return get_task_id(task);

}

static long sys_waitpid(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {

	irqmask_t cur = irq_disable();

	int task_id = arg1;
	struct sched_task *task = get_task(task_id);

	while (task->state != SCHED_FINISH) {
		sched_wait();
		sched();
	}

	task->state = SCHED_EMPTY;

	irq_enable(cur);

	return task->exit_status;
}

static long sys_halt(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	exit(arg1);

}

static long sys_exit(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {

	irqmask_t irq = irq_disable();

	int status = (int) arg1;

	struct sched_task *cur_task = sched_current();
	cur_task->exit_status = status;
	remove_task(cur_task);
	sched_notify(cur_task->parent);

	sched();
	irq_enable(irq);
	
	return 0;
}

static long sys_wait(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	irqmask_t irq = irq_disable();
	sched();
	irq_enable(irq);
	return 0;
}

static long sys_get_file_descr(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {

	char *file_name = (char *) arg1;
	int flags = (int) arg2;
	
	char path [256];
	get_absolute_path(file_name, path);
	
	int d = open(path, flags);
	
	return d;
}

static long sys_fclose_by_descr(int syscall,
	unsigned long arg1, unsigned long arg2,
	unsigned long arg3, unsigned long arg4,
	void *rest) {
		
	return close((int)arg1);
}
  
static long sys_task_id(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	return get_task_id(sched_current());
}

static long sys_sem_init(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	return sem_init(arg1);
}

static long sys_sem_use(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	return sem_use(arg1, arg2);
}

static long sys_sem_free(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	return sem_free(arg1);
}



#define TABLE_LIST(name) sys_ ## name,
static const sys_call_t sys_table[] = {
	SYSCALL_X(TABLE_LIST)
};
#undef TABLE_LIST

static long os_syscall(int syscall,
		unsigned long arg1, unsigned long arg2,
		unsigned long arg3, unsigned long arg4,
		void *rest) {
	long ret;
	__asm__ __volatile__(
		"int $0x81\n"
		: "=a"(ret)
		: "a"(syscall), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(rest)
		:
	);
	return ret;
}

int os_sys_write(int fd, const char *msg) {
	return os_syscall(os_syscall_nr_write, fd, (unsigned long) msg, 0, 0, NULL);
}

int os_sys_read(int fd, char *buffer, int size) {
	return os_syscall(os_syscall_nr_read, fd, (unsigned long) buffer, size, 0, NULL);
}

int os_get_file_descr(const char *file_name, int flags) {
	return os_syscall(os_syscall_nr_get_file_descr, (unsigned long) file_name, (unsigned long) flags, 0, 0, NULL);
}

int os_fclose_by_descr(int fd) {
	return os_syscall(os_syscall_nr_fclose_by_descr, fd, 0, 0, 0, NULL);
}

int os_clone(void (*fn) (void *arg), void *arg) {
	return os_syscall(os_syscall_nr_clone, (unsigned long) fn, (unsigned long) arg,
	0, 0, NULL);
}

int os_waitpid(int task_id) {
	return os_syscall(os_syscall_nr_waitpid, task_id, 0, 0, 0, NULL);
}

int os_halt(int status) {
	return os_syscall(os_syscall_nr_halt, status, 0, 0, 0, NULL);
}

int os_exit(int status) {
	return os_syscall(os_syscall_nr_exit, status, 0, 0, 0, NULL);
}

int os_wait(void) {
	return os_syscall(os_syscall_nr_wait, 0, 0, 0, 0, NULL);
}

int os_task_id(void) {
	return os_syscall(os_syscall_nr_task_id, 0, 0, 0, 0, NULL);
}

int os_sem_init(int cnt) {
	return os_syscall(os_syscall_nr_sem_init, cnt, 0, 0, 0, NULL);
}

int os_sem_use(int semid, int add) {
	return os_syscall(os_syscall_nr_sem_use, semid, add, 0, 0, NULL);
}

int os_sem_free(int semid) {
	return os_syscall(os_syscall_nr_sem_free, semid, 0, 0, 0, NULL);
}

static void os_sighnd(int sig, siginfo_t *info, void *ctx) {
	ucontext_t *uc = (ucontext_t *) ctx;
	greg_t *regs = uc->uc_mcontext.gregs;

	if (0x81cd == *(uint16_t *) regs[REG_RIP]) {
		int ret = sys_table[regs[REG_RAX]](regs[REG_RAX],
				regs[REG_RBX], regs[REG_RCX],
				regs[REG_RDX], regs[REG_RSI],
				(void *) regs[REG_RDI]);
		regs[REG_RAX] = ret;
		regs[REG_RIP] += 2;
	} else {
		abort();
	}
}

int syscall_init(void) {
	struct sigaction act = {
		.sa_sigaction = os_sighnd,
		.sa_flags = SA_RESTART,
	};
	sigemptyset(&act.sa_mask);

	if (-1 == sigaction(SIGSEGV, &act, NULL)) {
		perror("signal set failed");
		exit(1);
	}
	return 0;
}