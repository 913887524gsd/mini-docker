#pragma once

#define ROOT_DIR "/etc/mini-docker"

#define IMAGE_DIR       ROOT_DIR "/images"
#define RUNTIME_DIR     ROOT_DIR "/runtime"
#define NET_DIR         ROOT_DIR "/net"
#define DNS_FILE        ROOT_DIR "/resolv.conf"

#define SUBNET_PREFIX       "192.168.233"
#define SUBNET_PREFIX_LEN   24

#define BRIDGE_DEV       "mini-docker0"
#define BRIDGE_ADDR      SUBNET_PREFIX ".1"

#define CGROUP_ROOT     "/sys/fs/cgroup"
#define CGROUP_CPU      CGROUP_ROOT "/cpu"
#define CGROUP_MEM      CGROUP_ROOT "/memory"
#define CGROUP_BLK      CGROUP_ROOT "/blkio"

#define __CGROUP_RUNTIME_DIR        "mini-docker.slice"
#define CGROUP_RUNTIME_DIR(TYPE)    CGROUP_##TYPE "/" __CGROUP_RUNTIME_DIR

#define IPTABLE_CHAIN   "MINI_DOCKER"