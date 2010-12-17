/*
 * request_processor.h
 *
 *  Created on: Feb 1, 2010
 *      Author: jalp
 */

#ifndef REQUEST_PROCESSOR_H_
#define REQUEST_PROCESSOR_H_
#include <http_parser.h>
#include <lua.h>
#include "httpd_conf.h"

http_parser *request_processor_reset(http_parser *parser, int client_sd, httpd_conf_t *conf);

http_parser *request_processor_init();

void request_processor_destroy(http_parser *parser);

http_parser_settings *request_processor_settings_init();

void request_processor_settings_destroy(http_parser_settings *settings);


#endif /* REQUEST_PROCESSOR_H_ */
