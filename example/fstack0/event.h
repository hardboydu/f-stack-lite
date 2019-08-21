#ifndef __EVENT_H__
#define __EVENT_H__

#include <ff_api.h>
#include <ff_epoll.h>

#define MAX_EVENTS  10240

enum EVENT_ERRNO {
	EVENT_SUCCESS =  0,
	EVENT_ERROR   = -1
} ;

struct event {
	int                fd;
	struct epoll_event events[MAX_EVENTS];
} ;

struct event *event_open(void) ;
void event_close(struct event *ev) ;

int event_add(struct event *ev, int fd, void *arg, uint32_t events) ;
int event_mod(struct event *ev, int fd, void *arg, uint32_t events) ;
int event_del(struct event *ev, int fd) ;

int event_wait (struct event *ev) ;

#endif

