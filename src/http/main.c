/*
 * main.c
 *
 *  Created on: Jan 11, 2010
 *      Author: jalp
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <microhttpd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <util/log.h>
#include <util/lua_util.h>

#include <pthread.h>

#define PORT 8888

lua_State *L = NULL;

struct connection_info {
	const char *method;
	const char *url;
	struct MHD_Connection *connection;
};

int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key,
		  const char *value) {
	log_debug("%s = %s", key, value);
	return MHD_YES;
}


static int create_response(void *arg, uint64_t position, char *buffer, int max) {
	const char *head = "<html><head><title>Blocks</title></head><body>";
	const char *end = "</body></html>";
	struct connection_info *con_info = arg;

	log_debug("Url:%s. Position: %d. Max: %d", con_info->url, (int)position, max);
	
	if (position == 0) {
		return sprintf(buffer, "%s<h1>Hello there. You've requested %s</h1> %s", 
			       head, con_info->url, end);
	}
	return -1;

	
}
static void free_response(void *arg) {
	log_debug("Freeing response");
}

static void *test_thread(void *args) {
	const char *start = "<html><body>Hello there!";
	const char *end = "<br/>And goodbye!</body></html>";
	int ret;
	struct connection_info *con_info;
	struct MHD_Response *response;
	con_info = args;
	response = MHD_create_response_from_callback(-1, 
						     512, 
						     &create_response, 
						     (void*)con_info, 
						     &free_response);
	MHD_queue_response(con_info->connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return NULL;
}

int dispatch(void *cls, struct MHD_Connection *connection,
	     const char *url, const char *method, const char *version,
	     const char *upload_data, size_t *upload_data_size, void **con_cls) {
	int ret;
	pthread_t thread;
	struct connection_info *con_info;
	printf("New request %s for %s using version %s\n", method, url, version);
	MHD_get_connection_values(connection, MHD_HEADER_KIND, print_out_key, NULL);
	con_info = malloc(sizeof(struct connection_info));
	con_info->connection = connection;
	con_info->method = method;
	con_info->url = url;
	pthread_create(&thread, NULL, &test_thread, (void*)con_info);
	return MHD_YES;
}

int main(int argc, char **argv) {
	struct MHD_Daemon *daemon;
	int port;
	L = luaL_newstate();
	luaL_openlibs(L);

	luaL_loadfile(L, "scripts/init.lua");
	lua_eval(L);
	/* Load configuration */
	lua_getglobal(L, "port");
	if (lua_isnumber(L, -1)) {
		port = lua_tointeger(L, -1);
	} else {
		port = PORT;
	}
	
	daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
				  &dispatch, NULL, MHD_OPTION_END);
	if (NULL == daemon) {
		log_error("Failed to bind to port %d", port);
		return 1;
	}
	log_info("Service listening on %d", port);
	while ('q' != getchar()) {
	}
	lua_close(L);
	MHD_stop_daemon(daemon);
	return 0;
}
