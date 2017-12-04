
#ifndef EDUOS_OS_FILESYS_H
#define EDUOS_OS_FILESYS_H

extern void filesys_init(char *path);

extern void get_absolute_path(char *relative_path, char *path);

#endif /* EDUOS_OS_FILESYS_H */