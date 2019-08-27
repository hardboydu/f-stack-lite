#ifndef __TIMING_WHEEL_H__
#define __TIMING_WHEEL_H__

enum tw_code {
	TW_SUCCESS = 0,
	TW_CLOSE   = 1 
} ;

struct timingwheel ;
struct timingwheel_timer ;

typedef int (*timingwheel_func)(struct timingwheel *tw, struct timingwheel_timer *tw_timer, void *ctx) ;

struct timingwheel *tw_open(int size) ;
void tw_close(struct timingwheel *tw) ;

struct timingwheel_timer *tw_add_timer(struct timingwheel *tw, timingwheel_func func, void *ctx, int ms) ;
void tw_del_timer(struct timingwheel *tw, struct timingwheel_timer *timer) ;
void tw_trigger(struct timingwheel *tw) ;

#endif
