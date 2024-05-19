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

static void setup_chroot(const char *rootdir)
{
    errexit(chroot(rootdir));
    errexit(chdir("/"));
}

static void setup_mount_dev(void)
{
    mode_t old_umask = umask(0000);
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
    errexit(mknod("/dev/tty", S_IFCHR | 0666, makedev(5, 0)));
    errexit(mkdir("/dev/pts", 0755));
    errexit(mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC | MS_RELATIME, "gid=5,mode=620,ptmxmode=666"));
    errexit(symlink("/dev/pts/ptmx", "/dev/ptmx"));
    errexit(mkdir("/dev/shm", 0755));
    errexit(mount("tmpfs", "/dev/shm", "tmpfs", MS_NOSUID | MS_NODEV, "inode64"));
    errexit(mkdir("/dev/mqueue", 0755));
    errexit(mount("mqueue", "/dev/mqueue", "mqueue", MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RELATIME, NULL));
    errexit(mkdir("/dev/hugepages", 0755));
    errexit(mount("hugetblfs", "/dev/hugepages", "hugetlbfs", MS_RELATIME, "pagesize=2M"));
    umask(old_umask);
}

static void setup_mount(void)
{
    errexit(mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL));
    errexit(mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC, NULL));
    setup_mount_dev();
    errexit(mount("sysfs", "/sys", "sysfs", MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RELATIME, NULL));
}

static void setup_dns(void)
{
    FILE *fp = fopen("/etc/resolv.conf", "a");
    const char *dns = "\nnameserver 8.8.8.8\nnameserver 8.8.4.4\n";
    
    if (fp == NULL) {
        fprintf(stderr, "can not find dns file /etc/resolv.conf");
        return;
    }
    if (fwrite(dns, strlen(dns), 1, fp) != 1) {
        fprintf(stderr, "fwrite failed: %s", strerror(errno));
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
}

void setup_fs()
{
    setup_chroot(mergeddir);
    setup_mount();
    setup_dns();
}
