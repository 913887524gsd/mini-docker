#pragma once

#include <stdio.h>

int IPC_send(const void *buf, size_t length);
int IPC_recv(const void **buf);