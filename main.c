/********************************************************************
*
* Program:  webGet
*
* Description:  This program is a simple Web Get client.  It issues
*  a standard HTTP 1.1 GET command from a web server.  The
*  Connection is immediately closed after the transfer.
*
*
* Todo:
*   -Change to unix style params
*   -Add option to save URL object to a local file
*
*********************************************************************/

/* Common includes */
#include "common.h"

/* GLOBALS */
/*-------------------------------------------------------*/
double startTime, endTime, delay = 0.0;
unsigned long iterations, currentIter, bytesSent, bytesRecd;
double avgThroughput = 0.0, avgResponseTime = 0.0;
char Version[32] = "1.001   ";

/* PROGRAM */
/*-------------------------------------------------------*/
int main(int argc, char** argv) {
  int debugFlag = DEBUG_LEVEL_NONE;
  struct sockaddr_in serverAddress;
  char *domain = NULL;
  char *path = NULL;


  /* Make sure the poor user has a clue. */
  if (argc < 5) { usage(); }

  /* Parse args [with error checking/handling] */
  parse(argv[1], &serverAddress, &domain, &path);
  iterations = strtol(argv[2], NULL, 10); OR_DIE("Unable to parse 'iterations' parameter")
  delay = strtod(argv[3], NULL);          OR_DIE("Unable to parse 'delay' parameter")
  debugFlag = strtol(argv[4], NULL, 10);  OR_DIE("Unable to parse 'debugFlag' parameter")
  if (iterations < 0 || delay < 0) { die("Both 'iterations' and 'delay' must be >= 0!"); }

  /* Set up signal handler [for ^C] */
  signal(SIGINT, interrupt);

  /* Start the timer... */
  startTime = timestamp();

  /* Main loop */
  for (currentIter = 0; (iterations <= 0) || (currentIter < iterations);) {
    /* Run the thing */
    iteration(&serverAddress, domain, path, debugFlag);
    ++currentIter;  /* For more sane-looking dumps on ^C */

    /* Delay between iterations */
    usleep(delay * 1000000);
  }

  /* Wrap up */
  endTime = timestamp();
  stats();
}

/* PROGRAM FUNCTIONS */
/*-------------------------------------------------------*/
void usage() {
  printf("usage: webGet (version: %s) :  a simple URL GET client \n",Version);
  printf("usage:\t./webGet URL iterations delay debugFlag\n");
  printf("usage:\tURL: full URL decription\n");
  printf("usage:\tdelay: in seconds\n");
  printf("usage:\tdebugFlag:  0: none; 1:some; 2:data\n");
  printf("usage:\tExample:  ./webGet http://darkhelmet.cs.clemson.edu/DataFile2.dat 1 1 0\n");
  exit(EXIT_SUCCESS);
}

void interrupt(int sig) {
  /* Must have received CNT-C... Stop clock, print stats, and quit. */
  endTime = timestamp();
  stats();
  exit(EXIT_SUCCESS);
}

void stats() {
  /* Because ctime() is intellectually challenged and must have a time_t* arg... */
  time_t tStart = (time_t)startTime, tEnd = (time_t)endTime;

  /* Print stats */
  printf("Start time: %s", ctime(&tStart));
  printf("End time: %s", ctime(&tEnd));
  printf("Seconds elapsed: %.6f\n", endTime - startTime);
  printf("Iterations: %lu\n", currentIter);
  printf("Bytes sent: %lu\nBytes received: %lu\n", bytesSent, bytesRecd);
  printf("Average throughput: %.6lf Bps\n", avgThroughput);
  printf("Average response time: %.6f s\n", avgResponseTime);
}

