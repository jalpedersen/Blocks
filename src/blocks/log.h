/*
 * log.h
 *
 *  Created on: Jan 2, 2010
 *      Author: jalp
 */

#ifndef LOG_H_
#define LOG_H_
#include <stdio.h>

#define __log(file,...){fprintf(file,  "[%s:%d]: ", __FILE__, __LINE__); fprintf(file, __VA_ARGS__);fprintf(file, "\n");fflush(file);}

#define log_debug(...){__log(stdout, __VA_ARGS__);}

#define log_error(...){__log(stderr, __VA_ARGS__);}


#endif /* LOG_H_ */
