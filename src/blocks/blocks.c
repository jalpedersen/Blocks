/*
 * blocks.c
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <unistd.h>

#include "log.h"
#include "thread.h"

static thread_pool_t *pool = NULL;

static int l_blocks_init(lua_State *L) {
	if (pool == NULL) {
		pool = threads_init(L, 5);
	}
	return 0;
}

static int l_spawn(lua_State *L) {
	spawn(pool, L);
	return 1;
}

static int l_sleep(lua_State *L) {
	sleep(luaL_checkint(L, -1));
	return 0;
}
static const luaL_reg blocks_functions[] = {
		{"spawn", l_spawn},
		{"sleep", l_sleep},
		{ NULL, NULL}
};

LUALIB_API int luaopen_blocks(lua_State *L) {
	luaL_register(L, "blocks", blocks_functions);
	l_blocks_init(L);
	lua_settop(L, 0);
	return 0;
}
