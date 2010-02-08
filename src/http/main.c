/*
 * main.c
 *
 *  Created on: Jan 11, 2010
 *      Author: jalp
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <util/log.h>
#include <util/lua_util.h>
#include <comm/messagebus.h>
#include "request_processor.h"
#include "httpd_conf.h"
#include <pthread.h>

static httpd_conf_t *conf;


static http_parser *get_http_parser() {
	http_parser *parser;
	parser = request_processor_init();
	return parser;
}
static void release_http_parser(http_parser *parser) {
	request_processor_destroy(parser);
}

int dispatch(int client_sd, size_t size, int position, void *data, void **aux_data) {
	http_parser *parser = *(http_parser**)aux_data;
	return http_parser_execute(parser, (char *)data, size);
}

int msg_start(int client_sd, void **aux_data) {
	http_parser *parser = get_http_parser();
	log_debug("Starting %d", client_sd);
	*aux_data = request_processor_reset(parser, client_sd, conf);
	return 0;
}

int msg_end(int client_sd, void **aux_data) {
	log_debug("Ending %d", client_sd);
	release_http_parser(*(http_parser**)aux_data);
	*aux_data = NULL;
	return 0;
}

int main(int argc, char **argv) {
	int is_alive;
	mb_handler_t handler;
	mb_channel_t *tcp_channel;

	conf = httpd_conf_load("scripts/eva.cfg");

	tcp_channel = mb_channel_bind_port(conf->port);
	is_alive = tcp_channel != NULL;
	log_info("Listening on: %d", conf->port);
	/* setup handlers */
	handler.part_cb = dispatch;
	handler.begin_cb = msg_start;
	handler.end_cb = msg_end;

	while (is_alive) {
		mb_channel_receive(tcp_channel, &handler);
	}

	if (tcp_channel != NULL) {
		mb_channel_destroy(tcp_channel);
		return 0;
	}
	return 1;
}
