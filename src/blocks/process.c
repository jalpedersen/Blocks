/*
 * process.c
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
#include "process.h"
#include "log.h"
#include "lua_util.h"
#include "blocks.h"

struct task {
	lua_State *L;
	short is_loaded;
	struct task *next;
};

struct thread_list {
	pthread_t *thread;
	struct thread_list *next;
};
struct thread_pool {
	int size;
	pthread_cond_t new_job;
	pthread_mutex_t mutex;
	struct thread_list *head;
	struct thread_list *end;
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

static void task_destroy(struct task *t) {
	mailbox_destroy(mailbox_get(t->L));
	lua_close(t->L);
	free(t);

}

static void *worker(void *args) {
	lua_State *L;
	struct task *t;
	struct thread_pool *pool = args;

	while (1) {
		pthread_mutex_lock(&pool->mutex);
		t = pop_task(pool);
		while (t == NULL) {
			pthread_cond_wait(&pool->new_job, &pool->mutex);
			t = pop_task(pool);
		}
		pthread_mutex_unlock(&pool->mutex);
		if (t != NULL) {
			L = t->L;
			if ( ! t->is_loaded) {
				/* Evaluate function that evaluates the real function */
				lua_pushvalue(L, 1);
				/* Load function from string dump */
				lua_eval_part(L, 0, 1);
				/* Replace loader function with real function */
				lua_replace(L, 1);
				t->is_loaded = 1;
				/* Evaluate real function */
				lua_eval(L);
			} else {
				/* Pop message from queue and pass the values to the
				 * function left on the stack by the previous evaluation */
				lua_eval(L);
			}

			/* If function hasn't left anything on the
			 * stack it is done and we close it's state */
			if (lua_gettop(L) == 0) {
				task_destroy(t);
			} else {
				/* if a function is returned, task goes back
				 * in queue and waits for the new function to
				 * be evaluated. */
				if (lua_isfunction(L, 1)) {
					pthread_mutex_lock(&pool->mutex);
					push_task(pool, t);
					pthread_cond_signal(&pool->new_job);
					pthread_mutex_unlock(&pool->mutex);
				} else {
					lua_stackdump(L);
					task_destroy(t);
				}
			}
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
  luaL_Buffer b;
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
	size_t len;
	const char *str;

	if ( ! lua_isfunction(src, 1)) {
		luaL_error(src, "Argument 1 must be a function. Was %s", lua_typename(src, 1));
	}

	src_top = lua_gettop(src);
	for (i = 1; i <= src_top; i++) {
		type = lua_type(src, i);
		switch (type) {
		case LUA_TFUNCTION:
			copy_function(src, dst, i);
			break;
		case LUA_TNUMBER:
			lua_pushnumber(dst, lua_tonumber(src, i));
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean(dst, lua_toboolean(src, i));
			break;
		case LUA_TNIL:
			lua_pushnil(dst);
			break;
		case LUA_TNONE:
			break;
		case LUA_TSTRING:
			len = lua_objlen(src, i);
			str = lua_tolstring(src, i, &len);
			lua_pushlstring(dst, str, len);
			break;
		default:
			lua_pushnil(dst);
			log_debug("type: %s", lua_typename(src, type));
			break;
		}
	}
}

mailbox_t *process_spawn(struct thread_pool *pool, lua_State *parent) {
	mailbox_t *mailbox;
	struct task *task = malloc(sizeof(struct task));

	task->L = luaL_newstate();
	task->is_loaded = 0;
	luaL_openlibs(task->L);
	luaopen_blocks(task->L);
	mailbox = mailbox_get(task->L)->mailbox;

	/* Copy values from parent */
	copy_stack(parent, task->L);

	pthread_mutex_lock(&pool->mutex);
	push_task(pool, task);
	pthread_cond_signal(&pool->new_job);
	pthread_mutex_unlock(&pool->mutex);

	/* Remove spawned function and arguments from parent stack */
	lua_settop(parent, 0);

	/* Return mail box created in luaopen_blocks function */
	return mailbox;
}

thread_pool_t *threadpool_init(lua_State *L, int size) {
	struct thread_pool *pool = malloc(sizeof(struct thread_pool));
	pthread_mutex_init(&pool->mutex, NULL);
	pthread_cond_init(&pool->new_job, NULL);
	pool->task_queue = NULL;
	pool->head = NULL;
	pool->end = NULL;
	threadpool_extend(pool, size);
	return pool;
}

void threadpool_extend(thread_pool_t *pool, int size) {
	int i;
	pthread_t *thread;
	pthread_attr_t thread_attr;
	struct thread_list *prev_thread, *thread_item;

	/* Default stack size is 512k */
	pthread_attr_init(&thread_attr);
	pthread_attr_setstacksize(&thread_attr, 512 * 1024);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	prev_thread = pool->end;
	for (i = 0; i < size; i++) {
		thread = malloc(sizeof(pthread_t));
		thread_item = malloc(sizeof(struct thread_list));
		thread_item->thread = thread;
		thread_item->next = NULL;
		if (pool->head == NULL) {
			pool->head = thread_item;
		} else {
			prev_thread->next = thread_item;
		}
		prev_thread = thread_item;
		pthread_create(thread, &thread_attr, worker, pool);
	}
	pool->end = prev_thread;

}
