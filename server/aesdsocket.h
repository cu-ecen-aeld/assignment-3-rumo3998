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
#include <pthread.h>
#include "queue.h"
#include <time.h>
#include <stdbool.h>

/*syslog prints*/
#define DEBUG_LOG(msg,...) syslog(LOG_DEBUG, "Server [%s:%d] " msg, __func__, \
		__LINE__, ##__VA_ARGS__)
#define ERROR_LOG(msg,...) syslog(LOG_ERR, "Server ERR [%s:%d] " msg,  __func__, \
		__LINE__, ##__VA_ARGS__)

/* ---Thread data type--- */
struct thread_s{
	pthread_t tid;                     /*thread ID*/
	volatile bool is_done;             /*thread complete*/
	int conn_fd;                       /*connection fd*/
	SLIST_ENTRY(thread_s) thread_node; /*ptr to next node in LL*/
};

/* ---create head wrapper--- */
SLIST_HEAD(thread_s_head, thread_s);

/* ---function prototypes--- */
void signal_handler(int signo);
void *connection_thread(void *arg);
void *timestamp_thread(void *arg);
void free_list(void *arg);
void clean_list(void *arg);

#endif /*AESDSOCKET_H_*/

