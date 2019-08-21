#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "module.h"
#include "module_http.h"

#include "http_parser.h"

struct module_http {
	struct module *mod ;

	http_parser response ;
	struct http_parser_settings setting ;

	int count ;
} ;

static int module_http_on_connected(struct module *mod) ;
static int module_http_on_read(struct module *mod, const char *buf, uint32_t len) ;
static int module_http_on_write(struct module *mod, const char *buf, uint32_t len) ;
static int module_http_on_reset(struct module *mod) ;
static int module_http_on_close(struct module *mod) ;

static int http_message_begin_cb (http_parser *p) ;
static int http_request_url_cb (http_parser *p, const char *buf, size_t len) ;
static int http_status_cb (http_parser *p, const char *buf, size_t len) ;
static int http_header_field_cb (http_parser *p, const char *buf, size_t len) ;
static int http_header_value_cb (http_parser *p, const char *buf, size_t len) ;
static int http_header_complete_cb (http_parser *p) ;
static int http_body_cb (http_parser *p, const char *buf, size_t len) ;
static int http_message_complete_cb (http_parser *p) ;

void *module_http_open(struct module *mod) {
	struct module_http *http = (struct module_http *)calloc(1, sizeof(struct module_http)) ;
	if(http == NULL) {
		return NULL ;
	}
	
	http->mod = mod ;

	mod->op.on_connected = module_http_on_connected ;
	mod->op.on_read      = module_http_on_read ;
	mod->op.on_write     = module_http_on_write ;
	mod->op.on_reset     = module_http_on_reset ;
	mod->op.on_close     = module_http_on_close ;

	http_parser_init(&(http->response), HTTP_RESPONSE) ;

	http->response.data = (void *)http ;

	http->setting.on_message_begin    = http_message_begin_cb ;
	http->setting.on_url              = http_request_url_cb ;
	http->setting.on_status           = http_status_cb ;
	http->setting.on_header_field     = http_header_field_cb ;
	http->setting.on_header_value     = http_header_value_cb ;
	http->setting.on_headers_complete = http_header_complete_cb ;
	http->setting.on_body             = http_body_cb ;
	http->setting.on_message_complete = http_message_complete_cb ;

	return http ;
}

char HTTP_GET_EXAMPLE[] = 
"GET / HTTP/1.1\r\n"
"Host: 192.168.100.9\r\n"
"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/76.0.3809.100 Safari/537.36\r\n"
"\r\n";

static int module_http_on_connected(struct module *mod) {
	mod->cb.on_write(mod, HTTP_GET_EXAMPLE, sizeof(HTTP_GET_EXAMPLE) - 1);
	return 0 ;
} 

static int module_http_on_read(struct module *mod, const char *buf, uint32_t len) {
	struct module_http *http = (struct module_http *)mod->ctx ; 

	//write(1, buf, len);

	http_parser_execute(&(http->response), &http->setting, buf, len ) ;
	return 0 ;
}

static int module_http_on_write(struct module *mod, const char *buf, uint32_t len) {
	return 0 ;
}

static int module_http_on_reset(struct module *mod) {
	struct module_http *http = (struct module_http *)mod->ctx ; 
	http_parser_init(&(http->response), HTTP_RESPONSE) ;
	return 0 ;
}

static int module_http_on_close(struct module *mod) {
	return 0 ;
}

static int http_message_begin_cb (http_parser *p) {
	return 0;
}

static int http_request_url_cb (http_parser *p, const char *buf, size_t len) {
	return 0;
}

static int http_status_cb (http_parser *p, const char *buf, size_t len) {
	return 0;
}

static int http_header_field_cb (http_parser *p, const char *buf, size_t len) {
	return 0;
}

static int http_header_value_cb (http_parser *p, const char *buf, size_t len) {
	return 0;
}

static int http_header_complete_cb (http_parser *p) {
	return 0 ;
}

static int http_body_cb (http_parser *p, const char *buf, size_t len) {
	return 0 ;
}

static int http_message_complete_cb (http_parser *p) {
	struct module_http *http = (struct module_http *)p->data ; 
	struct module      *mod  = http->mod ;

	mod->cb.on_write(mod, HTTP_GET_EXAMPLE, sizeof(HTTP_GET_EXAMPLE) - 1);

//	printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa %d\n", http->count++);

	return 0;
}

