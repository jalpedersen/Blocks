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

char *get_full_path(char *path_buffer,
		const char* path, size_t path_length,
		const char* default_file, size_t default_file_length,
		const char *file, size_t file_length) {
	int i, depth;
	char *newfile;
	strncpy(path_buffer, path, path_length);
	strncpy(path_buffer+path_length, file, file_length);
	depth = 0;
	newfile = path_buffer + path_length;
	for (i = 0 ; i < file_length; i++) {
		if (newfile[i] == '.'
				&& newfile[i+1] == '.') {
			if (depth > 0) {
				newfile[i] = '/';
				newfile[i+1] = '/';
				i += 1;
				continue;
			}
		}
		if (newfile[i] == '/') {
			depth += 1;
		}
	}

	if (file_length > 0
			&& newfile[file_length -1] == '/') {
		strncpy(newfile + file_length,
				default_file, default_file_length);
		path_buffer[path_length + file_length +
		            default_file_length] = '\0';
	} else {
		path_buffer[path_length + file_length] = '\0';
	}

	return path_buffer;
}
int send_file(FILE *src, FILE *dst) {
	const size_t buf_size = 1024;
	char buffer[buf_size];

	int r_bytes, w_bytes = 0;
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
