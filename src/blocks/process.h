/*
 * process.h
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#ifndef PROCESS_H_
#define PROCESS_H_
#include "mailbox.h"

typedef struct thread_pool thread_pool_t;

thread_pool_t *threadpool_init(lua_State *L, int size);

void threadpool_extend(thread_pool_t *pool, int size);

mailbox_t *process_spawn(struct thread_pool *pool, lua_State *L);

#endif /* PROCESS_H_ */
