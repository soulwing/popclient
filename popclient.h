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
  module:       popclient.h
  program:      popclient
  SCCS ID:      @(#)popclient.h	2.3  4/1/94
  programmer:   Carl Harris, ceharris@vt.edu
  date:         29 December 1993
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3
  description:  global constant, type, and variable definitions.
 ***********************************************************************/
 

/* definitions for buffer sizes -- somewhat arbitrary */
#define		POPBUFSIZE	512	/* per RFC 937 */
#define		MSGBUFSIZE	1024   	/* size of message read buffer */
#define		HOSTLEN		128	/* max hostname length */
#define		USERIDLEN	32	/* max user-length */
#define		PASSWORDLEN	32      /* max password length */
#define		FOLDERLEN	256     /* max folder name length */


/* exit code values */
#define		PS_SUCCESS	0	/* successful receipt of messages */
#define		PS_NOMAIL       1	/* no mail available */
#define		PS_SOCKET	2	/* socket I/O woes */
#define		PS_AUTHFAIL	3	/* user authorization failed */
#define		PS_PROTOCOL	4	/* protocol violation */
#define		PS_SYNTAX	5	/* command-line syntax error */
#define		PS_FOLDER	6	/* local folder I/O woes */
#define		PS_ERROR	7	/* some kind of POP3 error condition */
#define		PS_UNDEFINED	9	/* something I hadn't thought of */


/* output noise level */
#define         O_SILENT	0	/* mute, max squelch, etc. */
#define		O_NORMAL	1	/* user-friendly */
#define		O_VERBOSE	2	/* excessive */

/* output folder type, used in options record */
#define		OF_SYSMBOX	1	/* use system default mailbox */
#define		OF_USERMBOX	2	/* use user's specified mailbox */
#define		OF_STDOUT	3	/* use stdout */

/* Command-line arguments are passed in this structure type */
struct optrec {
  int versioninfo;
  int keep;
  int verbose;
  int whichpop;
  int silent;
  int foldertype;
  char host [HOSTLEN];
  char loginid [USERIDLEN];
  char userid [USERIDLEN];
  char password [PASSWORDLEN];
  char userfolder [FOLDERLEN];
  char remotefolder [FOLDERLEN];
};

/* Controls the detail of status/progress messages written to stderr */
extern int outlevel;     /* see the O_.* constants above */

#ifndef NO_PROTO

/* prototypes for globally callable functions */
int doPOP2 (struct optrec *options); 
int doPOP3 (struct optrec *options);

int openuserfolder (struct optrec *options);
int closeuserfolder (int fd);
int openmailpipe (struct optrec *options);
int closemailpipe (int fd);

#endif

