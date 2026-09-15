#include "systemcalls.h"
#include <errno.h>
#include <stdlib.h>
#define _XOPEN_SOURCE

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
	/*Check that a cmd was passed, return false if not*/
	if(cmd == NULL){
		return false; 
	}

	/*Run system()*/
	int ret; 
	ret = system(cmd);

	/*check the return for an error or the exit status*/
	if(ret == -1){
		/*error with system()*/
		return false;		
	}

	/*system() ran, now check if cmd executed successfully*/	
	if (WIFEXITED(ret)){
		/*if WIFEXITED is true, then cmd/process terminated normally
		 * check the return code on the commands*/
		if(WEXITSTATUS(ret) ==  0){
			return true;
		}
		return false;
	}

	/*if the cmd/process did not exit normally return false*/
	return false;
}	

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    /*Input validation*/
    if (count < 1){
	    return false;    
    }
    
    /*Creates the variable arg list type, that are used with traversal macros*/
    va_list args;
    /*count, tells the va_start where the args actually begin*/
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
	/*va_arg gets the next arg in the list, given that the type is provided*/
        command[i] = va_arg(args, char *);
    }
    /*arg vector needs to be NULL terminated*/
    command[count] = NULL;

/*
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
   /*variables to hold ret status from pid and the pid number*/ 
    int status;
    pid_t pid;

    /*call fork, and then check if successful*/
    pid = fork();

    if(pid == -1){
	    va_end(args);
	    return false;
    }
    else if (pid == 0){
	 /*This is the child process*/
	 int execv_ret;
	 execv_ret = execv(command[0],command);

	 /*if execv returns then the call was unsuccessful*/
	 if(execv_ret == -1){
		 exit(EXIT_FAILURE);
	 }
    }

    /*Following the return of the cmd, the parent will reap the child process
     * wait for the PID given by pid, status is the child's info, and 0 means
     * the wait will be blocking*/
    if (waitpid(pid,&status,0) == -1){
	    va_end(args);
	    return false;    
    }
    else if (WIFEXITED(status) == 1){
	    /*exited normally, now check if cmd did as well*/
	    if(WEXITSTATUS(status) == 0){
			    /*clean up va list and then return*/
			    va_end(args);
			    return true;	    
	    }    
    }
    
    va_end(args);
    return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{

    /*input validation for *outputfile*/
    if(outputfile == NULL || count < 1){
	    return false;
    }

    /*parse the variable cmd list*/
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    
/*
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    /*now redirect the stdout, fd = 1*/
    int kidpid;
    int status;
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    /*check to see if the fd was successfully opened*/ 
    if (fd == -1){
	    va_end(args);
	    return false;
    }

    /*now that the fd is open fork the process, and redirect stdout*/
    switch (kidpid = fork()){
	    case -1:
		    close(fd);
		    va_end(args);
		    return false;
	    case 0:
		    if(dup2(fd, 1) == -1){ /*redirect to stdout which is fd = 1*/
			    va_end(args);
			    exit(EXIT_FAILURE);
		    }

		    /*close the original fd, passed in by the user*/
		    close(fd);

		    /*Run execv()*/
	   	    int execv_ret;
	   	    execv_ret = execv(command[0],command);
	   
		    /*if execv returns then the call was unsuccessful*/
		    if(execv_ret == -1){
			    exit(EXIT_FAILURE);
	   	    }

	    default:
		    close(fd);
    }

    /*Following the return of the cmd, the parent will reap the child process
     * wait for the PID given by pid, status is the child's info, and 0 means
     * the wait will be blocking*/
    if (waitpid(kidpid,&status,0) == -1){
	    va_end(args);
	    return false;    
    }
    else if (WIFEXITED(status) == 1){
	    /*exited normally, now check if cmd did as well*/
	    if(WEXITSTATUS(status) == 0){
			    /*clean up va list and then return*/
			    va_end(args);
			    return true;	    
	    }    
    }
    
    va_end(args);
    return false;
}


