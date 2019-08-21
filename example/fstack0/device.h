#ifndef __DPDK_H__
#define __DPDK_H__

#include <stdio.h>
#include <stdint.h>

struct main_app ;

int device_init(struct main_app *app, int argc, char *argv[]) ;
int device_setup(struct main_app *app) ;
void device_check_all_ports_link_status(uint32_t port_mask) ;

int device_init_default(struct main_app *app, int argc, char *argv[]) ;

int device_run(struct main_app *app) ;

#endif

