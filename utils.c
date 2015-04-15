/* CpSc 852 Hw 3 Q 3: Utility code */
#include "common.h"
#include "globals.h"

void die(const char *msg) {
  if (errno == 0) {
    /* Just the specified message--no error code */
    puts(msg);
  } else {
    /* Message WITH error code/name */
    perror(msg);
  }
  printf("Die message: %s \n", msg);
  
  /* DIE */
  exit(EXIT_FAILURE);
}

double timestamp() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) { die("the sky is falling!"); }
  return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
}

void safe_send(int sock, char *string) {
  int sent;
  if ((sent = send(sock, string, strlen(string), 0)) < strlen(string)) {
    die("send() did not send all the bytes it should have [bad send()]");
  }
  bytesSent += sent;
}

int safe_recv(int sock, char *buffer, int len) {
  int recd;
  if ((recd = recv(sock, buffer, len, 0)) == -1) {
    die("recv() had a bad day");
  }
  buffer[recd] = '\0';
  bytesRecd += recd;
  return recd;
}
