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
#include "lua_util.h"
#include "thread.h"
#include "blocks.h"
#include "mailbox.h"


static thread_pool_t *pool = NULL;

static int l_blocks_init(lua_State *L) {
	if (pool == NULL) {
		pool = threads_init(L, 5);
	}
	return 0;
}

static int l_spawn(lua_State *L) {
	mailbox_ref_t *mailbox_ref;
	mailbox_t *child_mailbox;
	child_mailbox = spawn(pool, L);

	/* Return reference to spawned task's mailbox */
	mailbox_ref = lua_newuserdata(L, sizeof(mailbox_ref_t));
	log_debug("Spawning from %p", L);
	mailbox_ref->mailbox = child_mailbox;
	luaL_getmetatable(L, MAILBOX_REF_TYPE_NAME);
	lua_setmetatable(L, -2);

	return 1;
}

static int l_sleep(lua_State *L) {
	sleep(luaL_checkint(L, -1));
	return 0;
}

static int l_receive(lua_State *L) {
	mailbox_t *mailbox;
	mailbox = mailbox_get(L);

	return 0;
}

static int l_send(lua_State *L) {
	mailbox_ref_t *recepient;
	mailbox_t *sender;
	recepient = luaL_checkudata(L, 1, MAILBOX_REF_TYPE_NAME);
	sender = mailbox_get(L);
	log_debug("Sending message from %p to %p", sender, recepient->mailbox);

	lua_pushboolean(L, 1);
	return 1;
}

static const luaL_reg blocks_functions[] = {
		{"spawn", l_spawn},
		{"sleep", l_sleep},
		{"receive", l_receive},
		{ NULL, NULL}
};

static int l_process_tostring(lua_State *L) {
	lua_pushstring(L, "testing");
}
LUALIB_API int luaopen_blocks(lua_State *L) {
	mailbox_t *mailbox;
	luaL_register(L, "blocks", blocks_functions);
	l_blocks_init(L);

	luaL_newmetatable(L, MAILBOX_REF_TYPE_NAME);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	lua_pushstring(L, "send");
	lua_pushcfunction(L, l_send);
	lua_settable(L, -3);
	lua_pushstring(L, "__metadata");
	lua_pushstring(L, "restricted");
	lua_settable(L, -3);
	lua_settop(L, 0);

	/* Register mail box per Lua state */
	mailbox = mailbox_init(L);
	return 0;
}
