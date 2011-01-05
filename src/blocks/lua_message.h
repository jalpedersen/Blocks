/*
 * lua_message.h
 *
 *  Created on: Jan 17, 2010
 *      Author: jalp
 */

#ifndef LUA_MESSAGE_H_
#define LUA_MESSAGE_H_
#include <lua.h>
#include "mailbox.h"

int lua_message_push(lua_State *L, message_content_t *content);

message_content_t *lua_message_pop(lua_State *L, int offset);

void lua_message_content_destroy(message_content_t *content);

#endif /* LUA_MESSAGE_H_ */
