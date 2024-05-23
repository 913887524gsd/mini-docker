#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "You must be root to run this script"
    exit 1
fi

cd `dirname $0`

source ./config.sh

containers=`ls ${RUNTIME_DIR}`
if [[ -n${containers} ]]; then
    echo "There are still running containers: ${containers}"
    exit 1
fi

ip link delete ${BR_DEV}

iptables -t nat -D OUTPUT -j ${CHAIN} ! -d localhost/8 -m addrtype --dst-type LOCAL
iptables -t nat -D PREROUTING -j ${CHAIN} -m addrtype --dst-type LOCAL
iptables -t nat -F ${CHAIN}
iptables -t nat -X ${CHAIN}

iptables -t nat -D POSTROUTING -s ${BR_ADDR}/24 ! -o ${BR_DEV} -j MASQUERADE

rm -r ${ROOT_DIR}