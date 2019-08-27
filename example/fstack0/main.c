#include <stdio.h>
#include <string.h>

#include "main.h"

#include "socket.h"
#include "module.h"
#include "device.h"
#include "netio.h"
#include "timingwheel.h"
#include "vdebug.h"

static int main_timer(struct main_app *app) {
	app->cur_tsc = rte_rdtsc() ; 
	if(app->cur_tsc - app->pre_tsc > (app->sec_tsc / 100)) {
		ff_hardclock();
		ff_update_current_ts();

		app->pre_tsc = app->cur_tsc ;

		tw_trigger(app->tw);
	}

	return 0 ;
}

static int main_loop(struct main_app *app) {

	while (1) {
		main_timer(app) ;
		netio_poll(app) ;
		device_run(app) ;
	}
	return 0 ;
}

int timer_add_connect(struct timingwheel *tw, struct timingwheel_timer *tw_timer, void *ctx) {

	struct main_app *app = (struct main_app *)ctx ;

	struct sock *sock = socket_open_tcp_client() ;
	if(sock == NULL) {
		main_app_close(app);
		return TW_SUCCESS ;
	}

	struct module *mod = module_open("http");
	if(mod) {
		mod->sock = sock ;
		mod->app  = app ;

		list_add_tail(&mod->node, &sock->mods);
	}

	int ret = socket_connect(sock, inet_addr("192.168.100.9"), htons(80));
	if(ret != SOCK_INPROCESS) {
		socket_close(sock);
		main_app_close(app) ;
		return TW_SUCCESS ;
	}

	event_add(app->event, sock->fd, sock, EPOLLOUT) ;

	app->client_num_current++ ;

	if(app->client_num_config == app->client_num_current) {
		return TW_CLOSE ;
	} else {
		return TW_SUCCESS ;
	}

	return TW_SUCCESS ;
}

int timer_status(struct timingwheel *tw, struct timingwheel_timer *tw_timer, void *ctx) {
	struct main_app *app = (struct main_app *)ctx ;

	printf("current connection %d\n", app->client_num_current);

	return TW_SUCCESS ;
}

struct main_app *main_app_open(int argc, char *argv[]) {
	struct main_app *app = (struct main_app *)calloc(1, sizeof(struct main_app));
	if(app == NULL) {
		return NULL ;
	}

	device_init_default(app, argc, argv);

	app->event = event_open() ;
	if(app->event == NULL) {
		main_app_close(app);
		return NULL ;
	}

	app->sec_tsc = rte_get_timer_hz() ;

	app->tw = tw_open(5 * 60 * 1000);
	tw_add_timer(app->tw, timer_add_connect, (void *)app, 10);
	tw_add_timer(app->tw, timer_status, (void *)app, 1000);

	return app ;
}

void main_app_close(struct main_app *app) {
	free(app);
}

int main(int argc, char *argv[]) {
	struct main_app *app = main_app_open(argc, argv) ;
	if(app == NULL) {
		return -1 ;
	}

#if 0
	struct sock *sock = socket_open_tcp_client() ;
	if(sock == NULL) {
		main_app_close(app);
		return -2 ;
	}
	
	struct module *mod = module_open("http");
	if(mod) {
		mod->sock = sock ;
		mod->app  = app ;

		list_add_tail(&mod->node, &sock->mods);
	}

	int ret = socket_connect(sock, inet_addr("192.168.100.9"), htons(80));
	if(ret != SOCK_INPROCESS) {
		socket_close(sock);
		main_app_close(app) ;
		return -3 ;
	}

	event_add(app->event, sock->fd, sock, EPOLLOUT) ;
#endif

	main_loop(app) ;
	
	return 0;
}

