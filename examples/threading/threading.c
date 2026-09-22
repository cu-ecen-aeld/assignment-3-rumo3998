#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#define MS_TO_NS (1000000)
#define S_TO_MS (1000)

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param){
    
    /*used for checking function return codes*/
    int rc;

    //wait, obtain mutex, wait, release mutex as described by thread_data structure
    //hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    /*type cast the thread_param to begin parsing*/
    struct thread_data *data = (struct thread_data *) thread_param;
    struct timespec wt_obtain, wt_release;
    
    /*input validation*/
    if(thread_param == NULL){
	    return NULL;
    }

    /*convert and assign the requested time to a timeval struct to be used with nanosleep*/
    wt_obtain.tv_sec = data->obtain_time_ms / S_TO_MS;
    wt_obtain.tv_nsec = (data->obtain_time_ms % S_TO_MS) * MS_TO_NS;
    wt_release.tv_sec = data->release_time_ms / S_TO_MS;
    wt_release.tv_nsec = (data->release_time_ms % S_TO_MS) * MS_TO_NS;

    /*wait and then obtain the mutex*/
    do{
	   errno = 0;
	   rc = nanosleep(&wt_obtain, &wt_obtain);
    
    }while(rc != 0 && errno == EINTR );

    /*lock the mutex*/
    rc = pthread_mutex_lock(data->mutex);

    if(rc != 0){
	    ERROR_LOG("Err: Mutex Lock fail.\n");
            return thread_param;	    
    }
    
    /*wait and then release the mutex*/
    do{
	   errno = 0;
	   rc = nanosleep(&wt_release, &wt_release);
    
    }while(rc != 0 && errno == EINTR );

    /*unlock the mutex*/
    rc = pthread_mutex_unlock(data->mutex);

    if(rc != 0){
	    ERROR_LOG("Err: Mutex unlock fail.\n");
            return thread_param;	    
    }

    /*Wait, lock, wait, unlock successful*/
    data->thread_complete_success = true;
    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /*
     * allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    
    /*Create a ptr to my thread_data type, and create an int to hold the ret codes*/
    struct thread_data *thread_data;
    int rc;

    /*init the data struct and check that it is not NULL*/
    thread_data = malloc(sizeof(struct thread_data));

    if(thread_data == NULL){
	    ERROR_LOG("Err: Dyn. Mem Allocation.\n");
	    return false;	   
    }

    /*init the thread_data values*/
    thread_data->mutex = mutex; 
    thread_data->obtain_time_ms = wait_to_obtain_ms; 
    thread_data->release_time_ms = wait_to_release_ms; 
    thread_data->thread_complete_success = false;

    /*create the thread and use thread_func as an entry point*/
    rc = pthread_create(thread, NULL, threadfunc, thread_data);

    if(rc != 0){
	    ERROR_LOG("Err: pthread creation fail.\n");
            free(thread_data);
	    return false;    
    }

    /*Able to start thread*/
    return true;
}

