/*
 * lua_message.c
 *
 *  Created on: Jan 17, 2010
 *      Author: jalp
 */
#include <stdlib.h>
#include <lauxlib.h>
#include <string.h>
#include "lua_message.h"
#include <util/log.h>
#include <util/lua_util.h>

#ifndef MSG_MAX_SIZE
#define MSG_MAX_SIZE (2 * 1024)
#endif
#define MSG_INC_SIZE 256

enum msg_type {
	T_INTEGER,
	T_FLOATING,
	T_STRING,
	T_BOOLEAN,
	T_FUNCTION,
	T_TABLE,
	T_NIL
};

struct value {
	enum msg_type type;
	size_t length;
};

static message_content_t *content_save(lua_State *L);
static int content_restore(lua_State *L, message_content_t *content);
int dump_function (lua_State *L, int size, const char **dst);

struct values {
	int size;
};

#define get_index(heap, position, size) (void*)((unsigned char*)heap + *(position)); *(position)+=size; 

int lua_message_push(lua_State *L, message_content_t *content) {
	int argc, i, heap_position;
	enum msg_type type;
	void *heap, *data;
	struct value *arg;
	struct values *arguments;
	heap = content->message;
	heap_position = 0;
	arguments = get_index(heap, &heap_position, sizeof(struct values));
	argc = arguments->size;
	for (i = 0; i < argc; i++) {
		arg = get_index(heap, &heap_position, sizeof(struct value));
		data = get_index(heap, &heap_position, arg->length);
		type = arg->type;
		switch (type) {
		case T_BOOLEAN:
			lua_pushboolean(L, *(char*)data);
			break;
		case T_INTEGER:
		case T_FLOATING:
		case T_STRING:
			lua_pushlstring(L, data, arg->length);
			break;
		case T_NIL:
			lua_pushnil(L);
			break;
		case T_FUNCTION:
			lua_pushnil(L);
			break;
		case T_TABLE:
			lua_pushnil(L);
			break;
		default:
			log_debug("un-handled type: %d", arg->type);
			break;
		}
	}
	/* Destroy message when done */
	lua_message_content_destroy(content);

	return argc;
}


message_content_t *lua_message_pop(lua_State *L) {
	int argc, i, l_type, type;
	size_t length, value_length;
	int message_size;
	int heap_position;
	int heap_size;
	void *heap, *data;
	message_content_t *content;
	struct values *args;
	struct value *arg;

	/* Don't include the 'self' object */
	argc = lua_gettop(L)-1;

	message_size = sizeof(struct values) + (sizeof(struct value) * argc);
	heap_size = message_size + MSG_INC_SIZE;
	heap = malloc(heap_size);
	heap_position = 0;

	args = get_index(heap, &heap_position, sizeof(struct values));
	args->size = argc;
	for (i = 2; i <= argc+1; i++) {
		l_type = lua_type(L, i);
		switch (l_type) {
		case LUA_TBOOLEAN: type = T_BOOLEAN; length = sizeof(char); break;
		case LUA_TSTRING: type = T_STRING; length = lua_objlen(L, i); break;
		case LUA_TNUMBER: type = T_FLOATING; length = lua_objlen(L, i); break;
		case LUA_TFUNCTION: type = T_FUNCTION; length = lua_objlen(L, i); break;
		case LUA_TNIL: type = T_NIL; length = lua_objlen(L, i); break;
		case LUA_TTABLE: type = T_TABLE; length = lua_objlen(L, i); break;
		default: type = T_NIL; length = lua_objlen(L, i); break;
		}
		value_length = length + sizeof(struct value); 
		if (heap_position + value_length >= heap_size) {
			heap_size += MSG_INC_SIZE + value_length;
			if (heap_size > MSG_MAX_SIZE) {
				lua_pushfstring(L, "Max message size of %d exceeded by %d", MSG_MAX_SIZE, heap_size-MSG_MAX_SIZE); 
				lua_error(L);
				free(heap);
				return NULL;
			}
			heap = realloc(heap, heap_size);
		}
		arg = get_index(heap, &heap_position, sizeof(struct value));
		data = get_index(heap, &heap_position, length);
		arg->length = length;
		arg->type = type;
		if (type == T_BOOLEAN) {
			*(char*)data = lua_toboolean(L, i);
		} else {
			memcpy(data, lua_tolstring(L, i, &length), length);
		}
		//log_debug("Type: %s. Size: %d", lua_typename(L, l_type), length);
	}
	content = malloc(sizeof(message_content_t));
	content->message = heap;
	return content;
}

void lua_message_content_destroy(message_content_t *content) {
	free(content->message);
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
		break;
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
