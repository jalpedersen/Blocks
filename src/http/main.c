/*
 * main.c
 *
 *  Created on: Jan 11, 2010
 *      Author: jalp
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <microhttpd.h>

#define PORT 8888

int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key,
		const char *value) {
	printf("%s = %s\n", key, value);
	return MHD_YES;
}

int answer_to_connection(void *cls, struct MHD_Connection *connection,
		const char *url, const char *method, const char *version,
		const char *upload_data, size_t *upload_data_size, void **con_cls) {
	const char *page = "<html><body>Hello there!</body></html>";
	struct MHD_Response *response;
	int ret;
	printf("New request %s for %s using version %s\n", method, url, version);
	MHD_get_connection_values(connection, MHD_HEADER_KIND, print_out_key, NULL);

	response =
			MHD_create_response_from_data (
					strlen (page), (void *) page,
					MHD_NO,	MHD_NO);
	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}

int main(int argc, char **argv) {
	struct MHD_Daemon *daemon;

	daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
			&answer_to_connection, NULL, MHD_OPTION_END);
	if (NULL == daemon)
		return 1;

	getchar();

	MHD_stop_daemon(daemon);
	return 0;
}
