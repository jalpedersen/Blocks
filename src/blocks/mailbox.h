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

typedef struct mailbox mailbox_t;

typedef struct message message_t;

typedef struct mailbox_ref {
	mailbox_t *mailbox;
} mailbox_ref_t;

typedef struct message_ref {
        message_t *message;
} message_ref_t;

mailbox_t *mailbox_init(lua_State *L);

mailbox_ref_t *mailbox_get(lua_State *L);

void mailbox_destroy(mailbox_ref_t *mailbox);

/**
   return 1 on success, 0 on failure
 */
int mailbox_send(mailbox_ref_t *mailbox, void *message);

message_t *mailbox_receive(mailbox_ref_t *mailbox);

#endif /* MAILBOX_H_ */
