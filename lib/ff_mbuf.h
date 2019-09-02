#ifndef _FSTACK_MBUF_H
#define _FSTACK_MBUF_H

struct mbuf ;

struct mbuf*
ff_mbuf_gethdr(void *pkt, unsigned short total, void *data, unsigned short len, void (*freef)(struct mbuf *m, void *, void *)) ;

struct mbuf *ff_mbuf_get(struct mbuf *m, void *data, unsigned short len);
void ff_mbuf_free(struct mbuf *m);

int ff_mbuf_copydata(struct mbuf *m, void *data, int off, int len);

unsigned char *ff_mbuf_mtod(struct mbuf *m) ;
unsigned int ff_mbuf_get_total_length(struct mbuf *m) ;

int ff_next_mbuf(void **mbuf_bsd, void **data, unsigned *len);
//void* ff_mbuf_mtod(void* bsd_mbuf);
void* ff_rte_frm_extcl(void* mbuf);

struct ff_tx_offload {
    unsigned char ip_csum;
    unsigned char tcp_csum;
    unsigned char udp_csum;
    unsigned char sctp_csum;
    unsigned short tso_seg_size;
};

void ff_mbuf_tx_offload(void *m, struct ff_tx_offload *offload);

void ff_mbuf_set_vlan_info(void *hdr, unsigned short vlan_tci);

#endif

