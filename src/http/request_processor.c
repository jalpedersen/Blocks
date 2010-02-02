/*
 * request_processor.c
 *
 *  Created on: Feb 1, 2010
 *      Author: jalp
 */
#include "request_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <util/lua_util.h>
#include <util/log.h>
#include <comm/messagebus.h>

struct http_conn_info {
	char *path;
	char *query;
	int client_sd;
	lua_State *L;
};

static const char *header = "HTTP/1.1 200 OK\nConnection: Close\nContent-Type: text/html\r\n\n";

static int start_processing(http_parser *parser) {
	struct http_conn_info *conn_info;
	lua_State *L;
	conn_info = (struct http_conn_info*)parser->data;
	/* Send HTTP headers to client */
	send(conn_info->client_sd, header, strlen(header), 0);
	L = conn_info->L;
	lua_settop(L, 0);
	lua_getglobal(L, "dispatch");
	lua_pushstring(L, conn_info->path);
	lua_pushstring(L, conn_info->query);
	lua_pushinteger(L, conn_info->client_sd);
	lua_eval(L);
	return 0;
}

int print_data(http_parser *parser, const char *data, size_t size) {
	char str[size+1];
	strncpy(str, data, size);
	str[size] = '\0';
	log_debug("data: %s", str);
	return 0;
}
int on_query(http_parser *parser, const char *data, size_t size) {
	struct http_conn_info *conn_info;
	char *str;
	str = malloc(sizeof(char) * size +1);
	conn_info = (struct http_conn_info*)parser->data;
	str[size] = '\0';
	strncpy(str, data, size);
	conn_info->query = str;
	return 0;
}
int on_path(http_parser *parser, const char *data, size_t size) {
	struct http_conn_info *conn_info;
	char *str;
	str = malloc(sizeof(char) * size +1);
	conn_info = (struct http_conn_info*)parser->data;
	str[size] = '\0';
	strncpy(str, data, size);
	conn_info->path = str;
	return 0;
}

http_parser *request_processor_reset(http_parser *parser, int client_sd, lua_State *L) {
	struct http_conn_info *conn_info;
	conn_info = parser->data;
	if (conn_info != NULL) {
		free(conn_info->path);
		free(conn_info->query);
	}
	http_parser_init(parser, parser->type);

	conn_info->path=NULL;
	conn_info->query=NULL;
	conn_info->L = L;
	conn_info->client_sd = client_sd;
	parser->data = conn_info;

	parser->on_message_begin = NULL;
	parser->on_header_field = NULL;
	parser->on_header_value = NULL;
	parser->on_path = on_path;
	parser->on_url = print_data;
	parser->on_fragment = NULL;
	parser->on_query_string = on_query;
	parser->on_body = print_data;
	parser->on_headers_complete = start_processing;
	parser->on_message_complete = NULL;

	return parser;
}

http_parser *request_processor_init() {
	struct http_conn_info *conn_info;
	http_parser *parser = malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);
	conn_info = malloc(sizeof(struct http_conn_info));
	conn_info->path = NULL;
	conn_info->query = NULL;
	parser->data = conn_info;
	return parser;
}

int request_processor_destroy(http_parser *parser) {
	struct http_conn_info *conn_info = parser->data;
	if (conn_info != NULL) {
		free(conn_info->query);
		free(conn_info->path);
		free(conn_info);
	}
	free(parser);
}

static int l_send(lua_State *L) {
	int sd, i, sent, received;
	sd = luaL_checkinteger(L, 1);
	sent = received = 0;
	for (i = 2; i <= lua_gettop(L); i++) {
		received += lua_objlen(L, i);
		sent += send(sd, lua_tostring(L, i), lua_objlen(L, i), 0);
		if (sent < 0) {
			lua_pushstring(L, "send failed");
			lua_error(L);
			return 0;
		} 
	}
	lua_pushinteger(L, sent);
	lua_pushinteger(L, received);
	return 2;
}

static int l_close(lua_State *L) {
	int sd;
	sd = luaL_checkinteger(L, 1);
	if (close(sd) < 0) {
		lua_pushstring(L, "close failed");
		lua_error(L);
		return 0;
	}
	return 0;
}

static int l_receive(lua_State *L) {
	return 0;
}

static const luaL_reg netio_functions[] = {
		{"send", l_send},
		{"close", l_close},
		{"receive", l_receive},
		{ NULL, NULL}
};

int luaopen_netio(lua_State *L) {
	luaL_register(L, "netio", netio_functions);
	lua_settop(L, 0);

}
