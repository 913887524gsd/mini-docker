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

sysctl -w net.ipv4.ip_forward=1

iptables -t filter -A FORWARD -j ACCEPT -o ${BR_DEV}
iptables -t filter -A FORWARD -j ACCEPT -i ${BR_DEV} ! -o ${BR_DEV}
iptables -t filter -A FORWARD -j ACCEPT -i ${BR_DEV} -o ${BR_DEV}

iptables -t nat -N ${CHAIN}
iptables -t nat -A ${CHAIN} -j RETURN -i ${BR_DEV}
iptables -t nat -A PREROUTING -j ${CHAIN} -m addrtype --dst-type LOCAL
iptables -t nat -A OUTPUT -j ${CHAIN} ! -d localhost/8 -m addrtype --dst-type LOCAL

iptables -t nat -A POSTROUTING -s ${BR_ADDR}/24 ! -o ${BR_DEV} -j MASQUERADE

mkdir -p ${ROOT_DIR}
cp resolv.conf ${ROOT_DIR}

if [ ! -d "${IMAGE_DIR}" ]; then
    mkdir -p ${IMAGE_DIR}
    tars=`ls *.tar.gz`
    for tar in ${tars}; do
        tar xzf ${tar} --directory=${IMAGE_DIR}
    done
fi
