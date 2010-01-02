/*
 * thread.h
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#ifndef THREAD_H_
#define THREAD_H_

typedef struct thread_pool thread_pool_t;

thread_pool_t *threads_init(lua_State *L);

int spawn(struct thread_pool *pool, lua_State *L);

#endif /* THREAD_H_ */
