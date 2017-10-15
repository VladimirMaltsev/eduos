#ifndef EDUOS_OS_H
#define EDUOS_OS_H

extern int os_sys_write(const char *msg);

extern int os_sys_read(char *buffer, int size);

extern int os_halt(int status);

extern int os_sys_clone(void (*fn) (void *arg), void* arg);

extern int os_sys_waitpid(int task_id);

extern int os_sys_exit();

#endif /* EDUOS_OS_H */
