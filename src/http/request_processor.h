/*
 * request_processor.h
 *
 *  Created on: Feb 1, 2010
 *      Author: jalp
 */

#ifndef REQUEST_PROCESSOR_H_
#define REQUEST_PROCESSOR_H_
#include <http_parser.h>
#include <lua.h>

http_parser *request_processor_reset(http_parser *parser, int client_sd, lua_State *L);

http_parser *request_processor_init();

int request_processor_destroy(http_parser *parser);

int luaopen_netio(lua_State *L);

#endif /* REQUEST_PROCESSOR_H_ */
