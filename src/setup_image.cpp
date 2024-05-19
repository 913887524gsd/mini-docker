#include <config.h>
#include <setup.h>
#include <typedef.h>
#include <util.h>

#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>

char *runtimedir;
char *mergeddir;

static void recycle_image(void)
{
    errexit(umount(mergeddir));
    errexit(rmdir_recursive(runtimedir, 0));
    free(mergeddir);
    free(runtimedir);
}

static void check_image(void)
{
    imagedir = NULL;
    errexit(asprintf(&imagedir, "%s/%s", IMAGE_DIR, image.c_str()));
    errexit(access(imagedir, F_OK));
}

static void generate_runtime_directory(void)
{
    runtimedir = NULL;
    errexit(asprintf(&runtimedir, "%s/XXXXXX", RUNTIME_DIR));
    runtimedir = mkdtemp(runtimedir);
    fprintf(stderr, "runtime dir: %s\n", runtimedir);
    if (runtimedir == NULL) {
        fprintf(stderr, "mkdtemp failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void setup_image()
{
    char *lowerdir = NULL, *upperdir = NULL, *workdir = NULL;
    char *data = NULL;

    check_image();
    generate_runtime_directory();
    errexit(asprintf(&lowerdir, "%s/lower", runtimedir));
    errexit(symlink(imagedir, lowerdir));
    errexit(asprintf(&upperdir, "%s/upper", runtimedir));
    errexit(mkdir(upperdir, 0755));
    errexit(asprintf(&workdir, "%s/worker", runtimedir));
    errexit(mkdir(workdir, 0755));
    errexit(asprintf(&mergeddir, "%s/merged", runtimedir));
    errexit(mkdir(mergeddir, 0755));
    errexit(asprintf(&data, "lowerdir=%s,upperdir=%s,workdir=%s", lowerdir, upperdir, workdir));
    errexit(mount("overlay", mergeddir, "overlay", 0, data));
    errexit(atexit(recycle_image));
    free(lowerdir);
    free(upperdir);
    free(workdir);
    free(data);
}