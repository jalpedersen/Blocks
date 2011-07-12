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

/*
 * This structure is taken from liolib.c
 */
typedef struct LStream {
	FILE *f;  /* stream */
	lua_CFunction closef;  /* to close stream (NULL for closed streams) */
} LStream;


/* Do not allow users to close sockets */
static int no_close_socket(lua_State *L) {
	LStream *p = (LStream *)luaL_checkudata(L, 1, LUA_FILEHANDLE);
	p->closef = &no_close_socket;
	return luaL_fileresult(L, 1, NULL);
}
/*
static int close_socket(lua_State *L) {
	LStream *p = (LStream *)luaL_checkudata(L, 1, LUA_FILEHANDLE);
	int ret = fclose(p->f);	
	return luaL_fileresult(L, (ret == 0), NULL);
}
*/
int lua_pushfile(lua_State *L, FILE *file) {
	LStream *p = (LStream *)lua_newuserdata(L, sizeof(LStream));
	p->f = file;
	p->closef = &no_close_socket;
	luaL_setmetatable(L, LUA_FILEHANDLE);
	return 1;
}
