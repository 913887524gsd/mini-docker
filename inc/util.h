#pragma once

#include <fcntl.h>

#define UT_RMDIR_NOT_REMOVE_ROOT    0x00000001
#define UT_RMDIR_RESOLVE_LINK       0x00000002
int rmdir_recursive(const char *path, int flags);
int mkdir_recursive(const char *path, mode_t mode);