#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "You must be root to run this script"
    exit 1
fi

cd `dirname $0`

BR_DEV="mini-docker0"
BR_ADDR="192.168.233.1"

ip link add ${BR_DEV} type bridge
ip link set ${BR_DEV} up
ip addr add ${BR_ADDR}/24 dev ${BR_DEV}
iptables -t nat -A POSTROUTING -s ${BR_ADDR}/24 ! -o ${BR_DEV} -j MASQUERADE

ROOT_DIR="/etc/mini-docker"
IMAGE_DIR="${ROOT_DIR}/images"

mkdir -p ${ROOT_DIR}

rm -rf ${IMAGE_DIR}
mkdir -p ${IMAGE_DIR}
tar xzf centos.tar.gz --directory=${IMAGE_DIR}