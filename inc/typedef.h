#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define errexit(ret) do {                                  \
        if ((ret) == -1) {                                 \
            fprintf(stderr, "%s:%d %s failed: %s\n",       \
                __FILE__, __LINE__, #ret, strerror(errno));\
            exit(EXIT_FAILURE);                            \
        }                                                  \
    } while (0)

