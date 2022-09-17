#include "threading.h"
#include <bits/pthreadtypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

  struct thread_data *data = (struct thread_data *) thread_param;
  int ms1 = data->wait_to_obtain_ms;
  int ms2 = data->wait_to_release_ms;
  int rc = -1;


  // TODO: wait, obtain mutex, wait, release mutex as described by thread_data
  // structure hint: use a cast like the one below to obtain thread arguments
  // from your parameter
  // struct thread_data* thread_func_args = (struct thread_data *) thread_param;
  usleep(ms1);

  rc = pthread_mutex_lock(data->mutex);

  if(rc != 0) {
    printf("Failed to lock mutex\n");

    data->thread_complete_success = false;
    return thread_param;
  }

  usleep(ms2);
  rc = pthread_mutex_unlock(data->mutex);
  if (rc != 0) {
    printf("Failed to unlock mutex\n");

    data->thread_complete_success = false;
    return thread_param;
  }

  data->thread_complete_success = true;

  return thread_param;
  }

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    struct thread_data *td = NULL;
    td = (struct thread_data *)malloc(sizeof(struct thread_data));

    td->thread_complete_success = false;
    td->wait_to_obtain_ms = wait_to_obtain_ms * 1000;
    td->wait_to_release_ms = wait_to_release_ms * 1000;

    td->mutex = mutex;
    if ((pthread_create(thread, NULL, threadfunc, td)) != 0)
      return false;
    /* pthread_join(*thread, NULL); */
    return true;
}
