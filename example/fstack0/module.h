#ifndef __MODULE_H__
#define __MODULE_H__

#include <stdint.h>
#include "list.h"

struct module ;
struct sock ;
struct main_app ;

typedef int (*module_data_cb) (struct module *mod, const char *data, uint32_t len) ;
typedef int (*module_cb) (struct module *mod) ;

struct module_opt {
	module_cb      connected ;
	module_data_cb read ;
	module_data_cb write ;

	module_cb      reset ;
	module_cb      close ;
} ;

struct module {
	void *ctx ;
	struct module_opt op ;
	struct module_opt cb ;

	struct sock     *sock ;
	struct main_app *app ;

	struct list_head node ;
} ;

struct module *module_open(const char *name) ;

#endif
