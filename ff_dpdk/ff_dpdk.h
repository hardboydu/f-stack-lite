#ifndef __VFSTACKDPDK_H__
#define __VFSTACKDPDK_H__

#include <ff_mbuf.h>
#include <rte_mbuf.h>

struct rte_mbuf *dpdk_ff_to_mbuf(struct rte_mempool *mempool, struct mbuf *m) {

	uint32_t ff_m_len ;

	struct rte_mbuf *dpdk_m_h = rte_pktmbuf_alloc(mempool);
	if (dpdk_m_h == NULL) {
		return NULL;
	}

	ff_m_len = ff_mbuf_get_total_length(m) ;

	dpdk_m_h->pkt_len = ff_m_len ;
	dpdk_m_h->nb_segs = 0;

	uint32_t ff_off = 0 ;
	struct rte_mbuf *dpdk_m_p = dpdk_m_h ;
	while(ff_m_len > 0) {
		void *data = rte_pktmbuf_mtod(dpdk_m_p, void*);
		int len = ff_m_len > RTE_MBUF_DEFAULT_DATAROOM ? RTE_MBUF_DEFAULT_DATAROOM : ff_m_len;
		int ret = ff_mbuf_copydata(m, data, ff_off, len);
		if (ret < 0) {
			ff_mbuf_free(m);
			rte_pktmbuf_free(dpdk_m_h);
			return NULL ;
		}

		dpdk_m_h->nb_segs++ ;

		dpdk_m_p->data_len  = len ;
		ff_off             += len ;
		ff_m_len           -= len ;

		if(ff_m_len > 0) {
			dpdk_m_p->next = rte_pktmbuf_alloc(mempool);
			if (dpdk_m_p->next == NULL) {
				ff_mbuf_free(m);
				rte_pktmbuf_free(dpdk_m_h);
				return NULL ;
			}

			dpdk_m_p = dpdk_m_p->next ;
		}
	}
	
	ff_mbuf_free(m);

	return dpdk_m_h ;
}

void dpdk_ff_freef(struct mbuf *m, void *arg1, void *arg2) {
	rte_pktmbuf_free((struct rte_mbuf *)arg1);
} ;

struct mbuf *dpdk_mbuf_to_ff(struct rte_mbuf *dpdk_m) {
	unsigned char *d = rte_pktmbuf_mtod(dpdk_m, unsigned char *);
	struct mbuf *ff_m = ff_mbuf_gethdr(dpdk_m, dpdk_m->pkt_len, d, dpdk_m->data_len, dpdk_ff_freef);

	struct rte_mbuf *dpdk_m_p = dpdk_m->next ;
	struct mbuf *ff_m_p = ff_m ;
	while(dpdk_m_p) {

		void *dat = rte_pktmbuf_mtod(dpdk_m_p, void*);
		int   len = rte_pktmbuf_data_len(dpdk_m_p);

		ff_m_p = ff_mbuf_get(ff_m_p, dat, len);
		if(ff_m_p == NULL) {
			ff_mbuf_free(ff_m) ;
			rte_pktmbuf_free(dpdk_m) ;
			return NULL ;
		}

		dpdk_m_p = dpdk_m_p->next ;
	}

	return ff_m ;
}

#endif

