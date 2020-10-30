/*
 * Copyright (c) 2010 Kip Macy All rights reserved.
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Derived in part from libplebnet's pn_init.c.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/pcpu.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/sx.h>
#include <sys/vmmeter.h>
#include <sys/cpuset.h>
#include <sys/sysctl.h>
#include <sys/filedesc.h>

#include <vm/uma.h>
#include <vm/uma_int.h>
#include <vm/vm.h>
#include <vm/vm_extern.h>

#include "ff_host_interface.h"
#include "ff_api.h"

int ff_freebsd_init(void);

struct boot_default {
	char *field ;
	char *value ;
} boot_default [] = {
	{"kern.hz"                          ,    "100"}, 
	{"kern.ncallout"                    , "262144"},
	{"fd_reserve"                       ,   "2048"},
	{"kern.ipc.maxsockets"              , "262144"},
	{"net.inet.tcp.syncache.hashsize"   ,   "4096"},
	{"net.inet.tcp.syncache.bucketlimit",    "100"},
	{"net.inet.tcp.tcbhashsize"         ,  "65536"},
	{"kern.features.inet6"              ,      "1"},
	{"net.inet6.ip6.auto_linklocal"     ,      "1"},
	{"net.inet6.ip6.accept_rtadv"       ,      "2"},
	{"net.inet6.icmp6.rediraccept"      ,      "1"},
	{"net.inet6.ip6.forwarding"         ,      "0"},
} ;

struct sysctl_default {
	char *f ;
	char *v ;
	char  t ;
} sysctl_default [] = {
	{"kern.ipc.somaxconn"                 ,    "32768", 'i'},
	{"kern.ipc.maxsockbuf"                , "16777216", 'l'},
	{"net.link.ether.inet.maxhold"        ,        "5", 'i'},
	{"net.inet.tcp.fast_finwait2_recycle" ,        "1", 'i'},
	{"net.inet.tcp.sendspace"             ,    "32768", 'i'},
	{"net.inet.tcp.recvspace"             ,    "32768", 'i'},
//	{"net.inet.tcp.nolocaltimewait"       ,        "1", 'i'},
	{"net.inet.tcp.cc.algorithm"          ,    "cubic", 's'},
	{"net.inet.tcp.sendbuf_max"           , "16777216", 'i'},
	{"net.inet.tcp.recvbuf_max"           , "16777216", 'i'},
	{"net.inet.tcp.sendbuf_auto"          ,        "1", 'i'},
	{"net.inet.tcp.recvbuf_auto"          ,        "1", 'i'},
	{"net.inet.tcp.sendbuf_inc"           ,    "16384", 'i'},
	{"net.inet.tcp.recvbuf_inc"           ,   "524288", 'i'},
	{"net.inet.tcp.sack.enable"           ,        "1", 'i'},
	{"net.inet.tcp.blackhole"             ,        "1", 'i'},
	{"net.inet.tcp.msl"                   ,     "2000", 'i'},
	{"net.inet.tcp.delayed_ack"           ,        "0", 'i'},
	{"net.inet.udp.blackhole"             ,        "1", 'i'},
	{"net.inet.ip.redirect"               ,        "0", 'i'},
} ;
static int ff_freebsd_boot_init (void) ;
static int ff_freebsd_sysctl_init (void) ;
static int ff_freebsd_cpu_init(void) ;
static int ff_freebsd_memory_init(void) ;
static int ff_freebsd_mutex_init(void) ;
static int ff_freebsd_module_init(void) ;
static int ff_freebsd_filesystem_init(void) ;

int
ff_freebsd_init(void)
{
	ff_freebsd_boot_init() ;

	/*CPU*/
	ff_freebsd_cpu_init() ;

	/*Memory UMA*/
	ff_freebsd_memory_init() ;

	/*Mutex*/
	ff_freebsd_mutex_init() ;

	/*Module Startup*/
	ff_freebsd_module_init() ;

	/*File*/
	ff_freebsd_filesystem_init();

	ff_freebsd_sysctl_init() ;

    return (0);
}

int atoi(const char *nptr);
long atol(const char *nptr);

static int 
ff_freebsd_boot_init (void) {
	int i = 0 ;
	int error = 0 ;
	for (i = 0; i < sizeof(boot_default) / sizeof(struct boot_default); ++i) {
		error = kern_setenv(boot_default[i].field, boot_default[i].value); 
		if(error != 0) {
			printf("boot setenv %s = %s error\n", boot_default[i].field, boot_default[i].value);
		}
	}
	return 0 ;
}

static int 
ff_freebsd_sysctl_init (void) {
	int  i       = 0 ;
	int  error   = 0 ;
	int  value_i = 0 ;
	long value_l = 0 ;

	for (i = 0; i < sizeof(sysctl_default) / sizeof(struct sysctl_default); ++i) {
		if(sysctl_default[i].t == 'i') {
			value_i = atoi(sysctl_default[i].v);
			error = kernel_sysctlbyname(curthread, sysctl_default[i].f, NULL, NULL, (void *)&value_i, sizeof(int), NULL, 0);  
		} else if(sysctl_default[i].t == 'l') {
			value_l = atol(sysctl_default[i].v);
			error = kernel_sysctlbyname(curthread, sysctl_default[i].f, NULL, NULL, (void *)&value_l, sizeof(long), NULL, 0);  
		} else if(sysctl_default[i].t == 's') {
			error = kernel_sysctlbyname(curthread, sysctl_default[i].f, NULL, NULL, (void *)sysctl_default[i].v, strlen(sysctl_default[i].v), NULL, 0);  
		}

		if(error != 0) {
			printf("sysctl %s = %s error\n", sysctl_default[i].f, sysctl_default[i].v);
		}
	}

	return 0 ;
}

struct pcpu *pcpup;
extern cpuset_t all_cpus;

extern void ff_init_thread0(void);

static int 
ff_freebsd_cpu_init(void) {
	pcpup = malloc(sizeof(struct pcpu), M_DEVBUF, M_ZERO);
	pcpu_init(pcpup, 0, sizeof(struct pcpu));

	CPU_SET(0, &all_cpus);

	ff_init_thread0();

	return 0 ;
}

long physmem;
struct uma_page_head *uma_page_slab_hash;
int uma_page_mask;

extern void uma_startup(void *, int);
extern void uma_startup2(void);

static int 
ff_freebsd_memory_init(void) {
	int boot_pages;
	void *bootmem;
	unsigned int num_hash_buckets;

	physmem = 1048576 * 256 ;
	
	boot_pages = 16;
	bootmem = (void *)kmem_malloc(NULL, boot_pages * PAGE_SIZE, M_ZERO);
	uma_startup(bootmem, boot_pages);
	uma_startup2();

	num_hash_buckets = 8192;
	uma_page_slab_hash = (struct uma_page_head *)kmem_malloc(NULL, sizeof(struct uma_page)*num_hash_buckets, M_ZERO);
	uma_page_mask = num_hash_buckets - 1;

	return 0 ;
}

struct sx proctree_lock;
extern void mutex_init(void);

static int 
ff_freebsd_mutex_init (void) {
	mutex_init();
	sx_init(&proctree_lock, "proctree");
	return 0 ;
}

extern void mi_startup(void);

static int 
ff_freebsd_module_init(void) {
	mi_startup();
	return 0 ;
}

static int 
ff_freebsd_filesystem_init(void) {
	ff_fdused_range(1024);
	return 0 ;
}

