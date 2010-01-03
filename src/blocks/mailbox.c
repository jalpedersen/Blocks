/*
 * mailbox.c
 *
 *  Created on: Jan 3, 2010
 *      Author: jalp
 */

#include "mailbox.h"
#include <pthread.h>

const char *mailbox_key = "M";

struct message {
	void *message;
	struct message *head;
	struct message *tail;
};

struct mailbox {
	pthread_mutex_t mutex;
	pthread_cond_t new_message;
	struct message *message;
};

mailbox_t *mailbox_init(lua_State *L) {
	struct mailbox *mailbox;
	mailbox = lua_newuserdata(L, sizeof(struct mailbox));
	pthread_cond_init(&mailbox->new_message, NULL);
	pthread_mutex_init(&mailbox->mutex, NULL);
	mailbox->message = NULL;

	lua_pushlightuserdata(L, &mailbox_key);
	lua_settable(L, LUA_REGISTRYINDEX);

	return mailbox;
}

mailbox_t *mailbox_get(lua_State *L) {
	mailbox_t *mailbox;
	lua_pushlightuserdata(L, &mailbox_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	mailbox = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return mailbox;
}

void mailbox_destroy(mailbox_t *mailbox) {
	pthread_cond_destroy(&mailbox->new_message);
	pthread_mutex_destroy(&mailbox->mutex);
}

void mailbox_send(mailbox_t *mailbox, void *message);

void *mailbox_receive(mailbox_t *mailbox);

