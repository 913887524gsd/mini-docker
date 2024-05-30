#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "You must be root to run this script"
    exit 1
fi

cd `dirname $0`

source ./config.sh

if [[ ! -d ${ROOT_DIR} ]]; then
    echo "mini-docker has been uninstalled"
    exit 0
fi

if [[ -d ${RUNTIME_DIR} ]]; then
    containers=`ls ${RUNTIME_DIR}`
    if [[ -n ${containers} ]]; then
        echo "There are still running containers: ${containers}"
        exit 1
    fi
fi

iptables -t filter -D FORWARD -j ACCEPT -o ${BR_DEV}
iptables -t filter -D FORWARD -j ACCEPT -i ${BR_DEV} ! -o ${BR_DEV}
iptables -t filter -D FORWARD -j ACCEPT -i ${BR_DEV} -o ${BR_DEV}

iptables -t nat -D OUTPUT -j ${CHAIN} ! -d localhost/8 -m addrtype --dst-type LOCAL
iptables -t nat -D PREROUTING -j ${CHAIN} -m addrtype --dst-type LOCAL
iptables -t nat -F ${CHAIN}
iptables -t nat -X ${CHAIN}

iptables -t nat -D POSTROUTING -s ${BR_ADDR}/24 ! -o ${BR_DEV} -j MASQUERADE

ip link delete ${BR_DEV}

rm -r ${ROOT_DIR}

rmdir /sys/fs/cgroup/cpu/mini-docker.slice
rmdir /sys/fs/cgroup/memory/mini-docker.slice
rmdir /sys/fs/cgroup/blkio/mini-docker.slice
