#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "socket.h"
#include "ff_api.h"

struct sock_buf {
#define SOCK_BUF_SIZE 8192
	char buf[SOCK_BUF_SIZE] ;
	uint32_t pos ;
	uint32_t len ;

	struct list_head node ;
} ;

struct sock *socket_open_empty(void) {
	struct sock *s = NULL ;

	s = (struct sock *)calloc (1, sizeof(struct sock)) ;
	if ( s != NULL ) {
		INIT_LIST_HEAD(&s->pool);
		INIT_LIST_HEAD(&s->mods);
	}

	return s ;
}

int socket_set_nonblocking(int fd) {
	int on = 1 ;
	int ret = ff_ioctl(fd, FIONBIO, &on);
	if(ret < 0) {
		return SOCK_ERROR ;
	}

	return SOCK_SUCCESS ;
}

struct sock *socket_open_tcp_client(void) {
	int err = 0 ;
	struct sock *s = NULL ;

	s = socket_open_empty () ;
	if ( s == NULL ) {
		err = 1 ;
		goto __err ;
	}

	s->fd = ff_socket ( AF_INET, SOCK_STREAM, 0 ) ;
	if ( s->fd < 0 )	{
		err = 2 ;
		goto __err ;
	}

	int ret = socket_set_nonblocking (s->fd) ;
	if(ret != SOCK_SUCCESS) {
		err = 3 ;
		goto __err ;
	}

	return s ;
__err : 
	switch ( err ) {
		case 3 : ff_close (s->fd) ;
		case 2 : socket_close(s) ;
		case 1 : break ;
	}

	return NULL ;
}

void socket_close(struct sock *s) {
	struct sock_buf *buf = NULL ;
	struct list_head *node = NULL ;
	struct list_head *temp = NULL ;

	if (s == NULL) return ;

	list_for_each_safe (node, temp, &s->pool) {
		buf = list_entry(node, struct sock_buf, node) ;

		list_del(node);
		free(buf);
	}

	ff_close(s->fd) ;

	free(s) ;
}

int socket_connect (struct sock *s, uint32_t ip, uint16_t port) {
	s->stat = SOCKET_STAT_CONNECTING ;

	s->peer_addr.sin_family      = AF_INET ;
	s->peer_addr.sin_addr.s_addr = ip ;
	s->peer_addr.sin_port        = port ;

	int ret = ff_connect(s->fd, (struct linux_sockaddr *)&(s->peer_addr), sizeof(s->peer_addr)) ;
	if(ret < 0) {
		if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS) {
			return SOCK_INPROCESS ;
		}
		return SOCK_ERROR ;
	}

	return SOCK_SUCCESS ;
}

int socket_reconnect (struct sock *s) {
	int err = 0 ;

	s->stat = SOCKET_STAT_CONNECTING ;

	ff_close (s->fd) ;

	s->fd = ff_socket(AF_INET, SOCK_STREAM, 0) ;
	if ( s->fd < 0 )	{
		err = 1 ;
		goto __err ;
	}

	int ret = socket_set_nonblocking(s->fd) ;
	if(ret != SOCK_SUCCESS) {
		err = 2 ;
		goto __err ;
	}

	ret = ff_connect(s->fd, (struct linux_sockaddr *)&(s->peer_addr), sizeof(s->peer_addr)) ;
	if(ret < 0) {
		if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS) {
			return SOCK_INPROCESS ;
		}

		err = 3 ;
		goto __err ;
	}

	return SOCK_SUCCESS ;

__err :

	switch ( err ) {
		case 3 :
		case 2 : ff_close (s->fd) ;
		case 1 : break ;
	}

	return SOCK_ERROR ;
}


struct sock_buf *sock_buf_open(void) {
	struct sock_buf *buf = NULL ;
	buf = (struct sock_buf *)calloc (1, sizeof(struct sock_buf)) ;
	if(buf == NULL) {
		return NULL ;
	}

	return buf ;
}

void sock_buf_close(struct sock_buf *buf) {
	free(buf) ;
}

static int socket_write_buffer(struct sock *s, uint8_t *data, uint32_t len) {
	uint32_t pos = 0 ;
	struct sock_buf *buf = NULL ;

	while (pos < len) {
		buf = sock_buf_open() ;
		if ( buf == NULL ) {
			return SOCK_ERROR ;
		}

		if(len > SOCK_BUF_SIZE + pos) {
			memcpy(buf->buf, data + pos, SOCK_BUF_SIZE) ;
			buf->pos = 0 ;
			buf->len = SOCK_BUF_SIZE ;

		} else {
			memcpy(buf->buf, data + pos, len - pos) ;
			buf->pos = 0 ;
			buf->len = len - pos ;
		}
	
		pos += buf->len ;

		list_add_tail(&buf->node, &s->pool) ;
	}

	return SOCK_SUCCESS ;
}

int socket_write (struct sock *s, uint8_t *data, uint32_t len, uint32_t *send_len) {
	int n = 0 ;
	int buf_len = 0 ;
	struct sock_buf *buf = NULL ;
	struct list_head *node = NULL ;
	struct list_head *temp = NULL ;

	if (s->stat == SOCKET_STAT_CONNECTING ) {
		socket_write_buffer ( s, data, len ) ;
		return SOCK_INPROCESS ;
	}

	list_for_each_safe (node, temp, &s->pool) {
		buf = list_entry ( node, struct sock_buf, node ) ;

		buf_len = buf->len - buf->pos ;

		n = ff_write(s->fd, buf->buf + buf->pos, buf_len) ;
		if ( n < 0 ) {
			if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS) {
				socket_write_buffer ( s, data, len ) ;
				return SOCK_INPROCESS ;
			}
			return SOCK_ERROR ;
		}

		*send_len += n ;

		if ( n < buf_len ) {
			buf->pos += n ;

			socket_write_buffer(s, data, len) ;

			return SOCK_INPROCESS ;
		}
		list_del(node) ;
		sock_buf_close(buf) ;
	}

	if ( len > 0 ) {
		n = ff_write( s->fd, data, len) ;
		if ( n < 0 ) {
			if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS) {
				socket_write_buffer ( s, data, len ) ;
				return SOCK_INPROCESS ;
			}
			return SOCK_ERROR ;
		}

		*send_len += n ;

		if (n < (int)len) {
			socket_write_buffer(s, data + n, len - n) ;
			return SOCK_INPROCESS ;
		}
	}

	return SOCK_SUCCESS ;
}

int socket_read (struct sock *s, uint8_t *rbuf, uint32_t size) {
	int n = ff_read ( s->fd, rbuf, size) ;
	if ( n == 0 ) {
		return SOCK_SHUTDOWN ;
	} else if ( n < 0 ) {
		if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS) {
			return SOCK_INPROCESS ;
		}
		return SOCK_ERROR ;
	}

	return n ;
}

