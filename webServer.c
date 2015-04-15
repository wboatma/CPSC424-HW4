/* Cullum Smith (gcsmith@clemson.edu)
 * CPSC 624 - HW2
 * Primitive HTTP server
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define BUFSIZE 1024


/* signal handler to terminate zombie processes */
void childhandler() {
   while(waitpid(-1, 0, WNOHANG) > 0) ;
}

int main(int argc, char** argv) {
   int sd;
   int cd;
   int port;
   unsigned int len;
   unsigned long size;
   unsigned long sent = 0;
   struct sockaddr_in skaddr;
   struct sockaddr_in cliaddr;
   pid_t cpid;
   char buf[BUFSIZE];

   /* signal handler to terminate zombie processes */
   signal(SIGCHLD, childhandler);

   /* get port as command-line arg */
   if (argc != 2) {
      fprintf(stderr, "usage: ./webServer [port]\n");
      exit(EXIT_FAILURE);
   }
   port = atoi(argv[1]);

   /* create a TCP socket for the server to listen on */
   if ((sd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
      perror("could not create TCP socket for server");
      exit(EXIT_FAILURE);
   }

   /* set up skadrr for server socket */
   bzero(&skaddr, sizeof(skaddr));
   skaddr.sin_family       = AF_INET;
   skaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
   skaddr.sin_port         = htons(port);

   /* bind socket to port */
   if (bind(sd, (struct sockaddr *) &skaddr, sizeof(skaddr)) == -1) {
      perror("could not bind socket to port");
      exit(EXIT_FAILURE);
   }

   /* put the socket into passive mode to wait for connections */
   if (listen(sd, 32) == -1) {
      perror("could not put socket into passive mode\n");
      exit(EXIT_FAILURE);
   }

   for (;;) {
      /* block until a new connection is received */
      len = sizeof(cliaddr);
      if ((cd = accept(sd, (struct sockaddr*) &cliaddr, &len)) == -1) {
         perror("accept() failed for a client");
         continue;
      }

      /* accept() has established a connection - fork child */
      if ((cpid = fork()) == 0) {
         /* close listening socket */
         close(sd);

         /* read HTTP header */
         read(cd, buf, BUFSIZE);
         if (sscanf(buf, "GET /DATASIZE=%lu HTTP/1.1\r\nConnection: close\r\n", &size) != 1) {
            fprintf(stderr, "received malformed HTTP request\n");
            exit(EXIT_FAILURE) ;
         }

         /* print client info */
         fprintf(stderr, "client %s:%d connected, requesting %lu bytes\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), size);

         /* send HTTP header */
         len = sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %lu\r\n\r\n", size);
         if (write(cd, &buf, len) == -1) {
            perror("write failed");
            exit(EXIT_FAILURE);
         }

         /* send dummy data */
         bzero(buf, BUFSIZE);
         while (sent < size) {
            if (write(cd, &buf, (size-sent >= BUFSIZE ? BUFSIZE : size-sent)) == -1) {
               perror("write failed");
               exit(EXIT_FAILURE);
            }
            sent += BUFSIZE;
         }

         /* close client socket descriptor and terminate child process */
         close(cd);
         fprintf(stderr, "client %s:%d request serviced\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
         exit(EXIT_SUCCESS);
      } else {
         /* close client descriptor in parent process */
         close(cd);
      }
   }
   return 0;
}
