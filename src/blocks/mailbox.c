/*
 * mailbox.c
 *
 *  Created on: Jan 3, 2010
 *      Author: jalp
 */
#include <pthread.h>

#include "mailbox.h"
#include "lua_util.h"
#include "log.h"

const char mailbox_key = 'M';

struct message {
	void *message;
	struct message *head;
	struct message *tail;
};
enum mailbox_state {
	ALIVE,
	DEAD
};
struct mailbox {
	pthread_mutex_t mutex;
	pthread_cond_t new_message;
	struct message *message;
	enum mailbox_state state;
};

mailbox_t *mailbox_init(lua_State *L) {
	struct mailbox *mailbox;

	lua_pushlightuserdata(L, (void *)&mailbox_key);
	mailbox = lua_newuserdata(L, sizeof(struct mailbox));
	lua_settable(L, LUA_REGISTRYINDEX);

	pthread_cond_init(&mailbox->new_message, NULL);
	pthread_mutex_init(&mailbox->mutex, NULL);
	mailbox->state = ALIVE;
	mailbox->message = NULL;

	return mailbox;
}

mailbox_t *mailbox_get(lua_State *L) {
	mailbox_t *mailbox;
	lua_pushlightuserdata(L, (void *)&mailbox_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	mailbox = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return mailbox;
}

void mailbox_destroy(mailbox_t *mailbox) {
	int do_cleanup = 0;
	pthread_mutex_lock(&mailbox->mutex);
	if (mailbox->state == DEAD) {
		do_cleanup = 1;
	} else {
		mailbox->state = DEAD;
	}
	pthread_mutex_unlock(&mailbox->mutex);
	if (do_cleanup) {
		/* Remove messages as well*/
		pthread_cond_destroy(&mailbox->new_message);
		pthread_mutex_destroy(&mailbox->mutex);
	}
}

void mailbox_send(mailbox_t *mailbox, void *message);

void *mailbox_receive(mailbox_t *mailbox);

