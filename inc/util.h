#pragma once

#include <fcntl.h>
#include <signal.h>

void atexit_do_command(int, void *command);

int pidfd_open(pid_t pid, unsigned int flags);
int pidfd_send_signal(int pidfd, int sig, siginfo_t *info, unsigned int flags);

#define UT_RMDIR_NOT_REMOVE_ROOT    0x00000001
#define UT_RMDIR_RESOLVE_LINK       0x00000002
int rmdir_recursive(const char *path, int flags);
int mkdir_recursive(const char *path, mode_t mode);