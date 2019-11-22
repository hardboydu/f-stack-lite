#!/usr/bin/env bash

make

mkdir -p /opt/lib64/f-stack/lib
mkdir -p /opt/lib64/f-stack/include
cp lib/libfstack.a /opt/lib64/f-stack/lib
cp lib/ff_api.h lib/ff_errno.h lib/ff_event.h lib/ff_veth.h lib/ff_mbuf.h lib/ff_epoll.h ff_dpdk/ff_dpdk.h /opt/lib64/f-stack/include

