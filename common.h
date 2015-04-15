/* CpSc 852 Hw 3 Q 3: Main header file */

/* LIBRARIES */
/*-------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

/* PROTOTYPES */
/*-------------------------------------------------------*/
void usage();
void die(const char *);
double timestamp();
void interrupt(int);
void stats();
void safe_send(int, char *);
int safe_recv(int, char *, int);

void parse(const char *, struct sockaddr_in *, char **, char **);
void iteration(const struct sockaddr_in *, char *, char *, int);

/* Utility macro */
#define OR_DIE(msg) if (errno) { die(msg); }

/* debug levels */
#define DEBUG_LEVEL_NONE 0
#define DEBUG_LEVEL_TRACE 1
#define DEBUG_LEVEL_DATA  2

