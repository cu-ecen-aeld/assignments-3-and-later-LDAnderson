/* L.D. Anderson coursera intro to buildroot/linux
 * s/o to: https://www.linuxhowtos.org/C_C++/socket.htm
 */
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <syslog.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define DEFAULT_PORT 9000
#define LOG_FILE "/var/tmp/aesdsocketdata"

void int_handler();

struct sockaddr_in server_sockaddr, client_sockaddr;
int server_filed, client_filed;
int daemon_mode = 0;
socklen_t clientlen;
FILE *fp = NULL;
char buf[1024*1024], ch;

int main(int argc, char *argv[]) {
  pid_t pid;
  char remote_address[INET_ADDRSTRLEN];

  if(argc==2 && argv[1][1] == 'd') {
    printf("Starting daemon.\n");
    daemon_mode = 1;
    pid = fork();

    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if (setsid() < 0)
      exit(EXIT_FAILURE);
    /* close(STDIN_FILENO); */
    /* close(STDOUT_FILENO); */
    /* close(STDERR_FILENO); */
    if (pid > 0)
      exit(EXIT_SUCCESS);

  } // end daemon if

  server_filed = socket(AF_INET, SOCK_STREAM, 0);
  if(server_filed < 0) {
    perror("SERVER socket");
    exit(-1);
  }

  bzero((char *)&server_sockaddr, sizeof(server_sockaddr));
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(DEFAULT_PORT);
  server_sockaddr.sin_addr.s_addr = INADDR_ANY;
  const int enable = 1;
if (setsockopt(server_filed, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed"); 

  if(bind(server_filed, (struct sockaddr *) &server_sockaddr, sizeof(server_sockaddr)) < 0) {
    perror("SERVER bind");
    exit(-1);
  }

  signal(SIGINT, int_handler);
  signal(SIGTERM, int_handler);
  printf("Starting server.\n");

  fp = fopen(LOG_FILE, "a+");
  if (fp == NULL) {
    perror("SERVER opening log file");
    exit(-1);
  }

  while(1) {
    if (listen(server_filed, 5) < 0) {
      perror("SERVER listen");
      exit(-1);
    }

    for(;;) {
      clientlen = sizeof(client_sockaddr);
      client_filed =
          accept(server_filed, (struct sockaddr *)&client_sockaddr, &clientlen);
      if (client_filed < 0) {
        perror("SERVER accept");
        exit(-1);
      }

      inet_ntop(AF_INET, &(client_sockaddr.sin_addr), remote_address,
                INET_ADDRSTRLEN);
      syslog(LOG_USER, "Accepted connection from %s", remote_address);

      // get input from remote client
      int i;
      char c;

      bzero(buf, 30000);
      do {
        /* i = recv(client_filed, &buf, 30000, 0); */
        i = recv(client_filed, &c, 1, MSG_WAITALL);
        if(i<0) {
          perror("recv");
          exit(-1);
        }
        fprintf(fp, "%c", c);
        /* bzero(buf, 30000); */

      } while (c != '\n');

      rewind(fp);
      while (!feof(fp)) {
        ch = fgetc(fp);
        if(feof(fp))
          break;
        send(client_filed, &ch, 1, 0);
      }

      shutdown(client_filed, SHUT_RDWR);
      syslog(LOG_USER, "Closed connection from %s", remote_address);
      /* printf("Connection closed.\n"); */
    }

    fclose(fp);
    shutdown(client_filed, SHUT_RDWR);
    shutdown(server_filed, SHUT_RDWR);
    close(client_filed);
    close(server_filed);
    remove(LOG_FILE);

    exit(0);

    /* fp = fopen(LOG_FILE, "r"); */
    /* if (fp == NULL) { */
    /*   perror("SERVER opening log file"); */
    /*   exit(-1); */
    /* } */

    /* while (!feof(fp)) { */
    /*   ch = fgetc(fp); */
    /*   send(client_filed, &ch, 1, 0); */
    /* } */
  }


  return 0;
}

void int_handler() {
  syslog(LOG_USER, "Caught signal, exiting");
  printf("Shutting down server.\n");
  shutdown(client_filed, SHUT_RDWR);
  shutdown(server_filed, SHUT_RDWR);
  close(client_filed);
  close(server_filed);
  fclose(fp);
  remove(LOG_FILE);
  exit(0);
}
