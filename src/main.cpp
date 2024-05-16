#include <config.h>
#include <typedef.h>
#include <setup.h>
#include <util.h>

#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

static void init_runtime_enviroment(void)
{
    errexit(mkdir_recursive(IMAGE_DIR, 0755));
    errexit(mkdir_recursive(RUNTIME_DIR, 0755));
    errexit(rmdir_recursive(RUNTIME_DIR, UT_RMDIR_NOT_REMOVE_ROOT));
}

int main(int argc, char *argv[])
{
    init_runtime_enviroment();
    const char *imagedir = IMAGE_DIR "/centos";
    char *rootdir = NULL;
    setup_image(imagedir, &rootdir);
    pid_t pid = setup_child(mergeddir);
    if (waitpid(pid, NULL, 0) != pid) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
}