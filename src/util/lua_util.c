/*
 * lua_util.c
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#include "lua_util.h"
#include "../util/log.h"
#include <lauxlib.h>
static int traceback (lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg)
    luaL_traceback(L, L, msg, 1);
  else if (!lua_isnoneornil(L, 1)) {  /* is there an error object? */
    if (!luaL_callmeta(L, 1, "__tostring"))  /* try its 'tostring' metamethod */
      lua_pushliteral(L, "(no error message)");
  }
  return 1;
}
int lua_eval(lua_State *L) {
	return lua_eval_part(L,
			     lua_gettop(L)-1, /* don't include function */
			     LUA_MULTRET);
}
int lua_eval_part(lua_State *L, int narg, int nres) {
	int ret, error_index, function_index;
	function_index = lua_gettop(L) - narg;
	if ( ! lua_isfunction(L, function_index)) {
		log_error("Runtime error: %s", lua_tostring(L, function_index));
		return LUA_ERRRUN;
	}
	error_index = lua_gettop(L) - narg;
	lua_pushcfunction(L, traceback);
	lua_insert(L, error_index);
	ret = lua_pcall(L, narg, nres, error_index);

	/* Remove traceback function */
	lua_remove(L, error_index);
	switch (ret) {
	case 0:
		/* All is good */
		break;
	case LUA_ERRRUN:
		log_error("Runtime error: %s", lua_tostring(L, -1));
		break;
	case LUA_ERRSYNTAX:
		log_error("Syntax error: %s", lua_tostring(L, -1));
		break;
	case LUA_ERRMEM:
		log_error("Memory allocation error: %s", lua_tostring(L, -1));
		break;
	case LUA_ERRGCMM:
		log_error("Garbage collector error: %s", lua_tostring(L, -1));
		break;
	case LUA_ERRERR:
		log_error("Error calling traceback function: %s", lua_tostring(L, -1));
		break;
	default:
		log_error("Unknown state: %d", ret);
		lua_pushstring(L, "Unknown state");
		lua_error(L);
	}
	if (ret != 0) {
		lua_pop(L, -1); /* Remove error message */
	}
	return ret;
}

void lua_stackdump (lua_State *L) {
  int i;

  int top = lua_gettop(L);
  printf("-- Stack-dump:");
  for (i = 1; i <= top; i++) {  /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {
      case LUA_TSTRING:  /* strings */
        printf("`%s'", lua_tostring(L, i));
        break;

      case LUA_TBOOLEAN:  /* booleans */
        printf(lua_toboolean(L, i) ? "true" : "false");
        break;

      case LUA_TNUMBER:  /* numbers */
        printf("%g", lua_tonumber(L, i));
        break;

      default:  /* other values */
        printf("%s", lua_typename(L, t));
        break;

    }
    printf(", ");  /* put a separator */
  }
  printf("\n");  /* end the listing */
  fflush(stdout);
}
