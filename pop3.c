/* Copyright 1993,1994 by Carl Harris, Jr.
 * All rights reserved
 *
 * Distribute freely, except: don't remove my name from the source or
 * documentation (don't take credit for my work), mark your changes (don't
 * get me blamed for your possible bugs), don't alter or remove this
 * notice.  May be sold if buildable source is provided to buyer.  No
 * warrantee of any kind, express or implied, is included with this
 * software; use at your own risk, responsibility for damages (if any) to
 * anyone resulting from the use of this software rests entirely with the
 * user.
 *
 * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
 * I'll try to keep a version up to date.  I can be reached as follows:
 * Carl Harris <ceharris@vt.edu>
 */


/***********************************************************************
  module:       pop3.c
  program:      popclient
  SCCS ID:      @(#)pop3.c	2.4  3/31/94
  programmer:   Carl Harris, ceharris@vt.edu
  date:         29 December 1993
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3 
  description:  POP2 client code.
 ***********************************************************************/
 
#include  <stdio.h>
#include  <sys/time.h>
#include  <unistd.h>
#include  <ctype.h>
#include  <errno.h>

#include  "socket.h"
#include  "config.h"
#include  "popclient.h"


/* requisite, venerable SCCS ID string */
static char sccs_id [] = "@(#)pop3.c	2.4\t3/31/94";

/* TCP port number for POP2 as defined by RFC 937 */
#define	  POP3_PORT	110

#ifndef NO_PROTO
/* prototypes for internal functions */
int POP3_OK (char *buf, int socket);
int POP3_auth (char *userid, char *password, int socket);
int POP3_sendQUIT (int socket);
int POP3_sendSTAT (int *msgcount, int socket);
int POP3_sendRETR (int msgnum, int socket);
int POP3_sendDELE (int msgnum, int socket);
int POP3_readmsg (int socket, int mboxfd, int topipe);
#endif


/*********************************************************************
  function:      doPOP3
  description:   retrieve messages from the specified mail server
                 using Post Office Protocol 3.

  arguments:     
    options      fully-specified options (i.e. parsed, defaults invoked,
                 etc).

  return value:  exit code from the set of PS_.* constants defined in 
                 popclient.h
  calls:
  globals:       reads outlevel.
 *********************************************************************/

int doPOP3 (options)
struct optrec *options;
{
  int ok;
  int mboxfd;
  char buf [POPBUFSIZE];
  int socket;
  int number,count;


  /* open the folder if we're not using the system mailbox */
  if (options->foldertype != OF_SYSMBOX) 
    if ((mboxfd = openuserfolder(options)) < 0) 
      return(PS_FOLDER);
    
  /* open the socket and get the greeting */
  if ((socket = Socket(options->host,POP3_PORT)) < 0) {
    perror("doPOP3: socket");
    return(PS_SOCKET);
  }

  ok = POP3_OK(buf,socket);
  if (ok != 0) {
    if (ok != PS_SOCKET)
      POP3_sendQUIT(socket);
    close(socket);
    return(ok);
  }

  /* print the greeting */
  if (outlevel > O_SILENT && outlevel < O_VERBOSE) 
    fprintf(stderr,"%s\n",buf);
  else 
    ;

  /* try to get authorized */
  ok = POP3_auth(options->userid,options->password,socket);
  if (ok == PS_ERROR)
    ok = PS_AUTHFAIL;
  if (ok != 0)
    goto cleanUp;

  /* find out how many messages are waiting */
  ok = POP3_sendSTAT(&count,socket);
  if (ok != 0) {
    goto cleanUp;
  }

  /* show them how many messages we'll be downloading */
  if (outlevel > O_SILENT && outlevel < O_VERBOSE)
    fprintf(stderr,"%d messages in folder\n",count);
  else
    ;

  if (count > 0) { 
    for (number = 1;  number <= count;  number++) {

      /* open the mail pipe if we're using the system mailbox */
      if (options->foldertype == OF_SYSMBOX) {
        ok = (mboxfd = openmailpipe(options)) < 0 ? -1 : 0;
        if (ok != 0)
          goto cleanUp;
      }
           
      ok = POP3_sendRETR(number,socket);
      if (ok != 0)
        goto cleanUp;
        
      ok = POP3_readmsg(socket,mboxfd,options->foldertype == OF_SYSMBOX);
      if (ok != 0)
        goto cleanUp;

      if (!options->keep) {
        ok = POP3_sendDELE(number,socket);
        if (ok != 0)
          goto cleanUp;
      }
      else
        ; /* message is kept */

      /* close the mail pipe if we're using the system mailbox */
      if (options->foldertype == OF_SYSMBOX) {
        ok = closemailpipe(mboxfd);
        if (ok != 0)
          goto cleanUp;
      }

    }
    ok = POP3_sendQUIT(socket);
    if (ok == 0)
      ok = PS_SUCCESS;
    close(socket);
    return(ok);
  }
  else {
    ok = POP3_sendQUIT(socket);
    if (ok == 0)
      ok = PS_NOMAIL;
    close(socket);
    return(ok);
  }

cleanUp:
  if (ok != 0 && ok != PS_SOCKET)
    POP3_sendQUIT(socket);

  if (options->foldertype != OF_SYSMBOX)
    if (closeuserfolder(mboxfd) < 0 && ok == 0)
      ok = PS_FOLDER;
    
  if (ok == PS_FOLDER || ok == PS_SOCKET) 
    perror("doPOP3: cleanUp");

  return(ok);
}



/*********************************************************************
  function:      POP3_OK
  description:   get the server's response to a command, and return
                 the extra arguments sent with the response.
  arguments:     
    argbuf       buffer to receive the argument string.
    socket       socket to which the server is connected.

  return value:  zero if okay, else return code.
  calls:         SockGets
  globals:       reads outlevel.
 *********************************************************************/

int POP3_OK (argbuf,socket)
char *argbuf;
int socket;
{
  int ok;
  char buf [POPBUFSIZE];
  char *bufp;

  if (SockGets(socket, buf, sizeof(buf)) == 0) {
    if (outlevel == O_VERBOSE)
      fprintf(stderr,"%s\n",buf);

    bufp = buf;
    if (*bufp == '+' || *bufp == '-')
      bufp++;
    else
      return(PS_PROTOCOL);

    while (isalpha(*bufp))
      bufp++;
    *(bufp++) = '\0';

    if (strcmp(buf,"+OK") == 0)
      ok = 0;
    else if (strcmp(buf,"-ERR") == 0)
      ok = PS_ERROR;
    else
      ok = PS_PROTOCOL;

    if (argbuf != NULL)
      strcpy(argbuf,bufp);
  }
  else 
    ok = PS_SOCKET;

  return(ok);
}



/*********************************************************************
  function:      POP3_auth
  description:   send the USER and PASS commands to the server, and
                 get the server's response.
  arguments:     
    userid       user's mailserver id.
    password     user's mailserver password.
    socket       socket to which the server is connected.

  return value:  non-zero if success, else zero.
  calls:         SockPrintf, POP3_OK.
  globals:       read outlevel.
 *********************************************************************/

int POP3_auth (userid,password,socket) 
char *userid, *password;
int socket;
{
  int ok;
  char buf [POPBUFSIZE];

  SockPrintf(socket,"USER %s\r\n",userid);
  if (outlevel == O_VERBOSE)
    fprintf(stderr,"> USER %s\n",userid);

  if ((ok = POP3_OK(buf,socket)) == 0) {
    SockPrintf(socket,"PASS %s\r\n",password);
    if (outlevel == O_VERBOSE)
      fprintf(stderr,"> PASS password\n");

    if ((ok = POP3_OK(buf,socket)) == 0) 
      ;  /* okay, we're approved.. */
    else if (outlevel > O_SILENT && outlevel < O_VERBOSE)
      fprintf(stderr,"%s\n",buf);
    else
      ; /* say nothing */
  }
  else if (outlevel > O_SILENT && outlevel < O_VERBOSE)
    fprintf(stderr,"%s\n",buf);
  else
    ; /* say nothing */
  
  return(ok);
}




/*********************************************************************
  function:      POP3_sendQUIT
  description:   send the QUIT command to the server and close 
                 the socket.

  arguments:     
    socket       socket to which the server is connected.

  return value:  none.
  calls:         SockPuts, POP3_OK.
  globals:       reads outlevel.
 *********************************************************************/

int POP3_sendQUIT (socket)
int socket;
{
  int ok;
  char buf [POPBUFSIZE];

  SockPuts(socket,"QUIT");

  if (outlevel == O_VERBOSE)
    fprintf(stderr,"> QUIT\n");
  else
    ;

  ok = POP3_OK(buf,socket);
  if (ok != 0 && outlevel > O_SILENT && outlevel < O_VERBOSE)
    fprintf(stderr,"%s\n",buf);

  return(ok);
}



/*********************************************************************
  function:      POP3_sendSTAT
  description:   send the STAT command to the POP3 server to find
                 out how many messages are waiting.
  arguments:     
    count        pointer to an integer to receive the message count.
    socket       socket to which the POP3 server is connected.

  return value:  return code from POP3_OK.
  calls:         POP3_OK, SockPrintf
  globals:       reads outlevel.
 *********************************************************************/

int POP3_sendSTAT (msgcount,socket)
int *msgcount;
int socket;
{
  int ok;
  char buf [POPBUFSIZE];
  int totalsize;

  SockPrintf(socket,"STAT\r\n");
  if (outlevel == O_VERBOSE)
    fprintf(stderr,"> STAT\n");
  
  ok = POP3_OK(buf,socket);
  if (ok == 0)
    sscanf(buf,"%d %d",msgcount,&totalsize);
  else if (outlevel > O_SILENT && outlevel < O_VERBOSE)
    fprintf(stderr,"%s\n",buf);

  return(ok);
}




/*********************************************************************
  function:      POP3_sendRETR
  description:   send the RETR command to the POP3 server.
  arguments:     
    msgnum       message ID number
    socket       socket to which the POP3 server is connected.

  return value:  return code from POP3_OK.
  calls:         POP3_OK, SockPrintf
  globals:       reads outlevel.
 *********************************************************************/

int POP3_sendRETR (msgnum,socket)
int msgnum;
int socket;
{
  int ok;
  char buf [POPBUFSIZE];

  SockPrintf(socket,"RETR %d\r\n",msgnum);
  if (outlevel == O_VERBOSE)
    fprintf(stderr,"> RETR %d\n",msgnum);
  
  ok = POP3_OK(buf,socket);
  if (ok != 0 && outlevel > O_SILENT && outlevel < O_VERBOSE)
    fprintf(stderr,"%s\n",buf);

  return(ok);
}



/*********************************************************************
  function:      POP3_sendDELE
  description:   send the DELE command to the POP3 server.
  arguments:     
    msgnum       message ID number
    socket       socket to which the POP3 server is connected.

  return value:  return code from POP3_OK.
  calls:         POP3_OK, SockPrintF.
  globals:       reads outlevel.
 *********************************************************************/

int POP3_sendDELE (msgnum,socket)
int msgnum;
int socket;
{
  int ok;
  char buf [POPBUFSIZE];

  SockPrintf(socket,"DELE %d\r\n",msgnum);
  if (outlevel == O_VERBOSE)
    fprintf(stderr,"> DELE %d\n",msgnum);
  
  ok = POP3_OK(buf,socket);
  if (ok != 0 && outlevel > O_SILENT && outlevel < O_VERBOSE)
    fprintf(stderr,"%s\n",buf);

  return(ok);
}



/*********************************************************************
  function:      POP3_readmsg
  description:   Read the message content as described in RFC 1225.
  arguments:     
    socket       ... to which the server is connected.
    mboxfd       open file descriptor to which the retrieved message will
                 be written.  
    topipe       true if we're writing to the system mailbox pipe.

  return value:  zero if success else PS_* return code.
  calls:         SockGets.
  globals:       reads outlevel. 
 *********************************************************************/

int POP3_readmsg (socket,mboxfd,topipe)
int socket;
int mboxfd;
int topipe;
{
  char buf [MSGBUFSIZE];
  char *bufp;
  int lines,sizeticker;
  time_t now;
  /* This keeps the retrieved message count for display purposes */
  static int msgnum = 0;  

  /* set up for status message if outlevel allows it */
  if (outlevel > O_SILENT && outlevel < O_VERBOSE) {
    fprintf(stderr,"reading message %d",++msgnum);
    /* won't do the '...' if retrieved messages are being sent to stdout */
    if (mboxfd == 1)
      fputs(".\n",stderr);
    else
      ;
  }
  else
    ;

  /* Unix mail folder format requires the Unix-syntax 'From' header.
     POP3 doesn't send it, so we fake it here. */
  if (!topipe) {  
    now = time(NULL);
    sprintf(buf,"From POPmail %s",ctime(&now));
    if (write(mboxfd,buf,strlen(buf)) < 0) {
      perror("POP3_readmsg: write");
      return(PS_FOLDER);
    }
  }


  /* read the message content from the server */
  lines = 0;
  sizeticker = MSGBUFSIZE;
  while (1) {
    if (SockGets(socket,buf,sizeof(buf)) < 0)
      return(PS_SOCKET);
    bufp = buf;
    if (*bufp == '.') {
      bufp++;
      if (*bufp == 0)
        break;  /* end of message */
    }
    strcat(bufp,"\n");
    if (write(mboxfd,bufp,strlen(bufp)) < 0) {
      perror("POP3_readmsg: write");
      return(PS_FOLDER);
    }

    sizeticker -= strlen(bufp);
    if (sizeticker <= 0) {
      if (outlevel > O_SILENT && outlevel < O_VERBOSE)
        fputc('.',stderr);
      sizeticker = MSGBUFSIZE;
    }
    lines++;
  }

  if (!topipe) {
    /* The server may not write the extra newline required by the Unix
       mail folder format, so we write one here just in case */
    if (write(mboxfd,"\n",1) < 0) {
      perror("POP3_readmsg: write");
      return(PS_FOLDER);
    }
  }
  else {
    /* The mail delivery agent may require a terminator.  Write it if
       it has been defined */
#ifdef BINMAIL_TERM
    if (write(mboxfd,BINMAIL_TERM,strlen(BINMAIL_TERM)) < 0) {
      perror("POP3_readmsg: write");
      return(PS_FOLDER);
    }
#endif
    }

  /* finish up display output */
  if (outlevel == O_VERBOSE)
    fprintf(stderr,"(%d lines of message content)\n",lines);
  else if (outlevel > O_SILENT && mboxfd != 1) 
    fputs(".\n",stderr);
  else
    ;
  return(0);
}
