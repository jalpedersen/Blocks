/*
 * lua_message.c
 *
 *  Created on: Jan 17, 2010
 *      Author: jalp
 */
#include <stdlib.h>
#include <lauxlib.h>
#include "lua_message.h"
#include <util/log.h>
#include <util/lua_util.h>

struct value {
	short type;
	int length;
	void *data;
};

static message_content_t *content_save(lua_State *L);
static int content_restore(lua_State *L, message_content_t *content);
int dump_function (lua_State *L, int size, const char **dst);

struct values {
	int size;
	struct value *values;
};

int lua_message_push(lua_State *L, message_content_t *content) {
	int argc, i;
	struct value *arg;
	struct values *arguments = content->message;
	argc = arguments->size;
	arg = arguments->values;
	log_debug("arguments: %d (%p)", argc, (void*)arguments)
	for (i = 0; i < argc; i++) {
		log_debug("Restoring a %s", lua_typename(L, arg->type));
		int type = LUA_TSTRING;
		switch (type) {
		case LUA_TFUNCTION:
			break;
		case LUA_TNUMBER:
			break;
		case LUA_TBOOLEAN:
			break;
		case LUA_TNIL:
			break;
		case LUA_TNONE:
			break;
		case LUA_TSTRING:
			lua_pushstring(L, "fisk");
			break;
		default:
			log_debug("type: %s", lua_typename(L, arg->type));
			break;
		}
		arg = arg + 1;
	}
	/* Destroy message when done */
	lua_message_content_destroy(content);

	return argc;
}

static void *get_index(void *heap, int size) {
	return (unsigned char*)heap + size;
}

message_content_t *lua_message_pop(lua_State *L) {
	int argc, i, type;
	int message_size;
	void *raw_message;
	message_content_t *content;
	struct values *args;
	struct value *arg;

	/* Don't include the 'self' object */
	argc = lua_gettop(L)-1;
	message_size = sizeof(message_content_t) + sizeof(struct values)
			+ (sizeof(struct value) * argc);

	raw_message = malloc(message_size);

	content = raw_message;
	args = get_index(raw_message, sizeof(message_content_t));
	content->message = args;
	args->size = argc;
	arg = get_index(args, sizeof(struct values));
	args->values = arg;
	for (i = 2; i <= argc+1; i++) {
		type = lua_type(L, i);
		arg->type = type;
		log_debug("Type: %s. Size: %d", lua_typename(L, type), lua_objlen(L, i));
		arg = arg + 1;
	}

	log_debug("arguments: %d, %p (%p)", argc, content->message, (void*)args->values);

	return content;
}

void lua_message_content_destroy(message_content_t *content) {
	free(content);
}


static int load_string(lua_State *L) {
	int ret;
	size_t fn_dump_size;
	const char *fn_dump;

	fn_dump_size = lua_tointeger(L, lua_upvalueindex(1));
	fn_dump = lua_tolstring(L, lua_upvalueindex(2), &fn_dump_size);

	ret = luaL_loadbuffer(L, fn_dump, fn_dump_size, "Chunk");
	switch (ret) {
	case 0:
		break;
	case LUA_ERRMEM:
	case LUA_ERRSYNTAX:
		luaL_error(L, lua_tostring(L, -1));
	default:
		luaL_error(L, "Unexpected state: %d", ret);
	}

	return 1;
}

static int function_writer (lua_State *L, const void* b, size_t size, void* B) {
	luaL_addlstring((luaL_Buffer*) B, (const char *)b, size);
	return 0;
}

int dump_function (lua_State *L, int size, const char **dest) {
  luaL_Buffer b;
  const char *fn_dump;
  size_t fn_size;
  lua_pushvalue(L, size);
  luaL_checktype(L, size, LUA_TFUNCTION);
  luaL_buffinit(L, &b);
  if (lua_dump(L, function_writer, &b) != 0) {
    luaL_error(L, "Could not dump function");
  }
  luaL_pushresult(&b);
  fn_size = lua_objlen(L, -1);
  fn_dump = lua_tolstring(L, -1, &fn_size);
  if (dest != NULL) {
	  *dest = fn_dump;
  }
  return fn_size;
}
