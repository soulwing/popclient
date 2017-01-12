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
  module:       config.h
  program:      popclient
  SCCS ID:      @(#)config.h	1.2  3/31/94
  programmer:   Carl Harris, ceharris@vt.edu
  date:         30 March 94
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3 
  description:  configuration defines.
 ***********************************************************************/


/***** POP defaults *****/

/* default protocol (2 = POP2, 3 = POP3) */
#define  DEF_PROTOCOL	2       


/***** Mail Delivery Agent (MDA) definitions  *****/

/* fully-qualified pathname for the MDA (usually /bin/mail) */
#define  MDA_PATH	"/bin/mail" 

/* passed as argv[0] to the MDA (usually just the basename of the mailer) */
#define  MDA_ALIAS	"mail"

/* MDA command args; $u passes user's login id (usually "-d $u") */
/* (may be undefined if the mailer doesn't require arguments (?!)) */
#define  MDA_ARGS	"-d $u"

/* MDA command arg count -- must be greater than or equal to the number 
   of space-delimited arguments defined in MDA_ARGS. */
#ifdef MDA_ARGS
#define MDA_ARGCOUNT	8
#endif
   
/* MDA input message terminator (usually not required) */
/* (may be undefined if the mailer doesn't require it) */
/* #define MDA_TERM  ".\n" */

