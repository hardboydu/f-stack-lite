#include <stdio.h>
#include <stdlib.h>

#include "list.h"

#include "timingwheel.h"

struct timingwheel_timer {
	void             *ctx ;
	timingwheel_func  func ;

	int               ms ;
	int               level ;
	struct list_head  node ;
} ;

struct timingwheel {
	int p ;
	int size ;
	struct list_head *slot ;
} ;

struct timingwheel *tw_open(int size) {
	struct timingwheel *tw = (struct timingwheel *)calloc(1, sizeof(struct timingwheel)) ;
	if(tw == NULL) {
		return NULL ;
	}

	if(size % 10 == 0) {
		tw->size = size / 10 ;
	} else {
		tw->size = (size / 10) + 1 ;
	}
	tw->slot = (struct list_head *)calloc(1, sizeof(struct list_head) * tw->size);
	if(tw->slot == NULL) {
		free(tw);
		return NULL ;
	}


	int i ;
	for (i = 0; i < tw->size; i++) {
		INIT_LIST_HEAD(&tw->slot[i]) ;
	}

	return tw ;
}

void tw_close(struct timingwheel *tw) {
	int i ;

	if(tw == NULL) return ;

	if(tw->slot) {
		for(i = 0; i < tw->size; i++) {
			struct list_head *head = &tw->slot[i] ;
			struct list_head *list, *temp ;

			list_for_each_safe(list, temp, head) {
				struct timingwheel_timer *timer = list_entry(list, struct timingwheel_timer, node) ;
				list_del(list);
				free(timer);
			}
		}
		free(tw->slot);
	}

	free(tw);
}

struct timingwheel_timer *tw_add_timer(struct timingwheel *tw, timingwheel_func func, void *ctx, int ms) {
	struct timingwheel_timer *timer = (struct timingwheel_timer *)calloc(1, sizeof(struct timingwheel_timer));

	int n = (ms / 10) ;
	int p = (tw->p + n) % tw->size ;
	int l = n / tw->size ;

	timer->ms    = ms ;
	timer->level = l ;
	timer->func  = func ;
	timer->ctx   = ctx ;

	list_add_tail(&timer->node, &tw->slot[p]);

	return timer ;
}

void tw_del_timer(struct timingwheel *tw, struct timingwheel_timer *timer) {
	if(timer) {
		list_del(&timer->node);
		free(timer);
	}
}

void tw_trigger(struct timingwheel *tw) {
	tw->p++ ;
	if(tw->p == tw->size) { 
		tw->p = 0 ;
	}

	struct list_head *head = &tw->slot[tw->p] ;
	struct list_head *list, *temp ;

	list_for_each_safe(list, temp, head) {
		struct timingwheel_timer *timer = list_entry(list, struct timingwheel_timer, node) ;

		if(timer->level == 0) {
			if(timer->func) {
				int ret = timer->func(tw, timer, timer->ctx);
				if(ret == TW_SUCCESS) {

					int n = (timer->ms / 10) ;
					int p = (tw->p + n) % tw->size ;
					int l = n / tw->size ;

					timer->level = l ;

					list_del(list);
					list_add_tail(&timer->node, &tw->slot[p]);
				} else if(ret == TW_CLOSE) {
					list_del(list);
					free(timer);
				}
			}
		} else {
			timer->level-- ;
		}
	}
}

