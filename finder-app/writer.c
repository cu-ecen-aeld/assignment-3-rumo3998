/*This file is the c implementation of the writer.sh bash script*/

#define MIN_ARGS (3)
#define RET_ERR  (1)
#define FILE_ERR (-1)

/*Includes for required libraries and source files*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

/*main function, will be cli parsed*/
int main(int argc,  char *argv[])
{
	/*Setup syslogger, use program name for prepended message, entirely opt*/
	openlog(NULL,0, LOG_USER);

	/*variable to hold the file descriptor*/
	int fd;

	/*variables to hold the num bytes requested and written*/
	size_t count;
	ssize_t nr;

	/*check for 2 args entered @ cmd line*/
	if(argc != MIN_ARGS){
		/*wrong number of args entered at cli*/
		syslog(LOG_ERR,"Error: Incorrect number of args, retry and enter"
			       "2.");
		return RET_ERR;
	}

	/*more than 2 args, so try to open file*/
	fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 
		 S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

	/*unable to open file*/
	if(fd == FILE_ERR){
		/*unable to open file*/	
		syslog(LOG_ERR,"Error: Unable to open the specified file, check"
			       "name and try again");
		return RET_ERR;
	}

	/*Now that the file is open try to write the string*/
	if(argv[2] == NULL){
		syslog(LOG_ERR,"Error: Invalid string, check string and try again");
		return RET_ERR;
	}

	/*Now that we have the file open and know the 2nd arg is not NULL, attempt write*/
	count = strlen(argv[2]);
	nr = write(fd, argv[2], count);
       if (nr == FILE_ERR){
	       syslog(LOG_ERR,"Error: Unable to write the specified string");
	       return RET_ERR;
       }
       else if(nr != count){
	       syslog(LOG_ERR,"Error: Partial write");
	       return RET_ERR;
       }
       
       /*Made it to the end so we should be good now*/
       syslog(LOG_DEBUG,"Writing %s to %s\n", argv[2], argv[1]);
       close(fd);
       closelog();
       return 0;
}

