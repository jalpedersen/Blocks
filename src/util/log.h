/*
 * log.h
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#ifndef LOG_H_
#define LOG_H_
#include <stdio.h>
#include <string.h>

#define __log(file,level,...){fprintf(file,  "[%s: %s:%d]: ", level, __FILE__, __LINE__); fprintf(file, __VA_ARGS__);fprintf(file, "\n");fflush(file);}

#define log_debug(...){__log(stdout, "DEBUG", __VA_ARGS__);}

#define log_info(...){__log(stdout, "INFO", __VA_ARGS__);}

#define log_warn(...){__log(stdout, "WARN", __VA_ARGS__);}

#define log_error(...){__log(stderr, "ERROR", __VA_ARGS__);}

#define log_perror(...){fprintf(stderr, "(%s) ", strerror(errno));__log(stderr, "ERROR", __VA_ARGS__);}

#endif /* LOG_H_ */
