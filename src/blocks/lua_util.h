/*
 * lua_util.h
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#ifndef LUA_UTIL_H_
#define LUA_UTIL_H_
#include <lua.h>

int lua_eval(lua_State *L);

void lua_stackdump(lua_State *L);
#endif /* LUA_UTIL_H_ */
