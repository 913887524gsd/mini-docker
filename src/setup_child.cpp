#include <config.h>
#include <typedef.h>
#include <setup.h>

#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

static int child_func(void *args)
{
    setup_fs((const char *)args);
    execl("/bin/bash", NULL);
    errexit(-1);
    return 0;
}

pid_t setup_child(const char *rootdir)
{
    char *child_stack = (char *)mmap(NULL, 4096 * 4, PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (child_stack == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    int pid = clone(child_func, child_stack + 4096 * 4,  
        CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWIPC | CLONE_NEWUTS | CLONE_NEWCGROUP | SIGCHLD, 
        (void *)rootdir, NULL, NULL, NULL);
    errexit(pid);
    return pid;
}