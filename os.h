#include <stddef.h>

#ifndef EDUOS_OS_H
#define EDUOS_OS_H

extern int os_sys_write(const char *msg);
extern int os_sys_read(char *buffer, size_t size);

#endif /* EDUOS_OS_H */
