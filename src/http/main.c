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
#include <pthread.h>

#define PORT 8888

lua_State *L = NULL;
http_parser *http_request_parser;

int dispatch(int client_sd, size_t size, int position, void *data) {
	http_parser *parser = http_request_parser;

	if (position == 0) {
		request_processor_reset(parser, client_sd, L);
	}
	return http_parser_execute(parser, (char *)data, size);
}

int main(int argc, char **argv) {
	int port;
	int is_alive;
	mb_channel_t *tcp_channel;
	L = luaL_newstate();
	luaL_openlibs(L);

	luaL_loadfile(L, "scripts/init.lua");
	lua_eval(L);
	/* Load configuration */
	lua_getglobal(L, "port");
	if (lua_isnumber(L, -1)) {
		port = lua_tointeger(L, -1);
	} else {
		port = PORT;
	}
	tcp_channel = mb_channel_bind_port(port);
	is_alive = tcp_channel != NULL;

	/* Set up HTTP parser */
	http_request_parser = request_processor_init();
	/* Register netio functions in Lua environment */
	luaopen_netio(L);
	while (is_alive) {
		mb_channel_receive(tcp_channel, dispatch);
	}
	lua_close(L);
	request_processor_destroy(http_request_parser);

	if (tcp_channel != NULL) {
		mb_channel_destroy(tcp_channel);
		return 0;
	}
	return 1;
}
