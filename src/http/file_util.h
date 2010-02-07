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

int lua_pushfile(lua_State *L, FILE *file);
#endif /* FILE_UTIL_H_ */
