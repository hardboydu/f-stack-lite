/*
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
 */

#include <stdlib.h>

#include "ff_api.h"
#include "ff_veth.h"

extern int ff_freebsd_init();

struct ff_veth_conf ff_veth_conf_default = {
	.name        = "veth%d",
	.mac         = "AA:BB:CC:DD:EE:FF",
	.ip          = "193.169.9.10" ,
	.netmask     = "255.255.255.0",
	.broadcast   = "193.169.9.255",
	.gateway     = "193.169.9.1",
	.hw_features = {
		.rx_csum    = 0 ,
		.rx_lro     = 0 ,
		.tx_csum_ip = 0 , 
		.tx_csum_l4 = 0 ,
		.tx_tso     = 0
	}
};

struct ff_veth_softc *
ff_init(struct ff_veth_conf *conf, void *host_ctx)
{
    int ret = 0 ;
    struct ff_veth_softc *sc ;

    ret = ff_freebsd_init();
    if (ret < 0) {
    	return NULL ;
    }

	if(conf == NULL) {
		sc = ff_veth_attach(&ff_veth_conf_default, host_ctx) ;
    } else {
		sc = ff_veth_attach(conf, host_ctx) ;
    }

    return sc ;
}


int
ff_init_orig(int argc, char * const argv[])
{
    int ret = 0 ;
    struct ff_veth_softc *sc ;

    ret = ff_freebsd_init();
    if (ret < 0)
    	exit(1);

    sc = ff_veth_attach(&ff_veth_conf_default, NULL) ;
    if(sc == NULL) 
    	exit(2);

    return 0;
}

void
ff_run(loop_func_t loop, void *arg)
{
	while(1) {
		loop(arg) ;
	}
}

