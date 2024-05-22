#include <config.h>
#include <setup.h>
#include <typedef.h>
#include <util.h>

#include <libgen.h>
#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>

char *runtimedir = NULL;
char *mergeddir = NULL;

char *runtimeID = NULL;

static void remove_runtime_directory(void)
{
    errexit(rmdir_recursive(runtimedir, 0));
    free(runtimedir);
    runtimeID = NULL;
}

static void generate_runtime_directory(void)
{
    errexit(asprintf(&runtimedir, "%s/XXXXXX", RUNTIME_DIR));
    runtimedir = mkdtemp(runtimedir);
    fprintf(stderr, "runtime dir: %s\n", runtimedir);
    if (runtimedir == NULL) {
        fprintf(stderr, "mkdtemp failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    runtimeID = runtimedir + strlen(runtimedir) - 6;
    errexit(atexit(remove_runtime_directory));
}

static void remove_overlay(void)
{
    errexit(umount(mergeddir));
    free(mergeddir);
}

static void setup_overlay(void)
{
    char *lowerdir = NULL, *upperdir = NULL, *workdir = NULL;
    char *data = NULL;
    errexit(asprintf(&lowerdir, "%s/lower", runtimedir));
    errexit(symlink(imagedir, lowerdir));
    errexit(asprintf(&upperdir, "%s/upper", runtimedir));
    errexit(mkdir(upperdir, 0755));
    errexit(asprintf(&workdir, "%s/work", runtimedir));
    errexit(mkdir(workdir, 0755));
    errexit(asprintf(&mergeddir, "%s/merged", runtimedir));
    errexit(mkdir(mergeddir, 0755));
    errexit(asprintf(&data, "lowerdir=%s,upperdir=%s,workdir=%s", lowerdir, upperdir, workdir));
    errexit(mount("overlay", mergeddir, "overlay", 0, data));
    errexit(atexit(remove_overlay));
    free(lowerdir);
    free(upperdir);
    free(workdir);
    free(data);
}

static void remove_volumn(int, void *container_path)
{
    errexit(umount((const char *)container_path));
    free(container_path);
}

static void setup_volumn(void)
{
    for (auto &[h,c] : volumns) {
        const char *host_path = h.c_str();
        char *container_path = NULL;
        char *dir_path = NULL;
        struct stat host_stat;
        errexit(asprintf(&container_path, "%s/%s", mergeddir, c.c_str()));
        errexit(access(host_path, F_OK));
        dir_path = dirname(strdup(container_path));
        if (strlen(dir_path) <= strlen(mergeddir)) {
            fprintf(stderr, "invalid container path: %s\n", c.c_str());
            exit(EXIT_FAILURE);
        }
        errexit(mkdir_recursive(dir_path, 0755));
        errexit(stat(host_path, &host_stat));
        if (S_ISDIR(host_stat.st_mode)) {
            errexit(mkdir(container_path, 0755));
        } else {
            int fd;
            errexit(fd = creat(container_path, 0644));
            close(fd);
        }
        errexit(mount(host_path, container_path, NULL, MS_BIND, NULL));
        errexit(on_exit(remove_volumn, container_path));
        free(dir_path);
    }
}

void setup_image(void)
{
    errexit(asprintf(&imagedir, "%s/%s", IMAGE_DIR, image.c_str()));
    generate_runtime_directory();
    setup_overlay();
    setup_volumn();
}