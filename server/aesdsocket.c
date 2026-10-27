/*
 * aesdsocket.c
 *
 * Created on: September 26, 2026
 * Author: Ruben Reyes Moreno
 */
/* Force safety definition override for Assignment 8 verification rules */
#ifndef USE_AESD_CHAR_DEVICE
#define USE_AESD_CHAR_DEVICE 1
#endif
/* ---Include libraries and headers---*/
#include "aesdsocket.h"
#include <sys/ioctl.h>
#include "aesd_ioctl.h"

/* ---Defines---*/
#define PORT      "9000" /*stream socket port*/
#define ERR       (-1)   /*return err code*/
#define BACKLOG   (10)   /*num of allowable pending connections*/
#define NUM_BYTES (1024) /*num of bytes to store data*/
#define MS_TO_NS  (1000000)
#define S_TO_MS   (1000)

/* ---build switch--- */
#ifndef USE_AESD_CHAR_DEVICE
#    define DST_FILE "/var/tmp/aesdsocketdata"
#else
#    define DST_FILE "/dev/aesdchar"
#endif

/* --globals--- */
volatile sig_atomic_t exit_code = 0;     /*atomic var that ctrls execution*/  
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
struct thread_s_head list_head = 
SLIST_HEAD_INITIALIZER(list_head);      /*sets head to NULL*/

/* ---main--- */
int main(int argc, char *argv[]){

	int rc;                          /*return code, for validation*/
	struct addrinfo hints;           /*relevant info*/
	struct addrinfo *server_info;    /*points to results*/
	struct addrinfo *next;           /*next addrinfo in linked list*/
	int listen_fd, conn_fd;          /*file descr. for listen and connect*/
	int yes = 1;                     /*option value for setsockopt()*/
	struct sigaction sa;             /*struct that allows finer signal ctrl*/
	int daemon_mode = 0;             /*daemon control*/
	
	/*parse cli args and check for daemon arg*/
	if(argc > 1 && strcmp(argv[1], "-d") == 0){
			daemon_mode = 1;
	}

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
	
	/*check to see if the next ptr is a valid addrinfo struct*/
	if(next == NULL){
		ERROR_LOG("socket connection was unsuccessful");
		freeaddrinfo(server_info);
		return ERR; /*return -1 on fail*/
	}

	/*free the linked-list*/
	freeaddrinfo(server_info);

	/*now try to listen to the socket*/
	rc = listen(listen_fd, BACKLOG);
	if(rc == ERR){
		ERROR_LOG("socket listen returned -1");
		close(listen_fd); /*close fd to prevent leak*/
		return ERR; /*return -1 on fail*/
	}

	struct timeval timeout;
	timeout.tv_sec=1;
	timeout.tv_usec=0;

	if(setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == ERR){
		ERROR_LOG("setsockopt SO_RCVTIMEO returned -1");
		close(listen_fd);
		return ERR;
	}

	/*create daemon given that -d was passed in cli as an arg*/
	if(daemon_mode){
		pid_t pid;

		/*create new process*/
		pid = fork();
		if(pid == ERR){
			return ERR;
		}
		else if(pid != 0){
			/*this is the parent process*/
			exit(EXIT_SUCCESS);
		}

		/*create new session and process group*/
		if(setsid() == ERR){
			return ERR;
		}

		/*set working dir to root*/
		if(chdir("/") == ERR){
			return ERR;	
		}

		/*close all open files*/
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	
		/*redirect fd's 0,1,2 to /dev/null*/
		int dev_null_fd = open("/dev/null",O_RDWR);
		if(dev_null_fd != ERR){
			dup2(dev_null_fd, STDIN_FILENO);
			dup2(dev_null_fd, STDOUT_FILENO);
			dup2(dev_null_fd, STDERR_FILENO);
			close(dev_null_fd); /*close original to prevent*/
		}
	}/*end daemon*/

	/*arm /register the signals*/
	sa.sa_handler = signal_handler; /*handler for when the sig is raised*/
	sigemptyset(&sa.sa_mask);       /*clear set before use*/
	sa.sa_flags = 0;                /*don't restart syscall if interrupted*/
	
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

/* ---build switch to disable timer thread--- */	
#ifndef USE_AESD_CHAR_DEVICE
	/*start time_stamp thread*/
	pthread_t time_tid;     /*thread id for timestamp*/
	rc = pthread_create(&time_tid, NULL, timestamp_thread, NULL);
	if(rc != 0){
		ERROR_LOG("time_thread creation returned an ERROR");
		close(listen_fd); /*close fd to prevent leak*/
		return ERR; /*return -1 on fail*/
	}
#endif

	/*main accept loop, run while global is not set*/
	while(!exit_code){
		struct sockaddr_in sock_addr;    /*ip4 addr*/
		socklen_t addr_size = sizeof(sock_addr);
		char ip4[INET_ADDRSTRLEN];       /*space to hold the ipv4 str*/

		conn_fd = accept(listen_fd, (struct sockaddr *)&sock_addr,&addr_size);
		if(conn_fd == ERR){
			if(errno == EINTR || errno == EAGAIN){ 
				/*stop loop, assume exit_code will stop future loops
				 * start to clean threads*/
				if(exit_code){
					free_list(&list_head);
				}else{
					clean_list(&list_head);
				}
				continue;
			}
			ERROR_LOG("Accept returned -1");
			continue;
		}

		/*at this point the conn_fd is est.*/
		const char * ret = inet_ntop(sock_addr.sin_family, 
				&sock_addr.sin_addr, ip4, sizeof(ip4));
		if(ret == NULL){
			ERROR_LOG("NTOP returned -1");
			close(conn_fd);
			continue;
		}
		/*successfully connected, log status*/
		DEBUG_LOG("Accepted connection from %s", ip4);

		/*start up thread for new connection socket*/
		struct thread_s *temp_thread = malloc(sizeof(struct thread_s));
		if(temp_thread == NULL){
			ERROR_LOG("connection thread pointer returned NULL");
			close(conn_fd);
			continue;
		}

		/*successfully allocated memory for a struct thread_s, now init
		 * struct*/
		temp_thread->is_done = false;
		temp_thread->conn_fd = conn_fd;
		rc = pthread_create(&temp_thread->tid, NULL, connection_thread, temp_thread); 

		if(rc != 0){
			ERROR_LOG("Failed to create conn thread");
			free(temp_thread);
			close(conn_fd);
			continue;
		}

		/*insert thread into linked list*/
		SLIST_INSERT_HEAD(&list_head, temp_thread, thread_node);

		/*check for completed threads and join / free*/
		clean_list(&list_head);				

	}/*end of connection accept() while*/
	
	/*Test to see if compile without cross compile*/
	DEBUG_LOG("Caught signal, exiting");

	/*free the linked list*/
	free_list(&list_head);

/*  ---build switch--- */
#ifndef USE_AESD_CHAR_DEVICE	
	pthread_cancel(time_tid);

	/*join the time stamp thread*/
	pthread_join(time_tid, NULL);
#endif
	/*close fd's and destroy mutex for fn access*/
	close(listen_fd);
	pthread_mutex_destroy(&file_mutex);

/*  ---build switch--- */
#ifndef USE_AESD_CHAR_DEVICE 
	if(remove(DST_FILE) == ERR){
		ERROR_LOG("File deletion returned -1");	
	}
	else{
		DEBUG_LOG("tmp file deleted");	
	}
#endif

	/*successful return*/
	return 0;
} /*end of main*/

/*
 *signal_handler function
 *
 */
void signal_handler(int signo){
	if(signo == SIGINT || signo == SIGTERM){
		exit_code = 1; /*keep handler short, update global*/
	}
}

/*
 *connection thread handler function
 *
 */
void *connection_thread(void *arg){
	
	/*input validation*/
	if(arg == NULL){
		return NULL;	
	}

	/*type cast the void ptr to a thread struct*/
	struct thread_s *node = (struct thread_s *)arg;

	/*extract the connection file descriptor*/
	int conn_fd = node->conn_fd;
	
	/*setup buffers to receive data*/
	char data_buf[NUM_BYTES];
	int packet_done = 0;
	ssize_t bytes_recvd; /*[-1,SSIZE_MAX] to check for ERR*/

	/*info used to interact with ipv4 address*/
	struct sockaddr_in sock_addr;             /*ip4 addr*/
	socklen_t addr_size = sizeof(sock_addr);  /*len of ip4 addr*/
	char ip4[INET_ADDRSTRLEN];                /*space to hold the ipv4 str*/

	/*get peer info*/
	getpeername(conn_fd, (struct sockaddr*)&sock_addr, &addr_size);

	const char * ret = inet_ntop(sock_addr.sin_family, 
			&sock_addr.sin_addr, ip4, sizeof(ip4));
	if(ret == NULL){
		ERROR_LOG("NTOP returned -1");
		close(conn_fd);
		node->is_done = true;
		return NULL;
	}

#ifndef USE_AESD_CHAR_DEVICE
	/* Regular file needs persistence for tracking positions */
	pthread_mutex_lock(&file_mutex);
	int data_fd = open(DST_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
	pthread_mutex_unlock(&file_mutex);
#else
	int data_fd = open(DST_FILE, O_RDWR);
#endif
	if(data_fd == ERR){
		ERROR_LOG("Failed to open destination path");
		close(conn_fd);
		node->is_done = true;
		return NULL;	
	}

	int is_seek_cmd = 0; /*declare and init seek command "bool"*/
		
	do{
		bytes_recvd = recv(conn_fd, data_buf, sizeof(data_buf), 0);
		if(bytes_recvd == ERR){
			ERROR_LOG("Recv returned -1");
			break;
		}

		/*check if client closed the fd*/
		if(bytes_recvd == 0){
			break;
		}

#ifdef USE_AESD_CHAR_DEVICE
		uint32_t cmd = 0;        /*Desired cmd*/
		uint32_t cmd_offset = 0; /*Desired offset from cmd*/
		const char *fmt = "AESDCHAR_IOCSEEKTO:%u,%u";

		/*check the data_buf for the string: AESDCHAR_IOCSEEKTO:X,Y
		 * where X and Y are unsigned decimal integer values
		 * X is write cmd to seek into and Y is the offset within 
		 * the write command.*/

		if(sscanf(data_buf, fmt, &cmd, &cmd_offset) == 2){
			DEBUG_LOG("IOCSEEKTO command found. Running with cmd:%u	cmd_offset:%u",
				       	cmd, cmd_offset);
			/*populate the struct aesd_seekto structure*/
			struct aesd_seekto seekto;
			seekto.write_cmd = cmd;
			seekto.write_cmd_offset = cmd_offset;

			/*let's try the ioctl call*/
			if(ioctl(data_fd, AESDCHAR_IOCSEEKTO, &seekto) == ERR){
				ERROR_LOG("IOCTL call failed");			
			}
			is_seek_cmd = 1; /*seek cmd ID'd and executed*/
		}

#else
		pthread_mutex_lock(&file_mutex);
#endif
		/*only do write if the seek command was not found/given*/
		if(!is_seek_cmd){
			write(data_fd, data_buf, bytes_recvd);
		}

#ifndef USE_AESD_CHAR_DEVICE
		pthread_mutex_unlock(&file_mutex);
#endif

		/*look thru data_buf for new line*/
		char *newline_found = memchr(data_buf, '\n', bytes_recvd);
		if(newline_found != NULL){
			packet_done = 1;
		}
		
	}while(!packet_done && !exit_code); /*end do(){}while;*/
		
	/*packet done, newline found, write back*/
	if(packet_done && !exit_code){
		
#ifndef USE_AESD_CHAR_DEVICE
		pthread_mutex_lock(&file_mutex);
		lseek(data_fd, 0, SEEK_SET);
#else
		if(!is_seek_cmd){
			lseek(data_fd, 0, SEEK_SET);
		}
#endif
		
		ssize_t bytes_read;
		/*read file NUM_BYTES @ a time*/
		while((bytes_read = read(data_fd, data_buf, sizeof(data_buf))) > 0){
			ssize_t bytes_sent = send(conn_fd, data_buf, bytes_read, 0);
			if(bytes_sent == ERR){
				ERROR_LOG("Conn sent returned -1");
				break;
			}
		}

#ifndef USE_AESD_CHAR_DEVICE
		pthread_mutex_unlock(&file_mutex);
#endif
	}
	
	/*close client and prep for next connection*/
	close(data_fd);
	close(conn_fd);


	/*disconnected, log status*/
	DEBUG_LOG("Closed connection from %s", ip4);
	node->is_done = true;
	return NULL;
}

/*
 *time stamp thread handler function
 *
 */
void *timestamp_thread(void *arg){
	while(!exit_code){
		int rc; /*return code*/
		struct timespec print_delay;
		print_delay.tv_sec = 10;
		print_delay.tv_nsec = 0;
		char time_str[200];
		time_t t;
		struct tm *tmp;

		/*wait to obtain the mutex*/
		do{
			errno = 0;
			rc = nanosleep(&print_delay, &print_delay);
		}while(rc != 0 && errno == EINTR && !exit_code);

		if(exit_code){
			break;
		}
		
		/*set up timestamp*/
		time(&t);
		tmp = localtime(&t);
		if(tmp == NULL){
			ERROR_LOG("localtime returned NULL");
			continue;
		}
		size_t len = strftime(time_str, sizeof(time_str), 
				"timestamp:%a, %d %b %Y %T %z\n", tmp);


		/*write chunk of data, but first lock mutex*/
		pthread_mutex_lock(&file_mutex);
		/*open or create the data file for RW appending*/
		int data_fd = open(DST_FILE, O_RDWR | O_CREAT | O_APPEND,
				0644);
		if(data_fd == ERR){
			ERROR_LOG("File returned -1");
		}
		else{
			write(data_fd, time_str, len);
			close(data_fd);
		}

		/*done writing, unlock mutex*/
		pthread_mutex_unlock(&file_mutex);
	}
	return NULL;
}


/*
 *helper function to free linked list elements
 */
void free_list(void *arg){
	/*type cast the arg to a thread_s_head type*/
	struct thread_s_head *head = (struct thread_s_head *)arg;
	struct thread_s *node = NULL; /*ptr to a thread_node*/

	/*check to make sure arg is valid*/
	if(head == NULL){
		return;
	}

	/*iterate thru the linked list and free each node*/
	while(!SLIST_EMPTY(head)){
		node = SLIST_FIRST(head);
		pthread_cancel(node->tid);
		pthread_join(node->tid, NULL);
		SLIST_REMOVE_HEAD(head, thread_node);
		free(node);
		node = NULL;
	}
}

/*
 *helper function to clean list
 */
void clean_list(void *arg){

	/*type cast the arg to a thread_s_head type*/
	struct thread_s_head *head = (struct thread_s_head *)arg;
	struct thread_s *node = NULL;
	struct thread_s *temp_node = NULL; /*ptrs to thread_node*/

	/*check to make sure arg is valid*/
	if(head == NULL){
		return;
	}

	/*iterate thru the linked list and free each node*/
	SLIST_FOREACH_SAFE(node, head, thread_node, temp_node){
		if(node->is_done){
			pthread_join(node->tid, NULL);
			SLIST_REMOVE(head, node, thread_s, thread_node);
			free(node);
			node = NULL;		
		}

	}
}

