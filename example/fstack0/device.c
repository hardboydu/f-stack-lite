#include <getopt.h>

#include <rte_ethdev.h>
#include <rte_malloc.h>

#include "main.h"
#include "device.h"

#include "ff_dpdk.h"

static const char short_options[] =
	"p:"  /* port id */
	"C:"  /* client numbers */
	;
static struct option long_options[] = {
	{"port"      , required_argument, 0,  'p' },
	{"client-num", required_argument, 0,  'C' },
	{           0,                 0, 0,   0  }
};

int device_init(struct main_app *app, int argc, char *argv[]) {
	int ret = 0 ;

#if 1
	FILE *logfp = NULL ;
	logfp = fopen("/dev/null", "w+");
	rte_openlog_stream(logfp) ;
#endif

	ret = rte_eal_init(argc, argv);
	if(ret < 0) {
		return -1 ;
	}

	argc -= ret ;
	argv += ret ;

	int opt ;
	int option_index ;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != EOF) {
		switch (opt) {
			case 'p':
				app->port = atoi(optarg);
				break;
			case 'C':
				app->client_num_config = atoi(optarg);
				break;
		}
	}

	app->port_count = rte_eth_dev_count_avail();

	return 0 ;
}

static struct rte_eth_conf port_conf = {
    .rxmode = {
        .max_rx_pkt_len = ETHER_MAX_LEN,
    },
};

static int device_setup_port(struct main_app *app, int port_id) {
	int ret = 0 ;

	int queue_id ;

	struct rte_eth_conf     local_port_conf = port_conf;
	struct rte_eth_dev_info dev_info;

	rte_eth_dev_info_get(port_id, &dev_info);
	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE) {
		local_port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
	}

	ret = rte_eth_dev_configure(port_id, app->rx_queue_size, app->tx_queue_size, &local_port_conf);
	if (ret < 0) {
		return -1 ;
	}

	ret = rte_eth_dev_adjust_nb_rx_tx_desc(port_id, &app->rx_desc_size, &app->tx_desc_size) ;
	if (ret < 0) {
		return -1 ;
	}

	for (queue_id = 0; queue_id < app->rx_queue_size; queue_id++) {
		ret = rte_eth_rx_queue_setup(
				port_id, 
				queue_id, 
				app->rx_desc_size,
				rte_eth_dev_socket_id(port_id), 
				NULL, 
				app->mbuf_pool
				);
		if (ret < 0) {
			return -1 ;
		}
	}

	for (queue_id = 0; queue_id < app->tx_queue_size; queue_id++) {
		ret = rte_eth_tx_queue_setup(
				port_id, 
				queue_id, 
				app->tx_desc_size,
				rte_eth_dev_socket_id(port_id), 
				NULL 
				);
		if (ret < 0) {
			return -1 ;
		}
	}

	ret = rte_eth_dev_start(port_id);
	if (ret < 0) {
		return -1 ;
	}

	rte_eth_promiscuous_enable(port_id);

	return 0 ;
}

int device_setup(struct main_app *app) {

	int ret = 0 ;

	app->mbuf_pool = rte_pktmbuf_pool_create(
			"mbuf_pool", 
			app->mbuf_size,
			app->mempool_cache_size, 
			0, 
			RTE_MBUF_DEFAULT_BUF_SIZE,
			rte_socket_id()
			);
	if (app->mbuf_pool == NULL) {
		return -1 ;
	}

	int port_id ;
	RTE_ETH_FOREACH_DEV(port_id) {
		ret = device_setup_port(app, port_id);
		if(ret != 0) {
			return -1 ;
		}
	}

	app->tx_buffer = rte_zmalloc_socket(
			"tx_buffer",
			RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 
			0,
			rte_eth_dev_socket_id(app->port)
			);

	if (app->tx_buffer == NULL) {
		return -1 ;
	}
	rte_eth_tx_buffer_init(app->tx_buffer, MAX_PKT_BURST);

	device_check_all_ports_link_status(0xFF);

    rte_delay_ms(2000);

	return 0 ;
} ;

volatile uint8_t force_quit = 0;
/* Check the link status of all ports in up to 9s, and print them finally */
void device_check_all_ports_link_status(uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint16_t portid;
	uint8_t count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		RTE_ETH_FOREACH_DEV(portid) {
			if (force_quit)
				return;
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf(
					"Port%d Link Up. Speed %u Mbps -%s\n",
						portid, link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n", portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == ETH_LINK_DOWN) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf(" done\n");
		}
	}

}

struct ff_veth_conf ff_veth_conf = {
	.name        = "veth%d",
	.mac         = "00:1B:21:BA:AC:CC",
	.ip          = "192.168.100.8" ,
	.netmask     = "255.255.255.0",
	.broadcast   = "192.168.100.255",
	.gateway     = "192.168.100.1",
	.hw_features = {
		.rx_csum    = 0 ,
		.rx_lro     = 0 ,
		.tx_csum_ip = 0 , 
		.tx_csum_l4 = 0 ,
		.tx_tso     = 0
	}
};

int device_on_transmit(struct ff_veth_softc *ifnet, void *host_ctx, struct mbuf *m) ;

int device_init_default(struct main_app *app, int argc, char *argv[]) {
	app->rx_queue_size      = 1 ;
	app->tx_queue_size      = 1 ;
	app->rx_desc_size       = 1024 ;
	app->tx_desc_size       = 1024 ;
	app->max_burst          = 32 ;
	app->mbuf_size          = 8192 ;
	app->mempool_cache_size = 256 ;

	int ret = device_init(app, argc, argv) ;
	if(ret < 0) {
		main_app_close(app);
		return -1 ;
	}

	ret = device_setup(app);
	if(ret < 0) {
		main_app_close(app);
		return -2 ;
	}

	ff_veth_conf.sc_transmit = device_on_transmit ; 
	app->ff_ifnet = ff_init(&ff_veth_conf, (void *)app);

	return 0 ;
}

int device_on_transmit(struct ff_veth_softc *ifnet, void *host_ctx, struct mbuf *m) {
	struct main_app *app = (struct main_app *)host_ctx ;
	struct rte_mbuf *dpdk_m = dpdk_ff_to_mbuf(app->mbuf_pool, m);

	rte_eth_tx_buffer(app->port, 0, app->tx_buffer, dpdk_m);
	return 0 ;
}

void device_on_recv(struct main_app *app, struct rte_mbuf *dpdk_m) {
	struct mbuf *ff_m = dpdk_mbuf_to_ff(dpdk_m) ;

	ff_veth_process_packet(app->ff_ifnet, ff_m);
}

int device_run(struct main_app *app) {
	int i ;
	struct rte_mbuf *bufs[32];

	const uint16_t nb_rx = rte_eth_rx_burst(
			app->port, 
			0,
			bufs, 
			32
			);

	if (likely(nb_rx != 0)) {
		for(i = 0; i < nb_rx; i++) {
			device_on_recv(app, bufs[i]);
		}
	}

	rte_eth_tx_buffer_flush(app->port, 0, app->tx_buffer);

	return 0 ;
}

