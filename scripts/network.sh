#!/bin/bash -eux
. "$(dirname ${BASH_SOURCE[0]})/lib"

_ifup () {
    ip link set dev $1 up
}

setup () {
    ip link add ${BR} type bridge
    _ifup ${BR}
    ip addr add 192.168.232.1/24 dev ${BR}

    ip tuntap add dev ${TAP} mode tap user ${br_owner}
    _ifup ${TAP}
    ip link set dev ${TAP} master ${BR}
}

teardown () {
    for IF in ${TAP} ${BR}
    do
        ip link set dev ${IF} down
        ip link del dev ${IF}
    done
}

if [ -n "${SUDO_USER:-}" ]
then
    br_owner=$SUDO_USER
else
    br_owner=${USER:-root}
fi

case "$1" in
    setup)
        setup
        ;;
    teardown)
        teardown
        ;;
    *)
        echo "$0 setup|teardown"
        exit 1
        ;;
esac
