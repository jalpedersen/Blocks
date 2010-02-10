/*
 * httpd_conf.h
 *
 *  Created on: Feb 8, 2010
 *      Author: jalp
 */

#ifndef HTTPD_CONF_H_
#define HTTPD_CONF_H_
#include <lua.h>
typedef struct mimetype {
	const char *postfix;
	int postfix_size;
	const char *mimetype;
	int mimetype_size;

} mimetype_t;

typedef struct httpd_lua_state {
	const char *pattern;
	size_t pattern_size;
	int idle_state_top;
	int idle_stack_size;
	lua_State **idle_states;
	const char *filename;
	const char *mimetype;
} httpd_lua_state_t;

typedef struct httpd_conf {
	mimetype_t *mimetypes;
	mimetype_t *default_mimetype;
	int port;
	httpd_lua_state_t *lua_states;
} httpd_conf_t;



httpd_conf_t *httpd_conf_load(const char *file);

#endif /* HTTPD_CONF_H_ */
