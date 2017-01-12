
/***********************************************************************
  module:       socket.h
  program:      popclient
  SCCS ID:	@(#)socket.h	1.5  4/1/94
  programmer:   Carl Harris, ceharris@vt.edu
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3 
  description:  Definitions module for socket.c
 ***********************************************************************/
 
#ifndef SOCKET__
#define SOCKET__

#ifndef  INADDR_NONE
#ifdef   INADDR_BROADCAST
#define  INADDR_NONE	INADDR_BROADCAST
#else
#define	 INADDR_NONE	-1
#endif
#endif

#ifndef NO_PROTO
/*
Create a new client socket 
returns < 0 on error 
*/
int Socket(char *host, int clientPort);

/* 
Get a string terminated by an '\n', delete any '\r' and the '\n'.
Pass it a valid socket, a buffer for the string, and
the length of the buffer (including the trailing \0)
returns 0 for success. 
*/
int SockGets(int socket, char *buf, int len);

/*
Send a nul terminated string to the socket, followed by 
a CR-LF.  Returns 0 for success.
*/
int SockPuts(int socket, char *buf);

/*
Write a chunk of bytes to the socket.
Returns 0 for success.
*/
int SockWrite(int socket, char *buf, int len);

/*
Read a chunk of bytes from the socket.
Returns 0 for success.
*/
int SockRead(int socket, char *buf, int len);

/* 
Send formatted output to the socket, followed
by a CR-LF.
Returns 0 for success.
*/
int SockPrintf();

/*
Check socket for readability.  return 0 for not readable,
>0 for readable.
*/
int SockStatus(int socket, int seconds);
 
#endif /* #ifndef NO_PROTO */

#endif /* SOCKET__ */
