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
#include <lauxlib.h>
#include "thread.h"
#include "log.h"
#include "lua_util.h"

const char *jobs_table_key = "J";

struct task {
	lua_State *parent;
	lua_State *L;

	struct task *next;
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
static void save_table(lua_State *L, void *thread) {
	lua_pushlightuserdata(L, &jobs_table_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, thread);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	/* Remove thread and table from stack */
	lua_pop(L, 2);
}

static void *worker(void *args) {
	lua_State *L;
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
			log_debug("Executing thread (0x%x)", (int)pthread_self());
			lua_eval(t->L);
			lua_pushnil(t->L);
			save_table(t->L, t->L);
			free(t);
		} else {
			log_debug("No new task");
		}
	}
	return NULL;
}


int spawn(struct thread_pool *pool, lua_State *parent) {
	struct task *task = malloc(sizeof(struct task));
	pthread_mutex_lock(&pool->mutex);
	log_debug("New job");

	task->parent = parent;
	task->L = lua_newthread(parent);

	save_table(parent, task->L);

	/* Copy values from parent */
	lua_xmove(parent, task->L, lua_gettop(parent));
	push_task(pool, task);
	lua_gc(parent, LUA_GCRESTART, 0);

	pthread_cond_signal(&pool->new_job);
	pthread_mutex_unlock(&pool->mutex);
	return 1;
}

thread_pool_t *threads_init(lua_State *L) {
	int i;
	struct thread_pool *pool = malloc(sizeof(struct thread_pool));
	pthread_mutex_init(&pool->mutex, NULL);
	pthread_cond_init(&pool->new_job, NULL);
	pool->threads = malloc(sizeof(pthread_t*) * 5);
	pool->task_queue = NULL;
	for (i = 0; i < 5; i++) {
		pool->threads[i] = malloc(sizeof(pthread_t));
		pthread_create(pool->threads[i], NULL, worker, pool);
	}
	lua_pushlightuserdata(L, &jobs_table_key);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	return pool;
}
