
#include <string.h>
#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include "os.h"

static char *directory;

void filesys_init(char *path) {
    directory = path;
}

void get_absolute_path(char *relative_path, char *path) {
    strcpy(path, directory);
    strcat(path, relative_path);
}