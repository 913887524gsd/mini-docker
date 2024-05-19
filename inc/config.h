#pragma once

#define ROOT_DIR "/etc/mini-docker"

#define IMAGE_DIR       ROOT_DIR "/images"
#define RUNTIME_DIR     ROOT_DIR "/runtime"
#define NET_DIR         ROOT_DIR "/net"

#define SUBNET_PREFIX       "192.168.233"
#define SUBNET_PREFIX_LEN   24

#define NETBRIDGE_DEV       "mini-docker0"
#define NETBRIDGE_ADDR      SUBNET_PREFIX ".1"