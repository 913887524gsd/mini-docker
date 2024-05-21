#include <config.h>
#include <typedef.h>
#include <setup.h>

#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <mntent.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

static char *get_runtimedir(const char *rootdir)
{
    char *dir = NULL;
    errexit(asprintf(&dir, "%s/%s", rootdir, runtimeID));
    return dir;
}

static void remove_cgroup_runtime(void)
{
    char *dir;
    dir = get_runtimedir(CGROUP_RUNTIME_DIR(CPU));
    errexit(rmdir(dir));
    free(dir);
    dir = get_runtimedir(CGROUP_RUNTIME_DIR(MEM));
    errexit(rmdir(dir));
    free(dir);
    dir = get_runtimedir(CGROUP_RUNTIME_DIR(BLK));
    errexit(rmdir(dir));
    free(dir);
}

static void setup_cgroup_runtime(void)
{
    char *dir;
    dir = get_runtimedir(CGROUP_RUNTIME_DIR(CPU));
    errexit(mkdir(dir, 0755));
    free(dir);
    dir = get_runtimedir(CGROUP_RUNTIME_DIR(MEM));
    errexit(mkdir(dir, 0755));
    free(dir);
    dir = get_runtimedir(CGROUP_RUNTIME_DIR(BLK));
    errexit(mkdir(dir, 0755));
    free(dir);
    errexit(atexit(remove_cgroup_runtime));
}

static void set_cgroup_cpu(pid_t pid)
{
    if (cpu_limit == -1)
        return;
    char *dir = get_runtimedir(CGROUP_RUNTIME_DIR(CPU));
    char *period_file = NULL;
    char *quota_file = NULL;
    char *tasks_file = NULL;
    FILE *fp;
    unsigned int period, quota;

    errexit(asprintf(&period_file, "%s/cpu.cfs_period_us", dir));
    errexit(asprintf(&quota_file, "%s/cpu.cfs_quota_us", dir));
    errexit(asprintf(&tasks_file, "%s/tasks", dir));
    if ((fp = fopen(period_file, "r")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fscanf(fp, "%u", &period));
    errexit(fclose(fp));
    quota = period * cpu_limit;
    if ((fp = fopen(quota_file, "w")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fprintf(fp, "%u\n", quota));
    errexit(fclose(fp));
    if ((fp = fopen(tasks_file, "w")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fprintf(fp, "%d\n", pid));
    errexit(fclose(fp));

    free(dir);
    free(period_file);
    free(quota_file);
    free(tasks_file);
}

static void set_cgroup_memory(pid_t pid)
{
    if (memory_limit == 0)
        return;
    char *dir = get_runtimedir(CGROUP_RUNTIME_DIR(MEM));
    char *limit_file = NULL;
    char *tasks_file = NULL;
    FILE *fp;

    errexit(asprintf(&limit_file, "%s/memory.limit_in_bytes", dir));
    errexit(asprintf(&tasks_file, "%s/tasks", dir));
    if ((fp = fopen(limit_file, "w")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fprintf(fp, "%zu\n", memory_limit));
    errexit(fclose(fp));
    if ((fp = fopen(tasks_file, "w")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fprintf(fp, "%d\n", pid));
    errexit(fclose(fp));

    free(dir);
    free(limit_file);
    free(tasks_file);
}

static char *get_root_dev(void)
{
    char *root_dev = NULL;
    struct stat buf;

    errexit(stat("/", &buf));
    errexit(asprintf(&root_dev, "%d:%d", major(buf.st_dev), 0));
    
    return root_dev;
}

static void set_cgroup_blk(pid_t pid)
{
    if (blkio_limit == 0)
        return;
    char *root_dev = get_root_dev();
    char *dir = get_runtimedir(CGROUP_RUNTIME_DIR(BLK));
    char *limit_read_file = NULL;
    char *limit_write_file = NULL;
    char *tasks_file = NULL;
    FILE *fp;

    errexit(asprintf(&limit_read_file, "%s/blkio.throttle.read_bps_device", dir));
    errexit(asprintf(&limit_write_file, "%s/blkio.throttle.write_bps_device", dir));
    errexit(asprintf(&tasks_file, "%s/tasks", dir));
    if ((fp = fopen(limit_read_file, "w")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fprintf(fp, "%s %zu\n", root_dev, blkio_limit));
    errexit(fclose(fp));
    if ((fp = fopen(limit_write_file, "w")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fprintf(fp, "%s %zu\n", root_dev, blkio_limit));
    errexit(fclose(fp));
    if ((fp = fopen(tasks_file, "w")) == NULL) {
        fprintf(stderr, "fopen failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errexit(fprintf(fp, "%d\n", pid));
    errexit(fclose(fp));

    free(root_dev);
    free(dir);
    free(limit_read_file);
    free(limit_write_file);
    free(tasks_file);
}

void setup_cgroup(pid_t pid)
{
    setup_cgroup_runtime();
    set_cgroup_cpu(pid);
    set_cgroup_memory(pid);
    set_cgroup_blk(pid);
}