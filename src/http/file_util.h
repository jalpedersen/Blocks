/*
 * file_util.h
 *
 *  Created on: Feb 6, 2010
 *      Author: jalp
 */

#ifndef FILE_UTIL_H_
#define FILE_UTIL_H_
#include <stdio.h>
#include <lua.h>

int send_file(FILE *src, FILE *dst);

char *get_full_path(char *path_buffer,
					const char* path, size_t path_length,
					const char* default_file, size_t default_file_length,
					const char *file, size_t file_length);

int lua_pushfile(lua_State *L, FILE *file);
#endif /* FILE_UTIL_H_ */
