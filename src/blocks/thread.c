/*
 * thread.c
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "thread.h"
#include "log.h"
#include "lua_util.h"
#include "blocks.h"

struct message {
	struct message *next;
};
struct task {
	lua_State *L;
	struct task *next;
	struct message *message;
};

struct thread_pool {
	int size;
	pthread_cond_t new_job;
	pthread_mutex_t mutex;
	pthread_t **threads;
	struct task *task_queue;
};


/**
 * Thread pool must be locked before calling get_task
 */
static struct task *pop_task(thread_pool_t *pool) {
	struct task *t = pool->task_queue;
	if (t != NULL) {
		pool->task_queue = t->next;
	}
	return t;
}

static struct task *push_task(thread_pool_t *pool, struct task *task) {
	struct task *head = pool->task_queue;
	task->next = NULL;
	if (head == NULL) {
		pool->task_queue = task;
	} else {
		while (head->next != NULL) {
			head = head->next;
		}
		head->next = task;
	}
	return head;
}

static void *worker(void *args) {
	struct task *t;
	struct thread_pool *pool = args;

	pthread_detach(pthread_self());
	while (1) {
		pthread_mutex_lock(&pool->mutex);
		t = pop_task(pool);
		while (t == NULL) {
			pthread_cond_wait(&pool->new_job, &pool->mutex);
			t = pop_task(pool);
		}
		pthread_mutex_unlock(&pool->mutex);
		log_debug("Got new job (0x%x)", (int)pthread_self());
		if (t != NULL) {
			/* Evaluate function that evaluates the real function */
			lua_pushvalue(t->L, 1);
			/* Load function from string dump */
			lua_eval_part(t->L, 0, 1);
			/* Replace loader function with real function */
			lua_replace(t->L, 1);
			/* Evaluate real function */
			lua_eval(t->L);
			lua_close(t->L);
			free(t);
		}
	}
	return NULL;
}

static int load_string(lua_State *L) {
	int ret;
	size_t fn_dump_size;
	const char *fn_dump;

	fn_dump_size = lua_tointeger(L, lua_upvalueindex(1));
	fn_dump = lua_tolstring(L, lua_upvalueindex(2), &fn_dump_size);

	ret = luaL_loadbuffer(L, fn_dump, fn_dump_size, "Chunk");
	switch (ret) {
	case 0:
		break;
	case LUA_ERRMEM:
	case LUA_ERRSYNTAX:
		luaL_error(L, lua_tostring(L, -1));
	default:
		luaL_error(L, "Unexpected state: %d", ret);
	}

	return 1;
}

static int function_writer (lua_State *L, const void* b, size_t size, void* B) {
	luaL_addlstring((luaL_Buffer*) B, (const char *)b, size);
	return 0;
}

static int copy_function (lua_State *src, lua_State *dst, int index) {
  luaL_Buffer b, b2;
  const char *fn_dump;
  size_t fn_size;
  lua_pushvalue(src, index);
  luaL_checktype(src, index, LUA_TFUNCTION);
  luaL_buffinit(src, &b);
  if (lua_dump(src, function_writer, &b) != 0) {
    luaL_error(src, "Could not dump function");
  }
  luaL_pushresult(&b);
  fn_size = lua_objlen(src, -1);
  fn_dump = lua_tolstring(src, -1, &fn_size);
  lua_pushinteger(dst, fn_size);
  lua_pushlstring(dst, fn_dump, fn_size);
  lua_pop(src, 2); /* Remove string and copied function */
  lua_pushcclosure(dst, load_string, 2);
  return 1;
}

static void copy_stack(lua_State *src, lua_State *dst) {
	int src_top, i, type;
	src_top = lua_gettop(src);
	if ( ! lua_isfunction(src, 1)) {
		luaL_error(src, "Argument 1 must be a function. Was %s", lua_typename(src, 1));
	}
	for (i = 1; i <= src_top; i++) {
		type = lua_type(src, i);
		log_debug("Copying %s (%d)", lua_typename(src, type), i);
		switch (type) {
		case LUA_TFUNCTION:
			copy_function(src, dst, i);
			break;
		case LUA_TTHREAD:
			lua_pushnil(dst);
			break;
		case LUA_TNUMBER:
			lua_pushnumber(dst, lua_tonumber(src, i));
			break;
		case LUA_TTABLE:
			lua_pushnil(dst);
			break;
		case LUA_TBOOLEAN:
			break;
		case LUA_TNIL:
		default:
			lua_pushnil(dst);
			log_debug("type: %s", lua_typename(src, type));
			break;
		}
	}
}

int spawn(struct thread_pool *pool, lua_State *parent) {
	struct task *task = malloc(sizeof(struct task));
	pthread_mutex_lock(&pool->mutex);

	task->L = luaL_newstate();
	luaL_openlibs(task->L);
	luaopen_blocks(task->L);

	/* Copy values from parent */
	copy_stack(parent, task->L);

	push_task(pool, task);

	pthread_cond_signal(&pool->new_job);
	pthread_mutex_unlock(&pool->mutex);
	return 1;
}

thread_pool_t *threads_init(lua_State *L, int size) {
	int i;
	struct thread_pool *pool = malloc(sizeof(struct thread_pool));
	pthread_mutex_init(&pool->mutex, NULL);
	pthread_cond_init(&pool->new_job, NULL);
	pool->threads = malloc(sizeof(pthread_t*) * size);
	pool->task_queue = NULL;
	for (i = 0; i < size; i++) {
		pool->threads[i] = malloc(sizeof(pthread_t));
		pthread_create(pool->threads[i], NULL, worker, pool);
	}
	return pool;
}
