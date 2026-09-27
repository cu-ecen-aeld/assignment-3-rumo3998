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
	struct addrinfo *next_node;      /*next addrinfo in linked list*/
	int listen_fd, conn_fd;          /*file descr. for listen and connect*/

	/*create a hint struct to help populate addrinfo*/
	memset(&hints, 0, sizeof(hints)); /*clear hints */
	hints.ai_flags = AI_PASSIVE;     /*auto populate ip addrinfo*/
	hints.ai_family = AF_INET;       /*want ipv4*/
	hints.ai_socktype = SOCK_STREAM; /*TCP SOCK*/

	/*try to populate addrinfo*/
	if((rc = getaddrinfo(NULL, PORT, &hints, &server_info)) != 0){
		ERROR_LOG("Err: addrinfo population");
		return ERR;
	}

	/*if succeeded server_info points to ll of struct addrinfo*/


	/*free the linked-list*/
	freeaddrinfo(server_info);

	/*Test to see if compile without cross compile*/
	printf("Hello\n");
}
