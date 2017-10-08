
#include <string.h>
#include <stdio.h>

#include <stdbool.h>

#include "os.h"
#include "os/sched.h"

#include "apps.h"

extern char *strtok_r(char *str, const char *delim, char **saveptr);

static int echo(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		os_sys_write(argv[i]);
		os_sys_write(i == argc - 1 ? "\n" : " ");
	}
	return 0;
}

static int sleep(int argc, char *argv[]) {
	return 1;
}

static int uptime(int argc, char *argv[]) {
	return 1;
}

struct mutex_test_arg {
	int semid;
	int cnt;
	bool fin;
};

static void mutex_test_task(void *_arg) {
	struct mutex_test_arg *arg = _arg;
	char msg[256];

	while (!arg->fin) {
		snprintf(msg, sizeof(msg), "%s: %d\n", __func__, os_task_id());
		os_sys_write(msg);

		os_sem_use(arg->semid, -1);

		if (arg->cnt) {
			os_sys_write("multiple process in critical section!");
			os_halt(1);
		}

		++arg->cnt;
		os_wait();
		--arg->cnt;

		os_sem_use(arg->semid, +1);
	}

	/*os_exit();*/
}

static int mutex_test(int argc, char *argv[]) {
	struct mutex_test_arg arg;

	arg.semid = os_sem_init(1);
	arg.cnt = 0;
	arg.fin = false;

	// FIXME use user-space app creation
	sched_add(mutex_test_task, &arg);
	sched_add(mutex_test_task, &arg);

	while (!arg.fin) {
		os_wait();
	}

	return 0;
}

static const struct {
	const char *name;
	int(*fn)(int, char *[]);
} app_list[] = {
	{ "echo", echo },
	{ "sleep", sleep },
	{ "uptime", uptime },
	{ "mutex_test", mutex_test },
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

static int do_task(char *command) {
	char *saveptr;
	char *arg = strtok_r(command, " ", &saveptr);
	char *argv[256];
	int argc = 0;
	while (arg) {
		argv[argc++] = arg;
		arg = strtok_r(NULL, " ", &saveptr);
	}

	for (int i = 0; i < ARRAY_SIZE(app_list); ++i) {
		if (!strcmp(argv[0], app_list[i].name)) {
			/* TODO run as sched task */
			return app_list[i].fn(argc, argv);
			/* TODO exit */
			/* TODO waitpid? */
		}
	}

	char msg[256] = "No such function: ";
	strcat(msg, argv[0]);
	strcat(msg, "\n");
	os_sys_write(msg);
	return 1;
}

void shell(void *args) {
	while (1) {
		os_sys_write("> ");
		char buffer[256];
		int bytes = os_sys_read(buffer, sizeof(buffer));
		if (!bytes) {
			break;
		}

		if (bytes < sizeof(buffer)) {
			buffer[bytes] = '\0';
		}

		char *saveptr;
		const char *comsep = "\n;";
		char *cmd = strtok_r(buffer, comsep, &saveptr);
		while (cmd) {
			do_task(cmd);
			cmd = strtok_r(NULL, comsep, &saveptr);
		}
	}

	os_sys_write("\n");
	os_halt(0);
}
