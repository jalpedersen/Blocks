/*
 * file_util.c
 *
 *  Created on: Feb 6, 2010
 *      Author: jalp
 */
#include "file_util.h"
#include <util/log.h>
#include <util/lua_util.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

int send_file(FILE *src, FILE *dst) {
	const size_t buf_size = 1024;
	char buffer[buf_size];

	int r_bytes, w_bytes;
	while ((r_bytes = fread(buffer, 1, buf_size, src)) > 0) {
		w_bytes = fwrite(buffer, 1, r_bytes, dst);
		if (w_bytes < r_bytes) {
			log_debug("Failed to send all bytes.");
			break;
		}
	}

	return w_bytes;
}

static int close_socket(lua_State *L) {
	FILE **file = ((FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE));
	if (*file != NULL) {
		fclose(*file);
		*file = NULL;
	}
	return 0;
}

int lua_pushfile(lua_State *L, FILE *file) {
	FILE **client = (FILE **)lua_newuserdata(L, sizeof(FILE *));
	*client = file;
	luaL_getmetatable(L, LUA_FILEHANDLE);
	lua_setmetatable(L, -2);

	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, close_socket);
	lua_setfield(L, -2, "__close");
	lua_setfenv(L, -2);

	return 0;
}
