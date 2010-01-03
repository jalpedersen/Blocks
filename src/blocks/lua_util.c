/*
 * lua_util.c
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#include "lua_util.h"
#include "log.h"

static int traceback (lua_State *L) {
	if ( ! lua_isstring(L, 1)) {
		return 1; /* In case 'message' is not a string */
	}
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  /* pass error message */
	lua_pushinteger(L, 2);  /* skip this function and traceback */
	lua_call(L, 2, 1);  /* call debug.traceback */
	return 1;
}
int lua_eval(lua_State *L) {
	return lua_eval_part(L,
						 lua_gettop(L)-1, /* don't include function */
						 LUA_MULTRET);
}
int lua_eval_part(lua_State *L, int narg, int nres) {
	int ret, error_index;

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
	case LUA_ERRMEM:
		log_error("Memory allocation error: %s", lua_tostring(L, -1));
		break;
	case LUA_ERRERR:
		log_error("Error calling traceback function: %s", lua_tostring(L, -1));
		break;
	default:
		log_error("Unknown state: %d", ret);
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
