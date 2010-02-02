/*
 * fileio.c
 *
 *  Created on: Feb 2, 2010
 *      Author: jalp
 */
#include <lua.h>
#include <lauxlib.h>
#include <unistd.h>
#include <sys/socket.h>

static int l_send(lua_State *L) {
	int sd, i, sent, received;
	sd = luaL_checkinteger(L, 1);
	sent = received = 0;
	for (i = 2; i <= lua_gettop(L); i++) {
		received += lua_objlen(L, i);
		sent += send(sd, lua_tostring(L, i), lua_objlen(L, i), 0);
		if (sent < 0) {
			lua_pushstring(L, "send failed");
			lua_error(L);
			return 0;
		}
	}
	lua_pushinteger(L, sent);
	lua_pushinteger(L, received);
	return 2;
}

static int l_close(lua_State *L) {
	int sd;
	sd = luaL_checkinteger(L, 1);
	if (close(sd) < 0) {
		lua_pushstring(L, "close failed");
		lua_error(L);
		return 0;
	}
	return 0;
}

static int l_receive(lua_State *L) {
	return 0;
}

static const luaL_reg netio_functions[] = {
		{"send", l_send},
		{"close", l_close},
		{"receive", l_receive},
		{ NULL, NULL}
};

int luaopen_fileio(lua_State *L) {
	luaL_register(L, "fileio", netio_functions);
	lua_settop(L, 0);
	return 0;
}

