/* CpSc 852 Hw 3 Q 3: Core functionality code */
#include "common.h"
#include "globals.h"

/* The "default" content-length if that header is omitted */
#define ALMOST_INFINITY 0x7FFFFFFFFFFFFFF

/* Parse out the scheme, host and path */
void parse(const char *url, struct sockaddr_in *addr, char **domain_ptr, char **path_ptr)
{
  char *workBuffer = strdup(url);
  char *domain, *port, *path;
  int port_number;

  /* Sanity-check of the scheme */
  if (strncmp(url, "http://", 7) != 0) {
    die("Malformed URL; must start with 'http://'");
  }

  /* Find the components */
  domain = strchr(workBuffer, '/') + 2; /* Starts after the "//" */
  port = strchr(domain, ':');
  if (port == NULL) {
    port_number = 80;
    path = strchr(domain, '/');
    if (path == NULL || path[1] == '\0') {
      /* No slash? Slash at end of string? */
      die("Malformed URL; must contain a full path");
    }
    *path++ = '\0';   /* Cap off the domain */
  } else {
    *port++ = '\0';   /* Cap off the domain */
    path = strchr(port, '/');
    if (path == NULL || path[1] == '\0') {
      /* No slash? Slash at end of string? */
      die("Malformed URL; must contain a full path");
    }
    *path++ = '\0';   /* Cap off the port */
    port_number = strtol(port, NULL, 10); OR_DIE("error parsing port number");
  }

  /* Save the domain and path */
  *domain_ptr = strdup(domain);
  *path_ptr = strdup(path);

  /* First, try to interp the domain as a dotted-quad */
  memset(addr, 0, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inet_addr(domain);
  addr->sin_port = htons(port_number);

  /* No dotted-quad--try a name resolution */
  if (addr->sin_addr.s_addr == -1) {
    struct hostent *host = gethostbyname(domain);
    if (host == NULL) {
      /* No names for us... */
      /* Use this rather than die() to get the resolver's error msg */
      herror("Unable to resolve domain name");
      exit(EXIT_FAILURE);
    }
    addr->sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);
  }

  /* Free up the work buffer */
  free(workBuffer);
}

void iteration(const struct sockaddr_in *serverAddr, char *domain, char *path, int debugFlag) {
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  int sent, recd;
  long long content_length = ALMOST_INFINITY;  /* Assume "infinite" length */
  unsigned int recd_at_start = bytesRecd;  /* How many had we received before this? */
  double start, end;  /* For measuring RESPONSE time. */
  int buffa, buffb, buffer_size, to_read;  /* We use a dynamically-sized buffer */
  char *buffer = NULL;
  char *ptr = NULL;
  double throughput, response_time;
  unsigned int r = 0;
  int totalRead=0;

  /* Calculate buffer size and allocate */
  buffa = strlen(domain) + 10;
  buffb = strlen(path) + 35;
  buffer_size = (buffa > buffb) ? buffa + 1 : buffb + 1;
  if (buffer_size < 256)
  {
     /* Minimum buffer size */
     buffer_size = 256;
  }
  if (debugFlag > 0) {
       printf("Set buffer size to  %d \n", buffer_size);
  }
  if ((buffer = malloc(buffer_size)) == NULL)
  {
     die("Unable to allocate buffer");
  }
  to_read = buffer_size - 1;

  /* Sanity check */
  if (sock < 0)
  {
    die("socket() failed");
  }

  /* Connect */
  if (connect(sock, (struct sockaddr *)serverAddr, sizeof(struct sockaddr)) < 0) { die("connect() failed"); }

  /* Send request path and non-keep-alive header (lines 1 & 2) */
  sprintf(buffer, "GET /%s HTTP/1.1\r\nConnection: close\r\n", path);
  safe_send(sock, buffer);
  if (debugFlag == 2) {
    printf("SEND:\n%s \n", buffer);
  }

  /* Next, the host header and separator [line 2 & 3] */
  sprintf(buffer, "Host: %s\r\n\r\n", domain);
  safe_send(sock, buffer);
  if (debugFlag == 2) {
    printf("SEND:\n%s \n", buffer);
  }

  /* Start response-time timer */
  start = timestamp();

  //JJM First time, the buffer has the sent GET
  if (debugFlag > 0) {
    if (strncmp(buffer, "\r\n\r\n", 4) == 0)
    {
      printf("FOUND the four control chars \n");
    }
  }
  /* Read until "\r\n\r\n" is at the head of the buffer */
  while (strncmp(buffer, "\r\n\r\n", 4) != 0) {
    /* Sniff for content-length header, if there
       (This isn't really needed now that we ask for connection: close,
       but it works and looks cool, so we're leaving it in...
    */
    if (strncasecmp(buffer, "\r\nContent-Length:", 17) == 0) {
      content_length = strtoll(buffer + 17, NULL, 10);
      if (debugFlag > 0)
      {
        printf("FOUND the Content-Length: %d \n",(unsigned int)content_length);
      }
    }

    /* If we read less than a full buffer, insert the data down the line,
      not at the beginning.  We want the new data to sit at the END
      of the buffer. */
    if (debugFlag > 0) {
      printf("Before FIRST safe_recv, buf ptr:%p, buffer_size:%d;  with to_read:%d (offset:%d)  \n", buffer + (buffer_size - 1 - to_read), buffer_size,to_read, (buffer_size - 1 - to_read));
    }
    r = safe_recv(sock, buffer + (buffer_size - 1 - to_read), to_read);
    totalRead += r;
//    if (r <= 0)
    if (r < 0) {
      die("premature error/disconnect while reading HTTP header");
    }
    if (debugFlag > 0) {
        printf("Bytes received from FIRST safe_recv %d, totalRead:%d \n", r,totalRead);
    }
    if (debugFlag == 2) {
      if (r > 0) {
        printf("Received from FIRST safe_recv:\n%s\n", buffer + (buffer_size - 1 - to_read));
      }
    }

    /* Find the first '\r' in the string [AFTER the first character] */
    if ((ptr = strchr(buffer + 1, '\r')) == NULL) {
      /* If there wasn't one, we'll completely refill the buffer next time */
      to_read = buffer_size - 1;
    } else {
      /* If there was one, "slide" it and all following characters
        to the beginning of our buffer and adjust our "to_read" count
        accordingly; this way we don't accidentally split the "\r\n\r\n"
        sequence between two recv()s and miss it... */
      to_read = ptr - buffer;
      strcpy(buffer, ptr);
    }
    if (r==0)
      break;
  }
  content_length -= buffer_size - 4;  /* Count the data we've already read */
  if (content_length < 0)
    content_length = 0;
  /* Read rest of data */
  to_read = buffer_size - 1;
  if (debugFlag > 0) {
     printf("DONE with FIRST PASS,  content_length:%d , left to_read:%d, buffer_size:%d, totalRead:%d\n", (unsigned int)content_length,to_read, buffer_size, totalRead);
  }

  while (content_length > 0) {
    if (debugFlag > 0) {
        printf("Before SECOND safe_recv, buf ptr:%p with to_read:%d, totalRead:%d  \n", buffer, to_read,totalRead);
    }
    r = safe_recv(sock, buffer, to_read);
    totalRead += r;
    if (debugFlag > 0) {
        printf("Bytes received from SECOND safe recv %d, totalRead:%d \n", r,totalRead);
    }
    if (debugFlag == 2) {
      if (r > 0) {
        printf("Received from SECOND safe_recv:\n%s \n", buffer);
      }
    }
    if (r == 0) { /* HUP--we're done */
      break;
    } else if (r < 0) { /* Bad medicine */
      die("error in recv()");
    }
    content_length -= r;
  }

  /* Stop clock */
  end = timestamp();

  /* Close our socket, free our buffer */
  close(sock);
  free(buffer);

  /* Update running averages */
  response_time = end - start;
  throughput = ((double)(bytesRecd - recd_at_start)) / response_time;
  avgThroughput += (throughput - avgThroughput) / (currentIter + 1);
  avgResponseTime += (response_time - avgResponseTime) / (currentIter + 1);

  /* Print stats if in debug mode */
  if (debugFlag > 0) {
    printf("%.6f: %d bytes received in %.6f seconds (totalRead:%d)\n", end,
            bytesRecd - recd_at_start, end - start,totalRead);
  }
}
