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

typedef struct mailbox mailbox_t;

typedef struct mailbox_ref {
	mailbox_t *mailbox;
} mailbox_ref_t;

mailbox_t *mailbox_init(lua_State *L);

mailbox_t *mailbox_get(lua_State *L);

void mailbox_destroy(mailbox_t *mailbox);

void mailbox_send(mailbox_t *mailbox, void *message);

void *mailbox_receive(mailbox_t *mailbox);

#endif /* MAILBOX_H_ */
