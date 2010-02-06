/*
 * file_util.c
 *
 *  Created on: Feb 6, 2010
 *      Author: jalp
 */
#include "file_util.h"
#include <util/log.h>

int send_file(FILE *src, FILE *dst) {
	const size_t buf_size = 1024;
	char buffer[buf_size];

	int r_bytes, w_bytes;
	while ((r_bytes = fread(buffer, 1, buf_size, src)) > 0) {
		w_bytes = fwrite(buffer, 1, r_bytes, dst);
		if (w_bytes < r_bytes) {
			log_debug("Failed to send all bytes.");
			break;
		}
	}

	return w_bytes;
}
