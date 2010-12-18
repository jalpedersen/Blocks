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
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <util/lua_util.h>
#include <util/log.h>
#include <comm/messagebus.h>
#include "file_util.h"
#include "httpd_conf.h"

extern FILE *fdopen (int __fd, __const char *__modes);

#define default_file "index.html"
#define default_file_location "./html/"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

struct http_conn_info {
	char *path;
	int path_size;
	char *query;
	int query_size;
	char *host;
	char *host_size;
	FILE *client_fd;
	int client_sd;
	FILE *temp_file;
	httpd_lua_state_t *lua;
	char type;
	httpd_conf_t *conf;
};


#define save_data(_conn_info, data_ptr, data_size, new_data_ptr, new_data_size) \
		struct http_conn_info *conn_info; \
		char *str; \
		conn_info = (struct http_conn_info*)_conn_info; \
		int new_size = new_data_size + conn_info->data_size; \
		str = realloc(conn_info->data_ptr, sizeof(char) * new_size +1); \
		str[new_size] = '\0'; \
		strncpy((str+conn_info->data_size), new_data_ptr, new_data_size); \
		conn_info->data_ptr = str; \
		conn_info->data_size = new_size; \


static int send_header(FILE *fd, int status, const char *status_msg,
		const char *mimetype) {
	char timebuffer[64];
	time_t now;
	now = time(NULL);
	strftime(timebuffer, sizeof(timebuffer), RFC1123FMT, gmtime(&now));

	return fprintf(fd,
			"HTTP/1.1 %d %s\nServer: Eva 1.0\nConnection: close\nContent-Coding: identity\nDate: %s\nContent-Type: %s\r\n\r\n",
			status, status_msg, timebuffer, mimetype);

}

static lua_State *get_lua_state(httpd_lua_state_t *state) {
	lua_State *L;
	int stack_top;
	stack_top = state->idle_state_top;
	if (stack_top < 0) {
		log_debug("Creating new state");
		L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, state->filename);
		lua_eval(L);
	} else {
		L = state->idle_states[stack_top];
		state->idle_state_top -= 1;
	}
	return L;
}

static void return_lua_state(httpd_lua_state_t *state, lua_State *L) {
	int stack_top;
	stack_top = state->idle_state_top;
	if (stack_top-1 < state->idle_stack_size) {
		stack_top += 1;
		state->idle_states[stack_top] = L;
		state->idle_state_top = stack_top;
	} else {
		lua_close(L);
	}
}

static int http_lua_eval(http_parser *parser) {
	struct http_conn_info *conn_info;
	lua_State *L;
	conn_info = (struct http_conn_info*)parser->data;

	/* Send HTTP headers to client */
	send_header(conn_info->client_fd, 200, "OK", conn_info->lua->mimetype);
	L = get_lua_state(conn_info->lua);
	lua_settop(L, 0);
	lua_getglobal(L, "dispatch");
	lua_pushstring(L, conn_info->path);
	lua_pushstring(L, conn_info->query);
	lua_pushfile(L, conn_info->client_fd);

	switch(parser->method) {
	case HTTP_GET: lua_pushstring(L, "GET");break;
	case HTTP_POST: lua_pushstring(L, "POST");break;
	case HTTP_PUT: lua_pushstring(L, "PUT"); break;
	case HTTP_DELETE: lua_pushstring(L, "DELETE"); break;
	default: lua_pushstring(L, "UNKNOWN");
	}
	lua_eval(L);
	return_lua_state(conn_info->lua, L);
	return 0;
}

static const char *get_mimetype(const char *path, httpd_conf_t *conf) {
	char *postfix;
	struct mimetype *t;
	postfix = rindex(path, '.');
	if (postfix != NULL) {
		postfix += 1;
		for (t = conf->mimetypes; t->postfix; t++) {
			if (strncasecmp(t->postfix, postfix, t->postfix_size) == 0) {
				return t->mimetype;
			}
		}
	}
	return conf->default_mimetype->mimetype;
}

static int http_send_file(http_parser *parser) {
	struct http_conn_info *conn_info;
	FILE *fd;
	const char *file;
	struct stat st;
	int file_ok;
	conn_info = (struct http_conn_info*)parser->data;

	char buffer[conn_info->path_size + sizeof(default_file_location)
	            +sizeof(default_file) + 1];

	file = get_full_path(buffer,
			default_file_location, sizeof(default_file_location)-1,
			default_file, sizeof(default_file)-1,
			conn_info->path, conn_info->path_size);
	if (stat(file, &st) >= 0 && S_ISREG(st.st_mode)) {
		file_ok = 1;
	} else {
		file_ok = 0;
	}
	if (file_ok) {
		fd = fopen(file, "r");
		log_debug("Getting: %s", file);
		send_header(conn_info->client_fd, 200, "OK",
				get_mimetype(file, conn_info->conf));
		send_file(fd, conn_info->client_fd);
		fclose(fd);
	} else {
		const char *err = "<html><head><title>Error</title></head><body>%s</body></html>";
		const char *err_404 = "<pre>File not found</pre>";
		log_debug("404 %s", file);
		send_header(conn_info->client_fd, 404, "NOT FOUND",
				"text/html");
		fprintf(conn_info->client_fd, err, err_404);
	}
	return 0;
}

static httpd_lua_state_t *get_script(const httpd_conf_t *conf, const char* path) {
	httpd_lua_state_t *state;
	state = conf->lua_states;
	while (state != NULL && state->pattern != NULL) {
		if (strncmp(state->pattern, path, state->pattern_size) == 0) {
			return state;
		}
		state += 1;
	}
	return NULL;
}

static int start_processing(http_parser *parser) {
	struct http_conn_info *conn_info;
	httpd_lua_state_t *lua_state;
	conn_info = (struct http_conn_info*)parser->data;

	lua_state = get_script(conn_info->conf, conn_info->path);
	log_debug("%s", conn_info->path);
	if (lua_state != NULL) {
		conn_info->type = 'L';
		conn_info->lua = lua_state;
		http_lua_eval(parser);
	} else {
		//log_debug("path : %s", conn_info->path);
		conn_info->type = 'S';
		http_send_file(parser);
	}
	return 0;
}
static int message_complete(http_parser *parser) {
	struct http_conn_info *conn_info;
	conn_info = (struct http_conn_info*)parser->data;


	if (conn_info->type == 'S') {
		fclose(conn_info->client_fd);
	}

	//log_debug("Message complete: %d", conn_info->client_sd);
	return 0;
}

int print_data(http_parser *parser, const char *data, size_t size) {
	char str[size+1];
	strncpy(str, data, size);
	str[size] = '\0';
	log_debug("data: %s", str);
	return 0;
}

int on_body(http_parser *parser, const char *data, size_t size) {
	struct http_conn_info *conn_info;
	conn_info = (struct http_conn_info*)parser->data;
	return 0;
}
int on_query(http_parser *parser, const char *data, size_t size) {
	save_data(parser->data, query, query_size, data, size);
	return 0;
}

int on_path(http_parser *parser, const char *data, size_t size) {
	save_data(parser->data, path, path_size, data, size);
	return 0;
}

http_parser *request_processor_reset(http_parser *parser, int client_sd, httpd_conf_t *conf) {
	struct http_conn_info *conn_info;
	conn_info = parser->data;
	if (conn_info != NULL) {
		free(conn_info->path);
		free(conn_info->query);
	}
	http_parser_init(parser, parser->type);

	conn_info->path = NULL;
	conn_info->path_size = 0;
	conn_info->query = NULL;
	conn_info->query_size = 0;
	conn_info->host = NULL;
	conn_info->host_size = 0;
	conn_info->lua = NULL;
	conn_info->conf = conf;
	conn_info->client_sd = client_sd;
	conn_info->client_fd = fdopen(client_sd, "r+");
	parser->data = conn_info;
	return parser;
}

http_parser_settings *request_processor_settings_init() {
	http_parser_settings *settings = malloc(sizeof(http_parser_settings));
	settings->on_message_begin = NULL;
	settings->on_header_field = NULL;
	settings->on_header_value = NULL;
	settings->on_path = on_path;
	settings->on_query_string = on_query;
	settings->on_body = on_body;
	settings->on_headers_complete = start_processing;
	settings->on_message_complete = message_complete;
	return settings;
}
void request_processor_settings_destroy(http_parser_settings *settings) {
	free(settings);
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

void request_processor_destroy(http_parser *parser) {
	struct http_conn_info *conn_info = parser->data;
	if (conn_info != NULL) {
		free(conn_info->query);
		free(conn_info->path);
		free(conn_info);
	}
	free(parser);
}
