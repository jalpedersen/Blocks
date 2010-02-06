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
#include <errno.h>
#include <time.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <util/lua_util.h>
#include <util/log.h>
#include <comm/messagebus.h>
#include "file_util.h"

extern FILE *fdopen (int __fd, __const char *__modes);

struct http_conn_info {
	char *path;
	int path_size;
	char *query;
	int query_size;
	FILE *client_fd;
	int client_sd;
	FILE *temp_file;
	lua_State *L;
	char type;
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

struct mime_type {
	const char *postfix;
	int postfix_size;
	const char *mimetype;
	int mimetype_size;

};
static struct mime_type json_mimetype = {"json", 4, "application/json", 16};
static struct mime_type bin_mimetype = {"bin", 3, "application/octet-stream", 24};

static struct mime_type mimetypes[] = {
		{"html", 4, "text/html", 9},
		{"htm", 3, "text/html", 9},
		{"png", 3, "image/png", 9},
		{"css", 3, "text/css", 8},
		{"js", 2, "text/javascript", 15},
		{"txt", 3, "text/plain", 10},
		NULL
};
#define DEFAULT_ROOT "./html"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

static int send_header(FILE *fd, int status, const char *status_msg,
		struct mime_type *type) {
	char timebuffer[64];
	time_t now;
	now = time(NULL);
	strftime(timebuffer, sizeof(timebuffer), RFC1123FMT, gmtime(&now));


	return fprintf(fd, "HTTP/1.1 %d %s\nConnection: Close\nDate: %s\nContent-Type: %s\r\n\n",
			status, status_msg, timebuffer, type->mimetype);

}

static int http_lua_eval(http_parser *parser) {
	struct http_conn_info *conn_info;
	lua_State *L;
	conn_info = (struct http_conn_info*)parser->data;

	/* Send HTTP headers to client */
	send_header(conn_info->client_fd, 200, "OK", &json_mimetype);
	L = conn_info->L;
	lua_settop(L, 0);
	lua_getglobal(L, "dispatch");
	lua_pushstring(L, conn_info->path);
	lua_pushstring(L, conn_info->query);
	//lua_pushinteger(L, conn_info->client_sd);
	switch(parser->method) {
	case HTTP_GET: lua_pushstring(L, "GET");break;
	case HTTP_POST: lua_pushstring(L, "POST");break;
	default: lua_pushstring(L, "UNKNOWN");
	}
	lua_eval(L);
	return 0;
}

static struct mime_type *get_mimetype(const char *path) {
	char *postfix;
	struct mime_type *t;
	postfix = rindex(path, '.');
	if (postfix != NULL) {
		postfix += 1;
		t = mimetypes;
		while (t != NULL) {
			if (strncasecmp(t->postfix, postfix, t->postfix_size) == 0) {
				return t;
			}
			t += 1;
		}
	}
	return &bin_mimetype;
}

static int http_send_file(http_parser *parser) {
	struct http_conn_info *conn_info;
	int r_bytes, w_bytes;
	FILE *fd;
	const char *file;

	conn_info = (struct http_conn_info*)parser->data;
	file = conn_info->path;
	fd = fopen(file, "r");
	if (fd == NULL) {
		send_header(conn_info->client_fd, 404, "NOT FOUND", get_mimetype(file));
	} else {
		send_header(conn_info->client_fd, 200, "OK", get_mimetype(file));
		send_file(fd, conn_info->client_fd);
		fclose(fd);
	}
	return 0;
}

static int start_processing(http_parser *parser) {
	struct http_conn_info *conn_info;
	conn_info = (struct http_conn_info*)parser->data;

	if (strncmp("/l/", conn_info->path, 3) == 0) {
		conn_info->type = 'L';
		http_lua_eval(parser);
	} else {
		log_debug("path : %s", conn_info->path);
		conn_info->type = 'S';
		http_send_file(parser);
	}
	return 0;
}
static int message_complete(http_parser *parser) {
	struct http_conn_info *conn_info;
	conn_info = (struct http_conn_info*)parser->data;
	fflush(conn_info->client_fd);
	if (conn_info->type != 'L') {
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
	int written;
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

http_parser *request_processor_reset(http_parser *parser, int client_sd, lua_State *L) {
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
	conn_info->L = L;
	conn_info->client_sd = client_sd;
	conn_info->client_fd = fdopen(client_sd, "r+");
	parser->data = conn_info;

	parser->on_message_begin = NULL;
	parser->on_header_field = NULL;
	parser->on_header_value = NULL;
	parser->on_path = on_path;
	parser->on_query_string = on_query;
	parser->on_body = on_body;
	parser->on_headers_complete = start_processing;
	parser->on_message_complete = message_complete;

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
