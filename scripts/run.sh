#!/bin/bash -x
. "$(dirname ${BASH_SOURCE[0]})/lib"

MAC=$(< /sys/class/net/${TAP}/address)

qemu-img create -F qcow2 -f qcow2 -b ${BASE_IMAGE} disk0.qcow2

if [ -n ${EXTRA_CMDLINE} ]
then
  EXTRA_CMDLINE="nosmap nosmep"
fi

${QEMU} \
  -accel kvm \
  -smp 14 -m $((1024*32)) \
  -s \
  -cpu Skylake-Client \
  -kernel ${KERNEL} \
  -initrd ${DEVENV_ROOT}/images/build/initrd.img \
  -append "console=ttyS0,115200 loglevel=7 nokaslr root=/dev/vda rw selinux=0 transparent_hugepage=never ${EXTRA_CMDLINE}" \
  -serial mon:stdio \
  -display none \
  -drive id=disk0,file=disk0.qcow2,if=virtio,format=qcow2 \
  -machine q35 \
  -netdev tap,id=net0,ifname=${TAP},script=no,downscript=no \
  -device e1000,mac=${MAC},netdev=net0 \
  -virtfs local,path=$PWD/../,mount_tag=share,readonly=on,security_model=none \
  $@
