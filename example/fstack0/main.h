#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>

#include <rte_eal.h>

#include <ff_api.h>
#include <ff_epoll.h>

#include "event.h"

#define MAX_PKT_BURST          32

#define EPOLL_MAX_EVENTS       10240
#define READ_BUFFER_SIZE       8192

struct timingwheel ;

struct main_app {
	int port ;
	int port_count ;

	uint16_t rx_queue_size ;
	uint16_t tx_queue_size ;
	uint16_t rx_desc_size ;
	uint16_t tx_desc_size ;

	uint16_t max_burst ;

	uint16_t mbuf_size ;
	uint16_t mempool_cache_size ;

	struct rte_mempool *mbuf_pool;

	struct rte_eth_dev_tx_buffer *tx_buffer ;

	struct ff_veth_softc *ff_ifnet ;

	struct event *event ;

	int                ff_epfd;
	struct epoll_event ff_events[EPOLL_MAX_EVENTS];

	char ff_read_buffer[READ_BUFFER_SIZE] ;

	uint64_t sec_tsc ;
	uint64_t pre_tsc ; 
	uint64_t cur_tsc ;

	struct timingwheel *tw ;

	int client_num_config ;
	int client_num_current ;
} ;

struct main_app *main_app_open(int argc, char *argv[]) ;
void main_app_close(struct main_app *app) ;

#endif
