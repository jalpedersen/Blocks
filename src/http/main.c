/*
 * main.c
 *
 *  Created on: Jan 11, 2010
 *      Author: jalp
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

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
static mb_channel_t *tcp_channel;
static volatile int is_alive;

static http_parser **parser_pool;
static int parser_stack_top;
static int parser_pool_size;
static http_parser_settings *parser_settings;

static void init_http_parser_pool(int size) {
	int i;
	parser_pool_size = size;
	parser_pool = malloc(sizeof(http_parser*) * size);
	parser_stack_top = 0;
	for (i = 0; i < size; i++) {
		parser_pool[i] = request_processor_init();
	}
}
static http_parser *get_http_parser() {
	http_parser *parser;
	if (parser_pool_size > parser_stack_top) {
		parser = parser_pool[parser_stack_top];
		parser_stack_top += 1;
	} else {
		parser = request_processor_init();
	}
	return parser;
}
static void release_http_parser(http_parser *parser) {
	parser_stack_top -= 1;
	if (parser_pool_size > parser_stack_top) {
		parser_pool[parser_stack_top] = parser;
	} else {
		request_processor_destroy(parser);
	} 
}

int dispatch(int client_sd, size_t size, int position, void *data, void **aux_data) {
	http_parser *parser = *(http_parser**)aux_data;
	return http_parser_execute(parser, parser_settings, (char *)data, size);
}

int msg_start(int client_sd, void **aux_data) {
	http_parser *parser = get_http_parser();
	*aux_data = request_processor_reset(parser, client_sd, conf);
	return 0;
}

int msg_end(int client_sd, void **aux_data) {
	release_http_parser(*(http_parser**)aux_data);
	*aux_data = NULL;
	return 0;
}

void term_handler(int signal) {
    is_alive = 0;
    mb_channel_stop(tcp_channel);
}

int main(int argc, char **argv) {
	mb_handler_t handler;

	conf = httpd_conf_load("scripts/eva.cfg");

	tcp_channel = mb_channel_bind_port(conf->port);
	is_alive = tcp_channel != NULL;
	log_info("Listening on: %d", conf->port);
	/* setup handlers */
	handler.part_cb = dispatch;
	handler.begin_cb = msg_start;
	handler.end_cb = msg_end;
	init_http_parser_pool(3);
	parser_settings = request_processor_settings_init();
	log_info("Running as pid: %d", getpid());
    signal(SIGTERM, term_handler);
    signal(SIGINT, term_handler);
    signal(SIGPIPE, SIG_IGN);
	while (is_alive) {
		mb_channel_receive(tcp_channel, &handler);
	}
    log_info("Stopping");
	if (tcp_channel != NULL) {
		mb_channel_destroy(tcp_channel);
		return 0;
	}
	request_processor_settings_destroy(parser_settings);
	return 1;
}
