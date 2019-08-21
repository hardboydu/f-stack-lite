#!/bin/bash

modprobe uio
insmod /opt/lib64/dpdk-18.11.1/lib/modules/3.10.0-957.el7.x86_64/extra/dpdk/igb_uio.ko

ifconfig enp2s0 down 
OLDPATH=`pwd`
cd /opt/lib64/dpdk-18.11.1/usr/local/sbin/
./dpdk-devbind --bind igb_uio enp2s0
cd $OLDPATH

mkdir -p /mnt/huge
(mount | grep /mnt/huge) > /dev/null || mount -t hugetlbfs hugetlbfs /mnt/huge
for i in {0..7}
do
	if [[ -e "/sys/devices/system/node/node$i" ]]
	then
		echo 512 > /sys/devices/system/node/node$i/hugepages/hugepages-2048kB/nr_hugepages
	fi
done

