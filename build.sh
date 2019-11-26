#!/usr/bin/env bash

function version_gt() { test "$(echo "$@" | tr " " "\n" | sort -V  | head -n 1)" != "$1"; }
function version_le() { test "$(echo "$@" | tr " " "\n" | sort -V  | head -n 1)" == "$1"; }
function version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }
function version_ge() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" == "$1"; }

CC_VERSION=`/usr/bin/gcc -dumpversion`

if version_lt $CC_VERSION "4.8.0" ; then
	make CC=/opt/devrte/runtime/gcc-7.3.0/bin/gcc
else
	make
fi

mkdir -p /opt/lib64/f-stack/lib
mkdir -p /opt/lib64/f-stack/include
cp lib/libfstack.a /opt/lib64/f-stack/lib
cp lib/ff_api.h lib/ff_errno.h lib/ff_event.h lib/ff_veth.h lib/ff_mbuf.h lib/ff_epoll.h ff_dpdk/ff_dpdk.h /opt/lib64/f-stack/include

