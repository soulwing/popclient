/***********************************************************************
  module:       socket.c
  program:      popclient
  SCCS ID:      @(#)socket.c	1.5  4/1/94
  programmer:   Virginia Tech Computing Center
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3 
  description:  UNIX sockets code.
 ***********************************************************************/
 
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <varargs.h>

#include "socket.h"

/* requisite, venerable SCCS ID */
static char sccs_id [] = "@(#)socket.c	1.4\t3/29/94";

int Socket(host, clientPort)
char *host;
int clientPort;
{
    int sock;
#ifndef USE_TERM  /*  USE_TERM *IS NOT* defined  */
    unsigned long inaddr;
    struct sockaddr_in ad;
    struct hostent *hp;
    
    memset(&ad, 0, sizeof(ad));
    ad.sin_family = AF_INET;

    inaddr = inet_addr(host);
    if (inaddr != INADDR_NONE)
        memcpy(&ad.sin_addr, &inaddr, sizeof(inaddr));
    else
    {
        hp = gethostbyname(host);
        if (hp == NULL)
            return -1;
        memcpy(&ad.sin_addr, hp->h_addr, hp->h_length);
    }
    ad.sin_port = htons(clientPort);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return sock;
    if (connect(sock, &ad, sizeof(ad)) < 0)
        return -1;
    return sock;
#else  /*  USE_TERM *IS* defined  */
#include <client.h>
    if ((sock = connect_server(0)) < 0) {  /*  try to get an fd from term  */
	/*  couldn't get an fd because the term server apparently wasn't
	    running  */
	return(-1);
    }
    if (send_command(sock, C_PORT, 0, "%s:%d", host, clientPort) >= 0) {
	/*  tell the term server how to handle this particular connection  */
	send_command(sock, C_PRIORITY, 1, TERM_PRIORITY);
	    /*  how much priority the term server should give this client  */
	send_command(sock, C_COMPRESS, 1, (TERM_COMPRESS) ? "y" : "n");
	    /*  whether or not term should compress data for this client  */
	send_command(sock, C_DUMB, 1, 0);
	    /*  send term server into dumb mode (ie it won't accept any
		further commands  */
	return(sock);
    }
    else  /*  term couldn't connect to host:clientPort;  report error(?)  */
	return(-1);
#endif
}

int SockGets(socket,buf,len)
int socket;
char *buf;
int len;
{
    while (--len)
    {
        if (read(socket, buf, 1) != 1)
            return -1;
        if (*buf == '\n')
            break;
        if (*buf != '\r') /* remove all CRs */
            buf++;
    }
    *buf = 0;
    return 0;
}

int SockPuts(socket,buf)
int socket;
char *buf;
{
    int rc;
    
    if (rc = SockWrite(socket, buf, strlen(buf)))
        return rc;
    return SockWrite(socket, "\r\n", 2);
}

int SockWrite(socket,buf,len)
int socket;
char *buf;
int len;
{
    int n;
    
    while (len)
    {
        n = write(socket, buf, len);
        if (n <= 0)
            return -1;
        len -= n;
        buf += n;
    }
    return 0;
}

int SockRead(socket,buf,len)
int socket;
char *buf;
int len;
{
    int n;
    
    while (len)
    {
        n = read(socket, buf, len);
        if (n <= 0)
            return -1;
        len -= n;
        buf += n;
    }
    return 0;
}

int SockPrintf(socket,format,va_alist)
int socket;
char *format;
va_dcl {

    va_list ap;
    char buf[8192];
    
    va_start(ap);
    vsprintf(buf, format, ap);
    va_end(ap);
    return SockWrite(socket, buf, strlen(buf));
}

int SockStatus(socket,seconds)
int socket;
int seconds;
{
    int fds = 0;
    struct timeval timeout;
    
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    fds |= 1 << socket;
    if (select(socket+1, &fds, NULL, NULL, &timeout) <= 0)
        return 0;
    return 1;
}
