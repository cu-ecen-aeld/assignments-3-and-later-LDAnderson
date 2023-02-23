/*
  assignment 6
 */
#include <assert.h>
#include <errno.h>
#include "queue.h"
#include <time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <syslog.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "aesd_ioctl.h"

#define DEFAULT_PORT 9000
#define USE_AESD_CHAR_DEVICE 1

#if USE_AESD_CHAR_DEVICE == 1
#define LOG_FILE "/dev/aesdchar"
#else
#define LOG_FILE "/var/tmp/aesdsocketdata"
#endif

void int_handler();
void *socket_thread(void* threadp);
void init_server();
void *timer_thread();
void open_log_file();
void init_linked_list();
void setup_signal_handlers();
pid_t start_daemon();

typedef struct {
  pthread_t* thread_id;
  struct sockaddr_in client_sockaddr;
  int client_filed;
  int finished;
} thread_info;

typedef struct slist_data_s slist_data_t;
struct slist_data_s {
  thread_info* thread;
  SLIST_ENTRY(slist_data_s) entries;
};

struct sockaddr_in server_sockaddr;
const int enable = 1;
pthread_mutex_t lock;
int daemon_mode = 0;
char buf[1024*1024] = {0}, ch = 0;
int server_filed = -1;
socklen_t clientlen = sizeof(server_sockaddr);
slist_data_t *datap = NULL;
FILE* fp = NULL;
SLIST_HEAD(slisthead, slist_data_s) head;

int main(int argc, char *argv[]) {
  int i = 0;
  pid_t pid;
  thread_info* threadinfo;
  pthread_t timer;

  init_linked_list();
  open_log_file();
  setup_signal_handlers();

  // background process if daemon cmd line option
  if(argc==2 && argv[1][1] == 'd') {
    pid = start_daemon();
  }

  #if USE_AESD_CHAR_DEVICE == 0
  // start timer thread
  pthread_create(&timer, (void *)0, timer_thread, (void *)0);
  #endif

  // begin server code
  printf("Starting server.\n");
  init_server();


  while(1) {
    if (listen(server_filed, 5) < 0) {
      perror("SERVER listen");
      exit(-1);
    }

    // Create client thread
    datap = malloc(sizeof(slist_data_t));
    datap->thread = malloc(sizeof(thread_info));
    datap->thread->thread_id = malloc(sizeof(pthread_t));

    datap->thread->finished = 0;
    datap->thread->client_filed =
      accept(server_filed, (struct sockaddr *)&datap->thread->client_sockaddr, &clientlen);
    if (datap->thread->client_filed < 0) {
      perror("SERVER accept");
      exit(-1);
    }

    SLIST_INSERT_HEAD(&head, datap, entries);

    pthread_create(datap->thread->thread_id,
                   (void *)0,
                   socket_thread,
                   datap->thread
    );

    SLIST_FOREACH(datap, &head, entries) {
      if(datap->thread->finished != 0) {
        pthread_join(*datap->thread->thread_id, NULL);
        free(datap->thread->thread_id);
        free(datap->thread);
        SLIST_REMOVE(&head, datap, slist_data_s, entries);
        /* free(datap); */
      }
    }
  }
}

void int_handler() {
  syslog(LOG_USER, "Caught signal, exiting");
  printf("Shutting down server.\n");
  while (!SLIST_EMPTY(&head)) {
    datap = SLIST_FIRST(&head);
    shutdown(datap->thread->client_filed, SHUT_RDWR);
    close(datap->thread->client_filed);
    pthread_join(*datap->thread->thread_id, NULL);
    free(datap->thread->thread_id);
    free(datap->thread);
    SLIST_REMOVE_HEAD(&head, entries);
    free(datap);
  }

  fclose(fp);
  shutdown(server_filed, SHUT_RDWR);
  close(server_filed);
  remove(LOG_FILE);
  exit(0);
}

void init_server() {


  server_filed = socket(AF_INET, SOCK_STREAM, 0);
  if (server_filed < 0) {
    perror("SERVER socket");
    exit(-1);
  }

  bzero((char *)&server_sockaddr, sizeof(server_sockaddr));
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(DEFAULT_PORT);
  server_sockaddr.sin_addr.s_addr = INADDR_ANY;
  if (setsockopt(server_filed, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
      0)
    perror("setsockopt(SO_REUSEADDR) failed");

  if (bind(server_filed, (struct sockaddr *)&server_sockaddr,
           sizeof(server_sockaddr)) < 0) {
    perror("SERVER bind");
    exit(-1);
  }
}

void *socket_thread(void* threadp) {
  FILE * fp2;
  int i;
  int fpos;
  char c;
  char *tmp;
  char remote_address[INET_ADDRSTRLEN];
  thread_info* threadparams = threadp;
  int client_filed = threadparams->client_filed;
  struct sockaddr_in client_sockaddr = threadparams->client_sockaddr;
  struct aesd_seekto ioctl_object;

  tmp = malloc(1024 * sizeof(char *));
  bzero(tmp, 1024);

  inet_ntop(AF_INET, &(client_sockaddr.sin_addr), remote_address,
            INET_ADDRSTRLEN);
  syslog(LOG_USER, "Accepted connection from %s", remote_address);

  // get input from remote client

  bzero(buf, 30000);

  pthread_mutex_lock(&lock);

  fpos = 0;
  do {
    fpos++;
    i = recv(client_filed, &c, 1, MSG_WAITALL);

    if (i < 0) {
      perror("recv");
      exit(-1);
    }

    sprintf(&tmp[fpos-1], "%c", c);
  } while (c != '\n');

  // check string for ioctl cmd

  if(strncmp(tmp, "AESDCHAR_IOCSEEKTO", strlen("AESDCHAR_IOCSEEKTO")) == 0) {

	  int cmd_pos = tmp[19] - '0';
	  int cmd_offset = tmp[21] - '0';

	  ioctl_object.write_cmd = cmd_pos;
	  ioctl_object.write_cmd_offset = cmd_offset;

	  printf("\nIN SEEKTO   pos: %d    offset: %d\n", ioctl_object.write_cmd, ioctl_object.write_cmd_offset);

	  if(ioctl(fileno(fp), AESDCHAR_IOCSEEKTO, &ioctl_object) < 0) {
		  perror("ioctl");
		  exit(-1);
	  }

  } else {

	fprintf(fp, "%s", tmp);
  }

  fflush(fp);
  pthread_mutex_unlock(&lock);

  fp2 = fopen(LOG_FILE, "r");
  while (!feof(fp2)) {
    ch = fgetc(fp2);
    if (feof(fp2))
      break;
    send(client_filed, &ch, 1, 0);
  }
  fclose(fp2);

  threadparams->finished = 1;
  shutdown(client_filed, SHUT_RDWR);
  syslog(LOG_USER, "Closed connection from %s", remote_address);
  close(client_filed);
}

void* timer_thread() {
  char timestr[200];
  time_t t;
  struct tm *tmp;

  for(;;) {
    sleep(10);
    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
      perror("localtime");
      exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&lock);
    strftime(timestr, sizeof(timestr), "%a, %d %b %Y %T %z", tmp);
    fprintf(fp, "timestamp:%s\n", timestr);
    pthread_mutex_unlock(&lock);
  }
}

void open_log_file() {
  fp = fopen(LOG_FILE, "w+");
  if (fp == NULL) {
    perror("SERVER opening log file");
    exit(-1);
  }
}

void init_linked_list() {
  SLIST_INIT(&head);
  assert(SLIST_EMPTY(&head) && "SList init");
}

void setup_signal_handlers() {
  if(signal(SIGINT, int_handler) == SIG_ERR) {
    perror("signal");
  }

  if (signal(SIGTERM, int_handler) == SIG_ERR) {
    perror("signal");
  }
}

pid_t start_daemon() {
  pid_t r;
  printf("Starting daemon.\n");
  daemon_mode = 1;
  r = fork();

  if (r < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (setsid() < 0)
    exit(EXIT_FAILURE);
  if (r > 0)
    exit(EXIT_SUCCESS);
  return r;
}
