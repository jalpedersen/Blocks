/*
 * httpd_conf.c
 *
 *  Created on: Feb 8, 2010
 *      Author: jalp
 */

#include "httpd_conf.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdlib.h>

#include <util/log.h>
#include <util/lua_util.h>
#define PORT 8888

static struct mimetype default_mimetypes[] = {
		{"html", 4, "text/html", 9},
		{"htm", 3, "text/html", 9},
		{"png", 3, "image/png", 9},
		{"gif", 3, "image/gif", 9},
		{"jpg", 3, "image/jpeg", 10},
		{"jpeg", 4, "image/jpeg", 10},
		{"css", 3, "text/css", 8},
		{"js", 2, "text/javascript", 15},
		{"txt", 3, "text/plain", 10},
		NULL
};

static struct mimetype bin_mimetype = {"bin", 3, "application/octet-stream", 24};
static struct mimetype json_mimetype = {"json", 4, "application/json", 16};

static int load_scripts(httpd_conf_t *conf, lua_State *L) {
	httpd_lua_state_t *states;
	states = malloc(sizeof(httpd_lua_state_t) * (1 + 1));
	states[0].filename = "scripts/init.lua";
	states[0].L = luaL_newstate();
	luaL_openlibs(states[0].L);
	luaL_loadfile(states[0].L, states[0].filename);
	lua_eval(states[0].L);
	states[0].mimetype = &json_mimetype;
	states[0].pattern = "/l/";
	states[1].filename = NULL;
	conf->lua_states = states;
	return 0;
}

httpd_conf_t *httpd_conf_load(const char *file) {
	httpd_conf_t *conf;
	int port, mimetype_length;
	mimetype_t  *mimetypes;
	lua_State *L;
	conf = malloc(sizeof(httpd_conf_t));
	L = luaL_newstate();
	luaL_openlibs(L);

	luaL_loadfile(L, file);
	lua_eval(L);
	/* Load configuration */
	lua_getglobal(L, "port");
	if (lua_isnumber(L, -1)) {
		port = lua_tointeger(L, -1);
		} else {
		port = PORT;
	}
	conf->port = port;
	lua_pop(L, 1);
	conf->mimetypes = default_mimetypes;
	conf->default_mimetype = &bin_mimetype;
	/* Register mime-types*/
	lua_getglobal(L, "mimetypes");
	if (lua_istable(L, -1)) {
		int i, top;
		mimetype_length = lua_objlen(L, -1);
		log_debug("mimetypes: %d", mimetype_length);

		mimetypes = malloc(sizeof(httpd_conf_t) * (mimetype_length + 1));

		lua_pushnil(L); /*First key*/
		for (i = 0; i < mimetype_length; i++) {
			if (lua_next(L, -2) == 0){
				break;
			}
			top = lua_gettop(L);
			lua_pushnil(L);
			if (lua_next(L, -2) != 0) {
				const char *key;
				const char *value;
				int key_length, value_length;
				key = lua_tostring(L, -2);
				value = lua_tostring(L, -1);
				log_debug("%s: %s", key, value);
				mimetypes[i].mimetype = value;
				mimetypes[i].mimetype_size = value_length;
				mimetypes[i].postfix = key;
				mimetypes[i].postfix_size = key_length;
			} else {
				mimetypes[i].postfix = NULL;
				lua_pushfstring(L, "Error while parsing mime-types in %s.", file);
				lua_error(L);
			}
			lua_settop(L, top);
			lua_pop(L,1);
		}
		mimetypes[i].postfix = NULL;
		conf->mimetypes = mimetypes;
	}
	/* Register scripts */
	load_scripts(conf, L);

	lua_close(L);
	return conf;
}
