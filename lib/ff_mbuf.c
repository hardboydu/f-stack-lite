#include <sys/ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/sched.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/if_tap.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <machine/atomic.h>

#include "ff_mbuf.h"

struct mbuf *
ff_mbuf_gethdr(void *pkt, unsigned short total, void *data, unsigned short len, void (*freef)(struct mbuf *m, void *, void *)) {
    struct mbuf *m = m_gethdr(M_NOWAIT, MT_DATA);
    if (m == NULL) {
        return NULL;
    }

    if (m_pkthdr_init(m, M_NOWAIT) != 0) {
        return NULL;
    }

    m_extadd(m, data, len, freef, pkt, NULL, 0, EXT_DISPOSABLE);

    m->m_pkthdr.len = total;
    m->m_len = len;
    m->m_next = NULL;
    m->m_nextpkt = NULL;

/*
    if (rx_csum) {
        m->m_pkthdr.csum_flags = CSUM_IP_CHECKED | CSUM_IP_VALID |
            CSUM_DATA_VALID | CSUM_PSEUDO_HDR;
        m->m_pkthdr.csum_data = 0xffff;
    }
*/

    return m;
}

struct mbuf *
ff_mbuf_get(struct mbuf *m, void *data, uint16_t len)
{
    struct mbuf *prev = m;
    struct mbuf *mb = m_get(M_NOWAIT, MT_DATA);

    if (mb == NULL) {
        return NULL; 
    }

    m_extadd(mb, data, len, NULL, NULL, NULL, 0, 0);

    mb->m_next = NULL;
    mb->m_nextpkt = NULL;
    mb->m_len = len;

    if (prev != NULL) {
        prev->m_next = mb;
    }

    return mb;
}

void
ff_mbuf_free(struct mbuf *m)
{
    m_freem(m);
}

int
ff_mbuf_copydata(struct mbuf *m, void *data, int off, int len)
{
    int ret;
    struct mbuf *mb = m;

    if (off + len > mb->m_pkthdr.len) {
        return -1;
    }

    m_copydata(mb, off, len, data);

    return 0;
}

unsigned char *ff_mbuf_mtod(struct mbuf *m) {
	return mtod(m, unsigned char *);
}

unsigned int ff_mbuf_get_total_length(struct mbuf *m) {
	return m->m_pkthdr.len ;
}

void
ff_mbuf_tx_offload(void *m, struct ff_veth_tx_offload *offload)
{
    struct mbuf *mb = (struct mbuf *)m;
    if (mb->m_pkthdr.csum_flags & CSUM_IP) {
        offload->ip_csum = 1;
    }

    if (mb->m_pkthdr.csum_flags & CSUM_TCP) {
        offload->tcp_csum = 1;
    }

    if (mb->m_pkthdr.csum_flags & CSUM_UDP) {
        offload->udp_csum = 1;
    }

    if (mb->m_pkthdr.csum_flags & CSUM_SCTP) {
        offload->sctp_csum = 1;
    }

    if (mb->m_pkthdr.csum_flags & CSUM_TSO) {
        offload->tso_seg_size = mb->m_pkthdr.tso_segsz;
    }
}

/********************
*  get next mbuf's addr, current mbuf's data and datalen.
*  
********************/
int ff_next_mbuf(void **mbuf_bsd, void **data, unsigned *len)
{
    struct mbuf *mb = *(struct mbuf **)mbuf_bsd;

    *len = mb->m_len;
    *data = mb->m_data;

    if (mb->m_next)
        *mbuf_bsd = mb->m_next;
    else
        *mbuf_bsd = NULL;
    return 0;
}

//t void * ff_mbuf_mtod(void* bsd_mbuf)
//{
//    if ( !bsd_mbuf )
//        return NULL;
//    return (void*)((struct mbuf *)bsd_mbuf)->m_data;
//}

// get source rte_mbuf from ext cluster, which carry rte_mbuf while recving pkt, such as arp.
void* ff_rte_frm_extcl(void* mbuf)
{
    struct mbuf *bsd_mbuf = mbuf;
    
    if ( bsd_mbuf->m_ext.ext_type==EXT_DISPOSABLE && bsd_mbuf->m_ext.ext_free!= NULL ){
        return bsd_mbuf->m_ext.ext_arg1;
    }
    else 
        return NULL;
}

void
ff_mbuf_set_vlan_info(void *hdr, uint16_t vlan_tci) {
    struct mbuf *m = (struct mbuf *)hdr;
    m->m_pkthdr.ether_vtag = vlan_tci;
    m->m_flags |= M_VLANTAG;
    return;
}

