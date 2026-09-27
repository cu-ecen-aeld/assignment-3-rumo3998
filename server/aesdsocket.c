/*
 * aesdsocket.c
 *
 * Created on: September 26, 2026
 * Author: Ruben Reyes Moreno
 */

/* ---Include libraries and headers---*/
#include "aesdsocket.h"

/* ---Defines---*/
#define PORT "9000" /*stream socket port*/
#define ERR  (-1)   /*return err code*/

int main(void){

	int rc;                          /*return code, for validation*/
	struct addrinfo hints;           /*relevant info*/
	struct addrinfo *server_info;    /*points to results*/
	struct addrinfo *next;           /*next addrinfo in linked list*/
	int listen_fd, conn_fd;          /*file descr. for listen and connect*/
	int yes = 1;                     /*option value for setsockopt()*/

	/*create a hint struct to help populate addrinfo*/
	memset(&hints, 0, sizeof(hints)); /*clear hints */
	hints.ai_flags = AI_PASSIVE;      /*auto populate ip addrinfo*/
	hints.ai_family = AF_INET;        /*want ipv4*/
	hints.ai_socktype = SOCK_STREAM;  /*TCP SOCK*/

	/*try to populate addrinfo*/
	if((rc = getaddrinfo(NULL, PORT, &hints, &server_info)) != 0){
		ERROR_LOG("Err: addrinfo population");
		return ERR;
	}

	/*if succeeded server_info points to ll of struct addrinfo
	 * now try to iterate through linked list and try to bind socket*/
	for(next = server_info; next != NULL; next = next->ai_next){
		
		/*current node does not equal  NULL, so try to get its file descr*/
		listen_fd = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
		if(listen_fd == ERR){
			ERROR_LOG("socket returned -1");
			continue;					
		}

		/*set options to avoid socket in use err, do this prior to  binding*/
		rc = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		if(rc == ERR){
			ERROR_LOG("socket opt returned -1");
			close(listen_fd); /*close fd to prevent leak*/
			continue;
		}

		/*now try to bind*/
		rc = bind(listen_fd, next->ai_addr, next->ai_addrlen);
		if(rc == ERR){
			ERROR_LOG("socket bind returned -1");
			close(listen_fd); /*close fd to prevent leak*/
			continue;
		}

		/*At this point, socket fd acquisition and bind were both successful*/
		break; /*no need to continue loop*/
	}
	
	/*free the linked-list*/
	freeaddrinfo(server_info);

	/*check to see if the next ptr is a valid addrinfo struct*/
	if(next == NULL){
		ERROR_LOG("socket connection was unsuccessful");
		return ERR; /*return -1 on fail*/
	}

	/*Test to see if compile without cross compile*/
	printf("Hello\n");
}
