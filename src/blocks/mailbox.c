/*
 * mailbox.c
 *
 *  Created on: Jan 3, 2010
 *      Author: jalp
 */
#include <pthread.h>
#include <stdlib.h>
#include "mailbox.h"
#include "process.h"
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
	message_content_t *content;
	mailbox_ref_t *sender;
	mailbox_ref_t *mailbox;
	struct message *next;
    enum message_state state;
    int ref_count;
    pthread_mutex_t mutex;
    pthread_cond_t ready;
};
enum mailbox_state {
	MBOX_ALIVE,
	MBOX_DEAD
};

struct mailbox {
	pthread_mutex_t mutex;
	pthread_cond_t new_message;
	message_t *head;
	message_t *end;
	enum mailbox_state state;
	task_t *task;
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
	mailbox->state = MBOX_ALIVE;
	mailbox->head = NULL;
	mailbox->end = NULL;
	mailbox->task = NULL;

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
	if (mailbox->state == MBOX_DEAD) {
		do_cleanup = 1;
	} else {
		mailbox->state = MBOX_DEAD;
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

message_t *mailbox_send(mailbox_ref_t *sender, mailbox_ref_t *recipient, message_content_t *content) {
	message_t *msg;
	mailbox_t *rcpt;
	rcpt = recipient->mailbox;

	if (rcpt->state == MBOX_DEAD) {
		return NULL;
	}
	msg = malloc(sizeof(message_t));
	pthread_mutex_init(&msg->mutex, NULL);
	pthread_cond_init(&msg->ready, NULL);
	msg->content = content;
	msg->state = MSG_WAITING;
	msg->ref_count = 2; /*One for sender, one for recipient */
	msg->mailbox = recipient;
	msg->sender = sender;
	msg->next = NULL;

	pthread_mutex_lock(&rcpt->mutex);
	if (rcpt->head == NULL) {
		rcpt->head = msg;
		rcpt->end = msg;
	} else {
		rcpt->end->next = msg;
		rcpt->end = msg;
	}
	pthread_mutex_unlock(&rcpt->mutex);
	process_notify_task(rcpt->task);
	return msg;
}

int mailbox_message_reply(message_t *message) {
	pthread_mutex_lock(&message->mutex);
	message->state = MSG_READY;
	pthread_cond_signal(&message->ready);
	pthread_mutex_unlock(&message->mutex);

	return 0;
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

message_t *mailbox_peek(mailbox_ref_t *mailbox_ref) {
	mailbox_t *mailbox;
	message_t *msg;

	mailbox = mailbox_ref->mailbox;
	pthread_mutex_lock(&mailbox->mutex);
	msg = mailbox->head;
	pthread_mutex_unlock(&mailbox->mutex);
    return msg;
}

message_t *mailbox_wait_for_reply(message_t *message, int timeout) {
	log_debug("Waiting for message in %p:", (void*)message);
	pthread_mutex_lock(&message->mutex);
	while (message->state == MSG_WAITING ||
			message->state == MSG_PROCESSING){
		pthread_cond_wait(&message->ready, &message->mutex);
	}
	return message;
	pthread_mutex_unlock(&message->mutex);

}


void mailbox_message_destroy(message_t *message) {
	pthread_mutex_lock(&message->mutex);
	message->ref_count--;
	log_debug("Message %p ref count: %d ", (void*)message, message->ref_count);
	if (message->ref_count <= 0 &&
			(message->state == MSG_READY || message->state == MSG_DEAD)) {
		log_debug("Destroying message %p", (void*)message);
		pthread_mutex_unlock(&message->mutex);
		pthread_cond_destroy(&message->ready);
		pthread_mutex_destroy(&message->mutex);
		free(message);
	} else {
		pthread_mutex_unlock(&message->mutex);
	}
}

void mailbox_set_task(mailbox_t *mailbox, task_t *task) {
	mailbox->task = task;
}

message_content_t *mailbox_message_get_content(message_t *message) {
	return message->content;
}
