#include <config.h>
#include <typedef.h>
#include <setup.h>

#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

static FILE *IPC_read = NULL, *IPC_write = NULL;

int IPC_send(const void *buf, size_t length)
{
    while (fwrite(&length, sizeof(length), 1, IPC_write) != 1) {
        if (errno != EINTR)
            return -1;
    }
    while (fwrite(buf, length, 1, IPC_write) != 1) {
        if (errno != EINTR)
            return -1;
    }
    fflush(IPC_write);
    return 0;
}
int IPC_recv(const void **buf)
{
    void *__buf;
    size_t length;
    while (fread(&length, sizeof(length), 1, IPC_read) != 1) {
        if (errno != EINTR)
            return -1;
    }
    __buf = malloc(length); 
    while (fread(__buf, length, 1, IPC_read) != 1) {
        if (errno != EINTR) {
            free(__buf);
            return -1;
        }
    }
    *buf = __buf;
    return 0;
}

static int child_func(void *args)
{
    void **arglist = (void **)args;
    int *readfd = (int *)arglist[2];
    int *writefd = (int *)arglist[1];
    close(readfd[1]);
    IPC_read = fdopen(readfd[0], "r");
    if (IPC_read == NULL) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }
    close(writefd[0]);
    IPC_write = fdopen(writefd[1], "w");
    if (IPC_write == NULL) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }
    setup_fs((const char *)arglist[0]);
    free(readfd);
    free(writefd);
    free(arglist);
    container_setup_net();
    execl("/bin/bash", NULL);
    errexit(-1);
    return 0;
}

void forcekill_child(int state, void *pid)
{
    if (state != 0) {
        kill((pid_t)(size_t)pid, SIGKILL);
    }
}

pid_t setup_child(const char *rootdir)
{
    char *child_stack = (char *)mmap(NULL, 4096 * 4, PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (child_stack == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    int *readfd = (int *)malloc(2 * sizeof(int));
    int *writefd = (int *)malloc(2 * sizeof(int));
    pipe2(readfd, O_CLOEXEC);
    pipe2(writefd, O_CLOEXEC);
    void **arglist = (void **)malloc(3 * sizeof(void *));
    arglist[0] = (void *)rootdir;
    arglist[1] = (void *)readfd;
    arglist[2] = (void *)writefd;
    int pid = clone(child_func, child_stack + 4096 * 4,  
        CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWIPC | CLONE_NEWUTS | CLONE_NEWCGROUP | SIGCHLD, 
        (void *)arglist, NULL, NULL, NULL);
    errexit(pid);
    close(readfd[1]);
    IPC_read = fdopen(readfd[0], "r");
    if (IPC_read == NULL) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }
    close(writefd[0]);
    IPC_write = fdopen(writefd[1], "w");
    if (IPC_write == NULL) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }
    errexit(on_exit(forcekill_child, (void *)(size_t)pid));
    free(readfd);
    free(writefd);
    free(arglist);
    return pid;
}