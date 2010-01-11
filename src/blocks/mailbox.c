/*
 * mailbox.c
 *
 *  Created on: Jan 3, 2010
 *      Author: jalp
 */
#include <pthread.h>
#include <stdlib.h>
#include "mailbox.h"
#include "lua_util.h"
#include "../util/log.h"

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
	struct mailbox_ref *ref;

	lua_pushlightuserdata(L, (void *)&mailbox_key);
	ref = lua_newuserdata(L, sizeof(struct mailbox_ref));
	lua_settable(L, LUA_REGISTRYINDEX);
	
	mailbox = malloc(sizeof(struct mailbox));
	pthread_cond_init(&mailbox->new_message, NULL);
	pthread_mutex_init(&mailbox->mutex, NULL);
	mailbox->state = ALIVE;
	mailbox->message = NULL;

	ref->mailbox = mailbox;
	return mailbox;
}

mailbox_ref_t *mailbox_get(lua_State *L) {
	mailbox_ref_t *mailbox_ref;
	lua_pushlightuserdata(L, (void *)&mailbox_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	mailbox_ref = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return mailbox_ref;
}

void mailbox_destroy(mailbox_ref_t *ref) {
	mailbox_t *mailbox;
	mailbox = ref->mailbox;
	int do_cleanup = 0;
	pthread_mutex_lock(&mailbox->mutex);
	if (mailbox->state == DEAD) {
		do_cleanup = 1;
	} else {
		mailbox->state = DEAD;
	}
	pthread_mutex_unlock(&mailbox->mutex);
	if (do_cleanup) {
		log_debug("Destroying mailbox: %p", (void*)mailbox);
		/* Remove messages as well*/
		pthread_cond_destroy(&mailbox->new_message);
		pthread_mutex_destroy(&mailbox->mutex);
		free(mailbox);
	}
}

void mailbox_send(mailbox_ref_t *mailbox, void *message);

void *mailbox_receive(mailbox_ref_t *mailbox);

