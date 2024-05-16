#include <util.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

int mkdir_recursive(const char *path, mode_t mode)
{
    char *subpath, *fullpath;
    int result = 0;

    fullpath = strdup(path);
    if (fullpath == NULL) {
        perror("strdup");
        return -1;
    }

    for (subpath = fullpath; *subpath; ++subpath) {
        if (*subpath == '/') {
            *subpath = '\0';
            if (strlen(fullpath) > 0 && mkdir(fullpath, mode) == -1) {
                if (errno != EEXIST) {
                    perror("mkdir");
                    result = -1;
                    break;
                }
            }
            *subpath = '/';
        }
    }

    if (result == 0 && mkdir(fullpath, mode) == -1) {
        if (errno != EEXIST) {
            perror("mkdir");
            result = -1;
        }
    }

    free(fullpath);
    return result;
}

int rmdir_recursive(const char *path, int flags)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;

        r = 0;
        while (!r && (p=readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2; 
            buf = (char *)malloc(len);

            if (buf) {
                int deleted = 0;
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                r2 = lstat(buf, &statbuf);
                if (!r2 && S_ISLNK(statbuf.st_mode)) {
                    if (!(flags & UT_RMDIR_RESOLVE_LINK)) {
                        r2 = unlink(buf);
                        deleted = 1;
                    } else {
                        r2 = stat(buf, &statbuf);
                    }
                } 
                if (!r2 && !deleted) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        r2 = rmdir_recursive(buf, (flags & ~UT_RMDIR_NOT_REMOVE_ROOT));
                    } else {
                        r2 = unlink(buf);
                    }
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r && !(flags & UT_RMDIR_NOT_REMOVE_ROOT))
        r = rmdir(path);

    return r;
}