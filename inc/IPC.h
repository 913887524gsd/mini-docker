#pragma once

#include <stdio.h>

int IPC_send(const void *buf, size_t length);
int IPC_recv(const void **buf);
void IPC_free(void);

#define IPC_send_sync() do {                    \
    int __sync = 1;                             \
    errexit(IPC_send(&__sync, sizeof(__sync))); \
} while (0)

#define IPC_recv_sync() do {    \
    const void *__sync;         \
    errexit(IPC_recv(&__sync)); \
    free((void *)__sync);       \
} while (0)
