#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "module.h"
#include "socket.h"
#include "event.h"
#include "module_http.h"

static int module_cb_connected(struct module *mod) ;
static int module_cb_read(struct module *mod, const char *buf, uint32_t len) ;
static int module_cb_write(struct module *mod, const char *buf, uint32_t len) ;

struct module *module_open(const char *name) {
	struct module *mod = (struct module *)calloc(1, sizeof(struct module));
	if(mod == NULL) {
		return NULL ;
	}

	if(strncmp(name, "http", 4) == 0) {
		mod->ctx = module_http_open(mod);

		mod->cb.connected = module_cb_connected ; 
		mod->cb.read      = module_cb_read ; 
		mod->cb.write     = module_cb_write ; 
	}

	return mod ;
}

static int module_cb_connected(struct module *mod) {
	struct module *mod_upper ;
	struct sock   *s = mod->sock ;
	if (mod->node.next != &s->mods) {
		mod_upper = list_entry(mod->node.next, struct module, node) ;
		mod_upper->op.connected(mod_upper) ;
	}

	return 0 ;
}

static int module_cb_read(struct module *mod, const char *buf, uint32_t len) {
	struct sock *s = mod->sock ;

	if ( mod->node.next != &s->mods ) {
		struct module *mod_upper = list_entry ( mod->node.next, struct module, node ) ;
		mod_upper->op.read(mod_upper, buf, len) ;
	}

	return 0 ;
}

static int module_cb_write(struct module *mod, const char *buf, uint32_t len) {
	struct main_app *app = mod->app ;
	struct sock *s = mod->sock ;
	uint32_t send_len = 0 ;

	if ( mod->node.prev == &s->mods ) {
		int ret = socket_write(s, (uint8_t *)buf, len, &send_len ) ;
		if ( ret == SOCK_SUCCESS ) {
			event_mod(app->event, s->fd, s, EPOLLIN) ;
		} else if ( ret == SOCK_INPROCESS ) {
			event_mod(app->event, s->fd, s, EPOLLIN | EPOLLOUT ) ;
		}
	} else {
		struct module *mod_under = list_entry ( mod->node.prev, struct module, node ) ;
		mod_under->op.write (mod_under, buf, len ) ;
	}

	return 0 ;
}

static int module_cb_reset(struct module *mod) {
	return 0 ;
}

static int module_cb_close(struct module *mod) {
	return 0 ;
}

