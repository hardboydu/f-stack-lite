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

#ifndef _FSTACK_VETH_H
#define _FSTACK_VETH_H

struct ff_veth_hw_features {
    unsigned char rx_csum;
    unsigned char rx_lro;
    unsigned char tx_csum_ip;
    unsigned char tx_csum_l4;
    unsigned char tx_tso;
};
	
struct ff_veth_softc {int unused ;} ;
struct mbuf ;
typedef int (*ff_sc_transmit)(struct ff_veth_softc *sc, void *host_ctx, struct mbuf *m) ;

struct ff_veth_conf {
	char    *name;
	char    *mac ;
    char    *ip;
    char    *netmask;
    char    *broadcast;
    char    *gateway;

    struct   ff_veth_hw_features hw_features;

    ff_sc_transmit sc_transmit ;
} ;

struct ff_veth_softc *ff_veth_attach(struct ff_veth_conf *conf, void *host_ctx);
int ff_veth_detach(struct ff_veth_softc *sc);

void ff_veth_process_packet(struct ff_veth_softc *sc, void *m);

#endif /* ifndef _FSTACK_VETH_H */

