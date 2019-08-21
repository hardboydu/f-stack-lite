#ifndef __VDEBUG_H__
#define __VDEBUG_H__

#include <stdio.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <rte_ether.h>
#include <rte_arp.h>
#include <rte_ip.h>
#include <rte_icmp.h>
#include <rte_tcp.h>
#include <rte_udp.h>

static inline char *dpdk_trace_mac(struct ether_addr *addr, char *temp, int len) {
	uint8_t *mac = addr->addr_bytes ;
	snprintf(temp, len, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) ;  
	return temp ;
}

static inline void dpdk_trace_arp(const unsigned char *m) {
	char mac_str[20] ;
	char ip_str[20] ;
	struct arp_hdr *arp = (struct arp_hdr *)(m + sizeof(struct ether_hdr)) ;

	fprintf(stdout, "\thardware type : %d\n",   ntohs(arp->arp_hrd));
	fprintf(stdout, "\thardware size : %d\n",   arp->arp_hln);
	fprintf(stdout, "\tprotocol type : %04X\n", ntohs(arp->arp_pro));
	fprintf(stdout, "\tprotocol size : %d\n",   arp->arp_pln);
	fprintf(stdout, "\t       opcode : %d\n",   ntohs(arp->arp_op));

	fprintf(stdout, "\tsender mac    : %s\n",   dpdk_trace_mac(&arp->arp_data.arp_sha    , mac_str, 20)) ;
	fprintf(stdout, "\tsender ipv4   : %s\n",   inet_ntop(AF_INET, &arp->arp_data.arp_sip, ip_str , 20)) ;
	fprintf(stdout, "\ttarget mac    : %s\n",   dpdk_trace_mac(&arp->arp_data.arp_tha    , mac_str, 20)) ;  
	fprintf(stdout, "\ttarget ipv4   : %s\n",   inet_ntop(AF_INET, &arp->arp_data.arp_tip, ip_str , 20)) ;
}

static inline void dpdk_trace_tcp(const unsigned char *m) {
	struct tcp_hdr *tcp  = (struct tcp_hdr *)(m + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr)) ;

	printf("\t\t%d -> %d\n", ntohs(tcp->src_port), ntohs(tcp->dst_port));
}

static inline void dpdk_trace_ipv4(const unsigned char *m) {
	char src[20] ;
	char dst[20] ;
	struct ipv4_hdr *ipv4 = (struct ipv4_hdr *)(m + sizeof(struct ether_hdr)) ;

	fprintf(stdout, "%20s -> %-20s : ", 
			inet_ntop(AF_INET, &ipv4->src_addr, src, 20),
			inet_ntop(AF_INET, &ipv4->dst_addr, dst, 20)
			);

	switch(ipv4->next_proto_id) {
		case IPPROTO_ICMP :
			fprintf(stdout, "ICMPv4\n") ;
			break ;
		case IPPROTO_TCP : 
			fprintf(stdout, "TCP\n") ;
			dpdk_trace_tcp(m);
			break ;
		case IPPROTO_UDP : 
			fprintf(stdout, "UDP\n") ;
			break ;
		default : 
			fprintf(stdout, "unknown\n") ;
			break ;
	}
}

static inline void dpdk_trace(const unsigned char *m) {
	struct ether_hdr *eth = (struct ether_hdr *)m ;

	uint8_t *smac = eth->s_addr.addr_bytes ;
	uint8_t *dmac = eth->d_addr.addr_bytes ;
	fprintf(stdout, 
			"%02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X : ", 
			smac[0], smac[1], smac[2], smac[3], smac[4], smac[5],
			dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]
			) ;

	switch(ntohs(eth->ether_type)) {
		case ETHER_TYPE_ARP : 
			fprintf(stdout, "%s\n", "ARP");
			dpdk_trace_arp(m);
			break ;
		case ETHER_TYPE_IPv4 : 
			fprintf(stdout, "%s\n", "IPv4");
			dpdk_trace_ipv4(m);
			break ;
		default : 
			fprintf(stdout, "unknown\n") ;
	}
}

#define DPDK_TRACE(m) \
	dpdk_trace(m)

struct pcap_file_header {
    uint32_t magic;
    u_short version_major;
    u_short version_minor;
    int32_t thiszone;        /* gmt to local correction */
    uint32_t sigfigs;        /* accuracy of timestamps */
    uint32_t snaplen;        /* max length saved portion of each pkt */
    uint32_t linktype;       /* data link type (LINKTYPE_*) */
};

struct pcap_pkthdr {
    uint32_t sec;            /* time stamp */
    uint32_t usec;           /* struct timeval time_t, in linux64: 8*2=16, in cap: 4 */
    uint32_t caplen;         /* length of portion present */
    uint32_t len;            /* length this packet (off wire) */
};

static int dpdk_enable_pcap(const char* dump_path)
{
    FILE* fp = fopen(dump_path, "w");
    if (fp == NULL) { 
        rte_exit(EXIT_FAILURE, "Cannot open pcap dump path: %s\n", dump_path);
        return -1;
    }

    struct pcap_file_header pcap_file_hdr;
    void* file_hdr = &pcap_file_hdr;

    pcap_file_hdr.magic         = 0xA1B2C3D4;
    pcap_file_hdr.version_major = 0x0002;
    pcap_file_hdr.version_minor = 0x0004;
    pcap_file_hdr.thiszone      = 0x00000000;
    pcap_file_hdr.sigfigs       = 0x00000000;
    pcap_file_hdr.snaplen       = 0x0000FFFF;  //65535
    pcap_file_hdr.linktype      = 0x00000001; //DLT_EN10MB, Ethernet (10Mb)

    fwrite(file_hdr, sizeof(struct pcap_file_header), 1, fp);
    fclose(fp);

    return 0;
}

static int dpdk_dump_packets(const char* dump_path, struct rte_mbuf* pkt) {
    FILE* fp = fopen(dump_path, "a");
    if (fp == NULL) {
        return -1;
    }

    struct pcap_pkthdr pcap_hdr;
    void* hdr = &pcap_hdr;

    struct timeval ts;
    gettimeofday(&ts, NULL);
    pcap_hdr.sec = ts.tv_sec;
    pcap_hdr.usec = ts.tv_usec;
    pcap_hdr.caplen = pkt->pkt_len;
    pcap_hdr.len = pkt->pkt_len;
    fwrite(hdr, sizeof(struct pcap_pkthdr), 1, fp);

    while(pkt != NULL) {
        fwrite(rte_pktmbuf_mtod(pkt, char*), pkt->data_len, 1, fp);
        pkt = pkt->next;
    }

    fclose(fp);

    return 0;
}

#endif

