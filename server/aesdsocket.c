/*
 * aesdsocket.c
 *
 * Created on: September 26, 2026
 * Author: Ruben Reyes Moreno
 */

/* ---Include libraries and headers---*/
#include "aesdsocket.h"

/* ---Defines---*/
#define PORT    "9000" /*stream socket port*/
#define ERR     (-1)   /*return err code*/
#define BACKLOG (10)   /*num of allowable pending connections*/

int main(void){

	int rc;                          /*return code, for validation*/
	struct addrinfo hints;           /*relevant info*/
	struct addrinfo *server_info;    /*points to results*/
	struct addrinfo *next;           /*next addrinfo in linked list*/
	int listen_fd, conn_fd;          /*file descr. for listen and connect*/
	int yes = 1;                     /*option value for setsockopt()*/
	struct sigaction sa;             /*struct that allows finer signal ctrl*/
	struct sockaddr_in sock_addr;    /*ip4 addr*/
	char ip4[INET_ADDRSTRLEN];       /*space to hold the ipv4 str*/

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

	/*now try to listen to the socket*/
	rc = listen(listen_fd, BACKLOG);
	if(rc == ERR){
		ERROR_LOG("socket listen returned -1");
		close(listen_fd); /*close fd to prevent leak*/
		return ERR; /*return -1 on fail*/
	}

	/*arm /register the signals*/
	sa.sa_handler = signal_handler; /*handler for when the sig is raised*/
	sigemptyset(&sa.sa_mask);    /*clear set before use*/
	sa.sa_flags = SA_RESTART;    /*restart syscall if interrupted*/

	/*child reaping process*/
	rc = sigaction(SIGCHLD, &sa, NULL);
	if(rc == ERR){
		ERROR_LOG("SIGCHLD registration returned -1");
		close(listen_fd); /*close fd to prevent leak*/
		return ERR; /*return -1 on fail*/
	}

	/*catch interrupt*/
	rc = sigaction(SIGINT, &sa, NULL);
	if(rc == ERR){
		ERROR_LOG("SIGINT registration returned -1");
		close(listen_fd); /*close fd to prevent leak*/
		return ERR; /*return -1 on fail*/
	}
	
	/*catch termination signal*/
	rc = sigaction(SIGTERM, &sa, NULL);
	if(rc == ERR){
		ERROR_LOG("SIGTERM registration returned -1");
		close(listen_fd); /*close fd to prevent leak*/
		return ERR; /*return -1 on fail*/
	}

	/*main accept loop*/
	while(1){
		conn_fd = accept(listen_fd, (struct sockaddr *)&sock_addr, INET_ADDRSTRLEN);
		if(conn_fd == ERR){
			ERROR_LOG("Accept returned -1");
			continue;
		}

		/*at this point the conn_fd is est.*/
		inet_ntop(sock_addr.sin_family, &sock_addr.sin_addr, ip4, INET_ADDRSTRLEN);
		if(ip4 == NULL){
			ERROR_LOG("NTOP returned -1");
			close(conn_fd);
			continue;
		}

		/*log message for successful connection*/
		DEBUG_LOG(	if(conn_fd == ERR){
			ERROR_LOG("Accept returned -1");
			continue;
		}





	
	
	}

	/*Test to see if compile without cross compile*/
	printf("Hello\n");
}

/*
 *signal_handler function
 *
 */
static void signal_handler(int signo){
	switch(signo){
		case SIGCHLD:
			int saved_errno = errno;
			while(waitpid(-1, NULL, WNOHANG) > 0);
			errno = saved_errno;
			break;
		case SIGINT:
			

	
	
	
	}



}
