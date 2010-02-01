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

#include <pthread.h>

#define PORT 8888

lua_State *L = NULL;


int dispatch(int client_sd, size_t size, void *data) {
	log_debug("Got %d bytes: %s", size, (char*)data);
	return 0;
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

	while (is_alive) {
		mb_channel_receive(tcp_channel, dispatch);
	}

	lua_close(L);
	if (tcp_channel != NULL) {
		mb_channel_destroy(tcp_channel);
		return 0;
	}
	return 1;
}
