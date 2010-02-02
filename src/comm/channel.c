/*
 * channel.c
 *
 *  Created on: Feb 1, 2010
 *      Author: jalp
 */
#include "messagebus.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <memory.h>
#include <util/log.h>

enum addr_type {
	ADDR_UNIX,
	ADDR_INET
};
struct mb_channel {
	int sd;
	enum addr_type type;
	const char *name;
	union {
		struct sockaddr_un un;
		struct sockaddr_in in;
	} addr;
};


static int set_non_blocking(int sd, int strict) {
	int option_value = 1;
	int ret;
	ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &option_value,
			sizeof(option_value));
	if (ret < 0) {
		if (strict) {
			log_perror("setsockopt failed");
		}
		close(sd);
		return -1;
	}
	ret = ioctl(sd, FIONBIO, (char *) &option_value);
	if (ret < 0) {
		if (strict) {
			log_perror("ioctl failed");
		}
		close(sd);
		return -1;
	}
	return 1;

}

mb_channel_t *mb_channel_bind_path(const char *path) {
	int ret;
	mb_channel_t *channel;

	channel = (mb_channel_t*) malloc(sizeof(mb_channel_t));
	channel->type = ADDR_UNIX;
	channel->sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (channel->sd < 0) {
		log_perror("socket failed");
		return NULL;
	}
	/* Setup the socket to be non-blocking */
	ret = set_non_blocking(channel->sd, 1);
	if (ret < 0) {
		/*error message already printed by previous function*/
		return NULL;
	}

	memset(&channel->addr.un, 0, sizeof(channel->addr.un));
	channel->addr.un.sun_family = AF_UNIX;
	strcpy(channel->addr.un.sun_path, path);
	channel->name = path;

	ret = connect(channel->sd, (struct sockaddr *) &channel->addr.un,
			sizeof(channel->addr.un));
	if (ret >= 0) {
		printf("Socket %s busy\n", path);
		return NULL;
	}
	unlink(path);
	ret = bind(channel->sd, (struct sockaddr *) &channel->addr.un,
			sizeof(channel->addr.un));
	if (ret < 0) {
		log_perror("bind failed");
		close(channel->sd);
		return NULL;
	}

	ret = listen(channel->sd, CHANNEL_BACKLOG);
	if (ret < 0) {
		log_perror("listen failed");
		close(channel->sd);
		return NULL;
	}
	return channel;
}

mb_channel_t *mb_channel_bind_port(int port) {
	int ret;
	mb_channel_t *channel;

	channel = (mb_channel_t*) malloc(sizeof(mb_channel_t));
	channel->type = ADDR_INET;
	channel->sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (channel->sd < 0) {
		log_perror("socket failed");
		return NULL;
	}
	/* Setup the socket to be non-blocking */
	ret = set_non_blocking(channel->sd, 1);
	if (ret < 0) {
		/*error message already printed by previous function*/
		return NULL;
	}

	memset(&channel->addr.in, 0, sizeof(channel->addr.in));
	channel->addr.in.sin_family = AF_INET;
	channel->addr.in.sin_port = htons(port);
	channel->addr.in.sin_addr.s_addr = htonl(INADDR_ANY);
	channel->name = "tcp";

	ret = bind(channel->sd, (struct sockaddr *) &channel->addr.in,
			sizeof(channel->addr.in));
	if (ret < 0) {
		log_perror("bind failed");
		close(channel->sd);
		return NULL;
	}

	ret = listen(channel->sd, CHANNEL_BACKLOG);
	if (ret < 0) {
		log_perror("listen failed");
		close(channel->sd);
		return NULL;
	}
	return channel;
}

int mb_channel_destroy(mb_channel_t *channel) {
	close(channel->sd);
	free(channel);
	return 0;
}

int mb_channel_receive(mb_channel_t *channel, mb_message_cb_t cb) {
	fd_set master_set, working_set;
	int ready_count;
	int sd, new_sd;
	int max_sd, ret;
	int is_conn_active;
	int position;
	struct timeval timeout;
	char buffer[CHANNEL_BUFFER];

	timeout.tv_sec = 30; /* 30 sec. timeout*/
	timeout.tv_usec = 0;

	FD_ZERO(&master_set);
	max_sd = channel->sd;
	FD_SET(channel->sd, &master_set);

	while (1) {
		memcpy(&working_set, &master_set, sizeof(master_set));
		ret = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
		if (ret < 0) {
			log_perror("select failed");
			return -1;
		}
		if (ret == 0) {
			/* Time-out. Reset connections. */
			break;
		}
		ready_count = ret;
		for (sd = 0; sd <= max_sd && ready_count > 0; sd++) {
			if (FD_ISSET(sd, &working_set)) {
				ready_count -= 1;
				if (sd == channel->sd) {
					new_sd = 0;
					while (new_sd != -1) {
						new_sd = accept(channel->sd, NULL, NULL);
						if (new_sd < 0) {
							if (errno != EWOULDBLOCK) {
								log_perror("accept failed");
							}
							break;
						}
						FD_SET(new_sd, &master_set);
						if (new_sd > max_sd) {
							max_sd = new_sd;
						}
					}
				} else {
					is_conn_active = 1;
					position = 0;
					while (1) {
						int msg_len;
						memset(buffer, 0, sizeof(buffer));
						set_non_blocking(sd, 0);
						ret = recv(sd, buffer, sizeof(buffer), 0);
						if (ret < 0) {
							if (errno == EBADF) {
								/* Socket was closed */
								is_conn_active = 0;
							}else if (errno != EWOULDBLOCK) {
								log_perror("receive failed");
								is_conn_active = 0;
							}
							break;
						}
						if (ret == 0) { /*Connection was closed */
							is_conn_active = 0;
							break;
						}
						msg_len = ret;
						ret = cb(sd, msg_len, position, buffer);
						if (ret < 0) {
							log_perror("callback failed");
							is_conn_active = 0;
							break;
						}
						position += msg_len;
					}
					if (!is_conn_active) {
						close(sd);
						FD_CLR(sd, &master_set);
						if (sd == max_sd) {
							while (FD_ISSET(max_sd, &master_set) == 0)
								max_sd -= 1;
						}
					}
				}
			}
		}
	}
	/*Close all in-active connections*/
	for (sd = 0; sd <= max_sd; sd++) {
		if (sd == channel->sd) {
			continue; /*Do not close server socket*/
		}
		if (FD_ISSET(sd, &master_set)) {
			close(sd);
		}
	}
	return 0;
}

static mb_channel_t *mb_channel_reopen_unix(mb_channel_t *channel) {
	int ret;
	channel->sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (channel->sd < 0) {
		log_perror("socket");
		mb_channel_destroy(channel);
		return NULL;
	}
	memset(&channel->addr, 0, sizeof(channel->addr.un));
	channel->addr.un.sun_family = AF_UNIX;
	strcpy(channel->addr.un.sun_path, channel->name);

	ret = connect(channel->sd, (struct sockaddr *) &channel->addr.un,
			sizeof(channel->addr.un));
	if (ret < 0) {
		log_perror("connect");
		mb_channel_destroy(channel);
		return NULL;
	}
	return channel;
}

mb_channel_t *mb_channel_open_path(const char *path) {
	mb_channel_t *channel = (mb_channel_t*) malloc(sizeof(mb_channel_t));
	channel->name = path;
	return mb_channel_reopen_unix(channel);
}

int mb_chanel_send(mb_channel_t *channel, size_t size, void *data,
		void **reply) {
	int msg_len, i;
	char recv_buffer[CHANNEL_BUFFER];
	for (i = 0; i < 2; i++) {
		msg_len = send(channel->sd, data, size, 0);
		if (msg_len != size) {
			mb_channel_reopen_unix(channel);
		} else {
			break;
		}
	}
	if (msg_len != size) {
		mb_channel_destroy(channel);
		log_perror("send");
		return -1;

	}
	msg_len = recv(channel->sd, recv_buffer, sizeof(recv_buffer), 0);
	*reply = (void*) malloc(msg_len);
	memcpy(*reply, recv_buffer, msg_len);
	return msg_len;
}
