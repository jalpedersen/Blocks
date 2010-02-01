/*
 * messagebus.h
 *
 *  Created on: Feb 1, 2010
 *      Author: jalp
 */

#ifndef MESSAGEBUS_H_
#define MESSAGEBUS_H_
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

typedef int (*mb_message_cb_t)(int client_sd, size_t size, void *data);


#ifndef CHANNEL_BUFFER
#define CHANNEL_BUFFER 256
#endif
#define CHANNEL_BACKLOG 32

typedef struct mb_channel mb_channel_t;

mb_channel_t *mb_channel_bind_path(const char *path);

mb_channel_t *mb_channel_bind_port(int port);

mb_channel_t *mb_channel_open_path(const char *path);

int mb_channel_destroy(mb_channel_t *channel);

int mb_channel_receive(mb_channel_t *channel, mb_message_cb_t cb);

/**
 * Remember to free reply when done
 */
int mb_channel_send(mb_channel_t *channel, size_t size, void *data, void **reply);

#endif /* MESSAGEBUS_H_ */
