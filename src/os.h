#ifndef EDUOS_OS_H
#define EDUOS_OS_H

extern int os_sys_write(int fd, const char *msg);

extern int os_sys_read(int fd, char *buffer, int size);

extern int os_clone(void (*fn) (void *arg), void *arg);

extern int os_waitpid(int task_id);

extern int os_halt(int status);

extern int os_exit(int status);

extern int os_wait(void);

extern int os_get_file_descr(const char *path, int flags);

extern int os_fclose_by_descr(int fd);

extern int os_task_id(void);

extern int os_sem_init(int cnt);

extern int os_sem_use(int semid, int add);

extern int os_sem_free(int semid);

extern int os_sleep(int seconds);

extern int os_uptime(void);

#endif /* EDUOS_OS_H */
