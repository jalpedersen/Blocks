/*
 * blocks.c
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <unistd.h>

#include <util/log.h>
#include <util/lua_util.h>
#include "process.h"
#include "blocks.h"
#include "mailbox.h"
#include "lua_message.h"

static thread_pool_t *pool = NULL;

static int l_blocks_init(lua_State *L) {
	if (pool == NULL) {
		pool = threadpool_init(L, 5);
	}
	return 0;
}

static int l_pool_extend(lua_State *L) {
	int size = luaL_checkinteger(L, 1);
	if (pool != NULL) {
		threadpool_extend(pool, size);
	}
	return 0;
}

static int l_spawn(lua_State *L) {
	mailbox_ref_t *mailbox_ref;
	mailbox_t *child_mailbox;
	child_mailbox = process_spawn(pool, L);

	/* Return reference to spawned task's mailbox */
	mailbox_ref = lua_newuserdata(L, sizeof(mailbox_ref_t));
	mailbox_ref->mailbox = child_mailbox;
	luaL_getmetatable(L, MAILBOX_REF_TYPE_NAME);
	lua_setmetatable(L, -2);

	return 1;
}

static int l_sleep(lua_State *L) {
	sleep(luaL_checkint(L, -1));
	return 0;
}

static int l_message_receive(lua_State *L) {
	mailbox_ref_t *mailbox;
	mailbox = mailbox_get(L);

	return 0;
}

static int l_message_send(lua_State *L) {
	mailbox_ref_t *recipient;
	mailbox_ref_t *sender;
	message_ref_t *msg_ref;;
	message_t *msg;
	message_content_t *content;

	recipient = luaL_checkudata(L, 1, MAILBOX_REF_TYPE_NAME);
	sender = mailbox_get(L);

	//log_debug("Sending message from %p to %p", 
        //          (void*)sender, (void*)recipient->mailbox);
	content = lua_message_pop(L, 1);
	msg = mailbox_send(sender, recipient, content);
	if (msg != NULL) {
		/* Return reference to message */
		msg_ref = lua_newuserdata(L, sizeof(message_ref_t));
		msg_ref->message = msg;
		luaL_getmetatable(L, MESSAGE_REF_TYPE_NAME);
		lua_setmetatable(L, -2);
	} else {
		lua_pushboolean(L, 0);
		lua_message_content_destroy(content);
	}
	return 1;
}

static int l_add_to_reply(lua_State *L) {
	return 0;
}

static int l_mailbox_tostring(lua_State *L) {
	mailbox_ref_t *ref;
	ref = luaL_checkudata(L, 1, MAILBOX_REF_TYPE_NAME);
	lua_pushfstring(L, "%s: %p", MAILBOX_TYPE_NAME, (void*)ref->mailbox);
	return 1;
}

static int l_message_tostring(lua_State *L) {
	message_ref_t *ref;
	ref = luaL_checkudata(L, 1, MESSAGE_REF_TYPE_NAME);
	lua_pushfstring(L, "%s: %p", MESSAGE_TYPE_NAME, (void*)ref->message);
	return 1;
}

static int l_mailbox_message_get(lua_State *L) {
	message_ref_t *ref;
	message_t *msg;

	ref = luaL_checkudata(L, 1, MESSAGE_REF_TYPE_NAME);
	msg = mailbox_wait_for_reply(ref->message, 0);
	return lua_message_push(L, mailbox_message_get_content(msg));
}

static int l_mailbox_ref_destroy(lua_State *L) {
	mailbox_ref_t *ref;
	ref = luaL_checkudata(L, 1, MAILBOX_REF_TYPE_NAME);
	mailbox_destroy(ref);
	return 0;
}

static int l_mailbox_message_destroy(lua_State *L) {
	message_ref_t *ref;
	log_debug("Destroying message");
	lua_stackdump(L);
	ref = luaL_checkudata(L, 1, MESSAGE_REF_TYPE_NAME);
	mailbox_message_destroy(ref->message);
	return 0;
}

static const luaL_Reg blocks_functions[] = {
		{"spawn", l_spawn},
		{"sleep", l_sleep},
		{"receive", l_message_receive},
		{"add_to_reply", l_add_to_reply},
		{"extend_pool", l_pool_extend},
		{ NULL, NULL}
};

LUALIB_API int luaopen_blocks(lua_State *L) {
	mailbox_t *mailbox;
	luaL_register(L, "blocks", blocks_functions);
	l_blocks_init(L);

	/* Mailbox reference meta-table */
	luaL_newmetatable(L, MAILBOX_REF_TYPE_NAME);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	lua_pushstring(L, "send");
	lua_pushcfunction(L, l_message_send);
	lua_settable(L, -3);
	lua_pushstring(L, "__metadata");
	lua_pushstring(L, "restricted");
	lua_settable(L, -3);
	lua_pushstring(L, "__type");
	lua_pushstring(L, MAILBOX_REF_TYPE_NAME);
	lua_settable(L, -3);
	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, l_mailbox_tostring);
	lua_settable(L, -3);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, l_mailbox_ref_destroy);
	lua_settable(L, -3);

        /* Message reference meta-table */
	luaL_newmetatable(L, MESSAGE_REF_TYPE_NAME);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	lua_pushstring(L, "get");
	lua_pushcfunction(L, l_mailbox_message_get);
	lua_settable(L, -3);
	lua_pushstring(L, "__metadata");
	lua_pushstring(L, "restricted");
	lua_settable(L, -3);
	lua_pushstring(L, "__type");
	lua_pushstring(L, MESSAGE_REF_TYPE_NAME);
	lua_settable(L, -3);
	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, l_message_tostring);
	lua_settable(L, -3);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, l_mailbox_message_destroy);
	lua_settable(L, -3);

	lua_settop(L, 0);

	/* Register mail box per Lua state */
	mailbox = mailbox_init(L);
	return 0;
}
