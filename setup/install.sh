#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "You must be root to run this script"
    exit 1
fi

cd `dirname $0`

source ./config.sh

ip link add ${BR_DEV} type bridge
ip link set ${BR_DEV} up
ip addr add ${BR_ADDR}/24 dev ${BR_DEV}

iptables -t nat -N ${CHAIN}
iptables -t nat -A ${CHAIN} -j RETURN -i ${BR_DEV}
iptables -t nat -A PREROUTING -j ${CHAIN} -m addrtype --dst-type LOCAL
iptables -t nat -A OUTPUT -j ${CHAIN} ! -d localhost/8 -m addrtype --dst-type LOCAL

iptables -t nat -A POSTROUTING -s ${BR_ADDR}/24 ! -o ${BR_DEV} -j MASQUERADE

mkdir -p ${ROOT_DIR}
cp resolv.conf ${ROOT_DIR}

if [ ! -d "${IMAGE_DIR}" ]; then
    mkdir -p ${IMAGE_DIR}
    tar xzf centos.tar.gz --directory=${IMAGE_DIR}
    tar xzf test.tar.gz --directory=${IMAGE_DIR}
fi