/*
 * aesdsocket.h
 *
 * Created on: September 26, 2026
 * Author: Ruben Reyes Moreno
 */

#ifndef AESDSOCKET_H_
#define AESDSOCKET_H_

/* ---Include libraries and headers---*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fs.h>

/* ---defines--- */

/*syslog prints*/
#define DEBUG_LOG(msg,...) syslog(LOG_DEBUG, "Server [%s:%d] " msg, __func__, \
		__LINE__, ##__VA_ARGS__)
#define ERROR_LOG(msg,...) syslog(LOG_ERR, "Server ERR [%s:%d] " msg,  __func__, \
		__LINE__, ##__VA_ARGS__)

/* ---function prototypes--- */
static void signal_handler(int signo);


#endif /*AESDSOCKET_H_*/

