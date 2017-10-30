
#include <stddef.h>

#include "os.h"
#include "os/syscall.h"
#include "os/irq.h"
#include "os/sched.h"
#include "os/time.h"
#include "os/filesys.h"

#include "apps.h"

int main(int argc, char *argv[]) {
	
	irq_init();

	syscall_init();

	time_init();

	sched_init();

	argc > 1 ? filesys_init(argv[1]) : filesys_init("");

	sched_add(shell, NULL);

	sched_loop();

	return 0;
}
