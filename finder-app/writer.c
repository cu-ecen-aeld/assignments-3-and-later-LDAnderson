/* Writes a string to a file
   usage: writer FULL_PATH STRING
   Leif Anderson 
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/syslog.h>
#include <syslog.h>

void writeFile(char *filename, char *s) {
  FILE* fp = NULL;

  syslog(LOG_DEBUG, "Writing %s to %s", filename, s);

  fp = fopen(filename, "w");
  if(fp == NULL) {
    syslog(LOG_ERR, "Cannot open file");
    exit(1);
  }

  fprintf(fp, s);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  openlog(NULL, 0, LOG_USER);

  if(argc != 3) {
    syslog(LOG_ERR, "Invalid arguments");
    exit(1);
  }

  char *filename = argv[1];
  char *s = argv[2];
  writeFile(filename, s);

  return 0;
}
