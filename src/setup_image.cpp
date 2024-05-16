#include <config.h>
#include <setup.h>
#include <typedef.h>

#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>

char *mergeddir;

static void recycle_image(void)
{
    errexit(umount(mergeddir));
}

void setup_image(const char *imagedir, char **rootdir)
{
    *rootdir = NULL;
    errexit(asprintf(rootdir, "%s/XXXXXX", RUNTIME_DIR));
    *rootdir = mkdtemp(*rootdir);
    errexit(fprintf(stderr, "runtime dir: %s\n", *rootdir));
    if (*rootdir == NULL) {
        perror("mkdtemp");
        exit(EXIT_FAILURE);
    }
    char *lowerdir = NULL;
    char *upperdir = NULL;
    char *workerdir = NULL;
    char *data = NULL;
    errexit(asprintf(&lowerdir, "%s/lower", *rootdir));
    errexit(symlink(imagedir, lowerdir));
    errexit(asprintf(&upperdir, "%s/upper", *rootdir));
    errexit(mkdir(upperdir, 0755));
    errexit(asprintf(&workerdir, "%s/worker", *rootdir));
    errexit(mkdir(workerdir, 0755));
    errexit(asprintf(&mergeddir, "%s/merged", *rootdir));
    errexit(mkdir(mergeddir, 0755));
    errexit(asprintf(&data, "lowerdir=%s,upperdir=%s,workdir=%s", lowerdir, upperdir, workerdir));
    errexit(mount("overlay", mergeddir, "overlay", 0, data));
    errexit(atexit(recycle_image));
    free(lowerdir);
    free(upperdir);
    free(workerdir);
    free(data);
}