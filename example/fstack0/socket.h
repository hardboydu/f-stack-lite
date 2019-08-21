#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "list.h"

enum sock_errno {
	SOCK_SUCCESS   =  0 ,
	SOCK_ERROR     = -1 ,
	SOCK_INPROCESS = -2 ,
	SOCK_SHUTDOWN  = -3
} ;

enum socket_stat {
	SOCKET_STAT_ESTABLISHED = 1 ,
	SOCKET_STAT_CONNECTING ,
	SOCKET_STAT_CLOSE_WAIT
} ;

struct module ;

struct sock {
	int fd ;

	struct sockaddr_in bind_addr ;
	struct sockaddr_in peer_addr ;

	uint16_t type ;
	uint16_t stat ;

	struct list_head pool ;

	struct list_head mods ;
} ;

struct sock *socket_open_empty(void) ;
struct sock *socket_open_tcp_client(void) ;

void socket_close(struct sock *s) ;

int socket_connect (struct sock *s, uint32_t ip, uint16_t port) ;
int socket_reconnect (struct sock *s) ;

int socket_write (struct sock *s, uint8_t *data, uint32_t len, uint32_t *send_len) ;
int socket_read (struct sock *s, uint8_t *rbuf, uint32_t size) ;

#endif

