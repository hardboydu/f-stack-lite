#include <unistd.h>

#include "main.h"
#include "socket.h"
#include "module.h"
#include "netio.h"

static int netio_on_error(struct main_app *app, struct sock *s) ;
static int netio_on_read(struct main_app *app, struct sock *s) ;
static int netio_on_write(struct main_app *app, struct sock *s) ;
static int netio_on_connected(struct main_app *app, struct sock *s) ;
static int netio_on_connect_failed(struct main_app *app, struct sock *s) ;

void netio_poll(struct main_app *app) {
	int n = event_wait(app->event) ;

	int i ;
	for (i = 0; i < n; i++) {
		struct sock *sock = (struct sock *)app->event->events[i].data.ptr ;

		if(app->event->events[i].events & EPOLLHUP) {
			netio_on_error(app, sock);
		} else if(app->event->events[i].events & EPOLLERR) {
			netio_on_error(app, sock);
		} else if(app->event->events[i].events & EPOLLRDHUP) {

			/*
			 * The EPOLL transport uses EPOLLRDHUP to detect when the peer 
			 * closes the write side of the socket. Currently Kqueue is not 
			 * able to mimic this behavior and the only way to detect if the
			 * peer has closed is to read. 
			 */

			netio_on_error(app, sock);
		} else {
			if(app->event->events[i].events & EPOLLIN) {
				netio_on_read(app, sock);
			}

			if(app->event->events[i].events & EPOLLOUT) {
				netio_on_write(app, sock);
			}
		}
	}
}

static int netio_on_error(struct main_app *app, struct sock *s) {

	if(s->stat == SOCKET_STAT_CONNECTING) {
		return netio_on_connect_failed(app, s) ;
	}

	int len = socket_read(s, (uint8_t *)app->ff_read_buffer, READ_BUFFER_SIZE);
	if(len > 0) {
		if(!list_empty(&s->mods)) {
			struct module *mod = list_entry(s->mods.next, struct module, node);
			mod->op.read(mod, app->ff_read_buffer, len);
		}
	}

	struct list_head *node ;
	list_for_each(node, &s->mods) {
		struct module *mod = list_entry(s->mods.next, struct module, node);
		mod->op.reset(mod);
	}
		
	event_del(app->event, s->fd);

	int ret = socket_reconnect(s);
	if(ret == SOCK_INPROCESS) {
		event_add(app->event, s->fd, s, EPOLLOUT) ;
	} else {
		socket_close(s);
	}

	return 0 ;
}

static int netio_on_read(struct main_app *app, struct sock *s) {
	int len = socket_read(s, (uint8_t *)app->ff_read_buffer, READ_BUFFER_SIZE);
	if(len > 0) {
		if(!list_empty(&s->mods)) {
			struct module *mod = list_entry(s->mods.next, struct module, node);
			mod->op.read(mod, app->ff_read_buffer, len);
		}
	} else {
		if (len == SOCK_SHUTDOWN) {
			netio_on_error(app, s);
		} else {
			netio_on_error(app, s);
		}
	}

	return 0 ;
}

static int netio_on_write(struct main_app *app, struct sock *s) {
	if(s->stat == SOCKET_STAT_CONNECTING) {
		return netio_on_connected(app, s) ;
	}
	return 0 ;
}

static int netio_on_connected(struct main_app *app, struct sock *s) {
	s->stat = SOCKET_STAT_ESTABLISHED ;
	
	event_mod(app->event, s->fd, s, EPOLLIN);

	if(!list_empty(&s->mods)) {
		struct module *mod = list_entry(s->mods.next, struct module, node);
		mod->op.connected(mod);
	}

	return 0 ;
}

static int netio_on_connect_failed(struct main_app *app, struct sock *s) {

	event_del(app->event, s->fd);
	socket_close(s);

	return 0 ;
}

