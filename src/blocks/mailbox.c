/*
 * mailbox.c
 *
 *  Created on: Jan 3, 2010
 *      Author: jalp
 */
#include <pthread.h>
#include <stdlib.h>
#include "mailbox.h"
#include <util/lua_util.h>
#include <util/log.h>

const char mailbox_key = 'M';

enum message_state {
        MSG_WAITING,
        MSG_PROCESSING,
        MSG_READY,
        MSG_DEAD
};

struct message {
	mailbox_ref_t *sender;
	mailbox_ref_t *mailbox;
    size_t msg_size;
	void *message;
	struct message *next;
    enum message_state state;
    int ref_count;
};
enum mailbox_state {
	ALIVE,
	DEAD
};

struct mailbox {
	pthread_mutex_t mutex;
	pthread_cond_t new_message;
	struct message *head;
	struct message *end;
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
	mailbox->head = NULL;
	mailbox->end = NULL;

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

message_t *mailbox_send(mailbox_ref_t *sender, mailbox_ref_t *recepient, void *message, size_t message_size) {
	message_t *msg;
	mailbox_t *rcpt;
	msg = malloc(sizeof(message_t));
	msg->message = message;
	msg->state=MSG_WAITING;
	msg->msg_size = message_size;
	msg->ref_count = 1;
	msg->mailbox = recepient;
	msg->sender = sender;
	rcpt = recepient->mailbox;
	pthread_mutex_lock(&rcpt->mutex);
	if (rcpt->head == NULL) {
		rcpt->head = msg;
		rcpt->end = msg;
	} else {
		rcpt->end->next = msg;
		rcpt->end = msg;
	}
	pthread_mutex_unlock(&rcpt->mutex);
	return msg;
}

message_t *mailbox_receive(mailbox_ref_t *mailbox_ref) {
	mailbox_t *mailbox;
	message_t *msg, *msg_head;

	mailbox = mailbox_ref->mailbox;
	pthread_mutex_lock(&mailbox->mutex);
	msg = mailbox->head;
	if (msg != NULL) {
		mailbox->head = msg->next;
	}
	pthread_mutex_unlock(&mailbox->mutex);
    return msg;
}

void mailbox_message_destroy(message_ref_t *mailbox) {
	log_debug("Destroying mailbox %p", (void*)mailbox);
}
