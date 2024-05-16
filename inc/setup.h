#pragma once

#include <unistd.h>

extern char *mergeddir;

void setup_fs(const char *rootdir);
void setup_image(const char *imagedir, char **rootdir);
pid_t setup_child(const char *rootdir);