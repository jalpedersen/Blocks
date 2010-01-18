/*
 * mailbox.h
 *
 *  Created on: Jan 3, 2010
 *      Author: jalp
 */

#ifndef MAILBOX_H_
#define MAILBOX_H_
#include <lua.h>

#define MAILBOX_TYPE_NAME "blocks.mailbox"
#define MAILBOX_REF_TYPE_NAME "blocks.mailbox-ref"

#define MESSAGE_TYPE_NAME "blocks.message"
#define MESSAGE_REF_TYPE_NAME "blocks.message-ref"

typedef struct task task_t;

typedef struct mailbox mailbox_t;

typedef struct message message_t;

typedef struct message_content {
    size_t msg_size;
	void *message;
} message_content_t;

typedef struct mailbox_ref {
	mailbox_t *mailbox;
} mailbox_ref_t;

typedef struct message_ref {
        message_t *message;
} message_ref_t;

mailbox_t *mailbox_init(lua_State *L);

mailbox_ref_t *mailbox_get(lua_State *L);

void mailbox_destroy(mailbox_ref_t *mailbox);

void mailbox_message_destroy(message_t *message);

void mailbox_set_task(mailbox_t *mailbox, task_t *task);

message_t *mailbox_send(mailbox_ref_t *sender, mailbox_ref_t *recipient,
				        message_content_t *content);

message_t *mailbox_receive(mailbox_ref_t *mailbox);

message_t *mailbox_peek(mailbox_ref_t *mailbox_ref);

message_t *mailbox_wait_for_reply(message_t *message, int timeout);

int mailbox_message_reply(message_t *message);

message_content_t *mailbox_message_get_content(message_t *message);

#endif /* MAILBOX_H_ */
