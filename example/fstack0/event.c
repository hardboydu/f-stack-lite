#include <stdlib.h>

#include "event.h"

struct event *event_open(void) {
	struct event *ev = calloc(1, sizeof(struct event)) ;
	if(ev == NULL) {
		return NULL ;
	}

	ev->fd = ff_epoll_create(0) ;
	if(ev->fd < 0) {
		free(ev);

		return NULL ;
	}

	return ev ;
}

void event_close(struct event *ev) {
	ff_close(ev->fd);
	free(ev);
}

int event_add (struct event *ev, int fd, void *arg, uint32_t events) {
	struct epoll_event event ;

	event.events = events ;
	event.data.ptr = arg ;
	if (ff_epoll_ctl(ev->fd, EPOLL_CTL_ADD, fd, &event) == -1 ) {
		return EVENT_ERROR ;
	}

	return EVENT_SUCCESS ;
}

int event_mod (struct event *ev, int fd, void *arg, uint32_t events) {
	struct epoll_event event ;

	event.events = events ;
	event.data.ptr = arg ;
	if (ff_epoll_ctl(ev->fd, EPOLL_CTL_MOD, fd, &event) == -1 ) {
		return EVENT_ERROR ;
	}

	return EVENT_SUCCESS ;
}

int event_del (struct event *ev, int fd) {
	if (ff_epoll_ctl(ev->fd, EPOLL_CTL_DEL, fd, NULL) == -1 ) {
		return EVENT_ERROR ;
	}

	return EVENT_SUCCESS ;
}

int event_wait (struct event *ev) {
	return ff_epoll_wait(ev->fd, ev->events, MAX_EVENTS, 0) ;
}

