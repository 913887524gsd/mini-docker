#include <config.h>
#include <setup.h>
#include <typedef.h>

#include <bits/stdc++.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <unistd.h>


static void setup_dev(void)
{
    errexit(mount("tmpfs", "/dev", "tmpfs", MS_NOSUID | MS_NOEXEC, "size=65536k,mode=755,inode64"));
    errexit(mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3)));
    errexit(mknod("/dev/zero", S_IFCHR | 0666, makedev(1, 5)));
    errexit(mknod("/dev/full", S_IFCHR | 0666, makedev(1, 7)));
    errexit(mknod("/dev/random", S_IFCHR | 0666, makedev(1, 8)));
    errexit(mknod("/dev/urandom", S_IFCHR | 0666, makedev(1, 9)));
    errexit(symlink("/proc/self/fd/0", "/dev/stdin"));
    errexit(symlink("/proc/self/fd/1", "/dev/stdout"));
    errexit(symlink("/proc/self/fd/2", "/dev/stderr"));
    errexit(symlink("/proc/self/fd", "/dev/fd"));
    errexit(mkdir("/dev/pts", 0755));
    errexit(mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC | MS_RELATIME, "gid=5,mode=620,ptmxmode=666"));
    errexit(symlink("/dev/pts/ptmx", "/dev/ptmx"));
}

static void setup_mount(void)
{
    errexit(mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL));
    errexit(mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC, NULL));
    setup_dev();
    errexit(mount("sysfs", "/sys", "sysfs", MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RELATIME, NULL));
}

static void setup_chroot(const char *rootdir)
{
    errexit(chroot(rootdir));
    errexit(chdir("/"));
}

void setup_fs(const char *rootdir)
{
    setup_chroot(rootdir);
    setup_mount();
}
