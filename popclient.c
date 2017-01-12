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
  module:       popclient.c
  program:      popclient
  SCCS ID:      @(#)popclient.c	2.5  4/1/94
  programmer:   Carl Harris, ceharris@vt.edu
  date:         29 December 1993
  compiler:     DEC RISC C compiler (Ultrix 4.1)
  environment:  DEC Ultrix 4.3 
  description:  main driver module for the popclient package.  contains
                code for command-line parsing and invocation of the
                appropriate POP driver routine.
 ***********************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "config.h"
#include "popclient.h"

/* release info */
#define         COMPILE_SCCS	"2.21 [2.5] (4/1/94)"

/* requisite, venerable SCCS ID string */
static char sccs_id [] = "@(#)popclient.c	2.5\t4/1/94";

#ifndef NO_PROTO
/* prototypes for internal functions */
int showoptions (struct optrec *options);
int parseMDAargs (struct optrec *options);
int parsecmdline (int argc, char **argv, struct optrec *options);
int setdefaults (struct optrec *options);
int showversioninfo (void);
int clearbiffbit (void);
int restorebiffbit (void);
#endif

/* Controls the detail of status/progress messages written to stderr */
int outlevel;      /* see the O_.* constants in popclient.h */
int biffwas = -1;  /* previous chmod bits on the controlling tty */

/* args for the MDA, parsed out in the usual fashion by parseMDAargs() */
#ifdef MDA_ARGS
char *mda_argv [MDA_ARGCOUNT + 2];
#else
char *mda_argv [2];
#endif


/*********************************************************************
  function:      main
  description:   main driver routine 
  arguments:     
    argc         argument count as passed by runtime startup code.
    argv         argument strings as passed by runtime startup code.

  return value:  an exit status code for the shell -- see the 
                 PS_.* constants defined above.
  calls:         parsecmdline, setdefaults, openuserfolder, doPOP2.
  globals:       none.
 *********************************************************************/

main (argc,argv)
int argc;
char **argv;
{ 
  int mboxfd;
  struct optrec options;
  int popstatus;
  int cleanup();

  signal(SIGINT,cleanup);
  clearbiffbit();

  if (parsecmdline(argc,argv,&options) == 0)
    if (!options.versioninfo)
      if (setdefaults(&options) == 0) {
        parseMDAargs(&options);
        if (options.whichpop == 3) 
          popstatus = doPOP3(&options);
        else
          popstatus = doPOP2(&options);
      } 
      else
        popstatus = PS_UNDEFINED;
    else
      showversioninfo();
  else
    popstatus = PS_SYNTAX;

  restorebiffbit();
  exit(popstatus);
}
    

 
/*********************************************************************
  function:      parsecmdline
  description:   parse/validate the command line options.
  arguments:
    argc         argument count.
    argv         argument strings.
    options      pointer to a struct optrec to receive the parsed 
                 options.

  return value:  zero if the command line was syntactically correct,
                 else non-zero (an error message has been written to stderr).  
  calls:         none.  
  globals:       none.  
 *********************************************************************/

int parsecmdline (argc,argv,options)
int argc;
char **argv;
struct optrec *options;
{
  int c,i;
  int fflag = 0;     /* TRUE when -o or -c has been specified */
  int errflag = 0;   /* TRUE when a syntax error is detected */
  extern int optind,opterr;     /* defined in getopt(2) */
  extern char *optarg;          /* defined in getopt(2) */

  bzero(options,sizeof(struct optrec));    /* start clean */
  while (!errflag && (c = getopt(argc,argv,"23Vkvscu:p:f:o:")) != EOF) {
    switch (c) {
      case '2':
        options->whichpop = 2;
        break;
      case '3':
        options->whichpop = 3;
        break;
      case 'V':
        options->versioninfo = !0;
        break;
      case 'k':
        options->keep = !0;
        break;
      case 'v':
        options->verbose = !0;
        break;
      case 's':
        options->silent = !0;
        break;
      case 'c':
        if (fflag)
          errflag++;
        else {
          fflag++;
          options->foldertype = OF_STDOUT;
        }
        break;
      case 'u':
        strcpy(options->userid,optarg);
        break;
      case 'p':
        strcpy(options->password,optarg);
        break;
      case 'o':
        if (fflag) 
          errflag++;
        else {
          fflag++;
          options->foldertype = OF_USERMBOX;
          strcpy(options->userfolder,optarg);
        }
        break;
      case 'f':
        strcpy(options->remotefolder,optarg);
        break;
      default:
        errflag++;
    }
  }

  if (!options->versioninfo) {
    /* get host argument -- error if not present */
    if (optind < argc)
      strcpy(options->host,argv[optind++]);
    else
      errflag++;

    /* error if more unparsed arguments are present */
    if (optind < argc)
      errflag++;
    else
      ;
  }
  else 
    ;


  /* squawk if syntax errors were detected */
  if (errflag) {
    fputs("usage:  popclient [-2|-3] [-Vksv] [-u server-userid] [-p server-password]\n",
          stderr);
    fputs("                  [-f remote-folder] [-c | -o local-folder]  host\n",
          stderr);
  }
  else
    ;

  /* clean up the command line in case -p was used */
  for (i = 1; i < argc; i++)
    argv[i] = (char *) 0;

  return(errflag);
}
         

/*********************************************************************
  function:      setdefaults
  description:   set reasonable default values for unspecified options.
  arguments:     
    options      option values parsed from the command-line; unspeci-
                 fied options must be filled with zero.

  return value:  zero if defaults were successfully set, else non-zero
                 (indicates a problem reading /etc/passwd).
  calls:         none.
  globals:       writes outlevel.
 *********************************************************************/

int setdefaults (options)
struct optrec *options;
{
  int uid;
  struct passwd *pw;
  char *mailvar;

  if ((pw = getpwuid(uid = getuid())) == NULL) {
    fprintf(stderr,"No passwd entry for uid %d\n",uid);
    return(-1);
  }
  /* save the login name for delivery use */
  strcpy(options->loginid,pw->pw_name);

  if (options->whichpop == 0)
    options->whichpop = DEF_PROTOCOL;
  else
    ; /* -2 or -3 was specified */

  if (*(options->userid) == '\0') 
    strcpy(options->userid,pw->pw_name);
  else
    ;  /* -u was specified */

  if (options->foldertype == 0) 
    options->foldertype = OF_SYSMBOX;
  else
    ;  /* -o or -c was specified */
    
  if (*(options->password) == '\0')  
    strcpy(options->password,(char *) getpass("Enter mailserver password: "));
  else
    ;  /* -p was specified */

  if (options->verbose)
    outlevel = O_VERBOSE;
  else if (options->silent)
    outlevel = O_SILENT;
  else
    outlevel = O_NORMAL;

  return(0);
}



/*********************************************************************
  function:      showversioninfo
  description:   display program release and compiler info
  arguments:     none.
  return value:  none.
  calls:         none.
  globals:       none.
 *********************************************************************/

int showversioninfo()
{
  printf("popclient release %s\n",COMPILE_SCCS);
}



/*********************************************************************
  function:      openuserfolder
  description:   open the file to which the retrieved messages will
                 be appended.  Do NOT call when options->foldertype
                 is OF_SYSMBOX.

  arguments:     
    options      fully-determined options (i.e. parsed, defaults invoked,
                 etc).

  return value:  file descriptor for the open file, else -1.
  calls:         none.
  globals:       none.
 *********************************************************************/

int openuserfolder (options)
struct optrec *options;
{
  int fd;

  if (options->foldertype == OF_STDOUT)
    return(1);
  else    /* options->foldertype == OF_USERMBOX */
    if ((fd = open(options->userfolder,O_CREAT|O_WRONLY|O_APPEND,0600)) >= 0) {
      return(fd);
    }
    else {
      perror("popclient: openuserfolder: open()");
      return(-1);
    }
  
}



/*********************************************************************
  function:      openmailpipe
  description:   open a one-way pipe to the mail delivery agent.
  arguments:     
    options      fully-determined options (i.e. parsed, defaults invoked,
                 etc).

  return value:  open file descriptor for the pipe or -1.
  calls:         none.
  globals:       reads mda_argv.
 *********************************************************************/

int openmailpipe (options)
struct optrec *options;
{
  int pipefd [2];
  int childpid;
  char binmailargs [80];

  if (pipe(pipefd) < 0) {
    perror("popclient: openmailpipe: pipe");
    return(-1);
  }
  if ((childpid = fork()) < 0) {
    perror("popclient: openmailpipe: fork");
    return(-1);
  }
  else if (childpid == 0) {

    /* in child process space */
    close(pipefd[1]);  /* close the 'write' end of the pipe */
    close(0);          /* get rid of inherited stdin */
    if (dup(pipefd[0]) != 0) {
      fputs("popclient: openmailpipe: dup() failed\n",stderr);
      exit(1);
    }

    execv(MDA_PATH,mda_argv);

    /* if we got here, an error occurred */
    perror("popclient: openmailpipe: exec");
    return(-1);

  }

  /* in the parent process space */
  close(pipefd[0]);  /* close the 'read' end of the pipe */
  return(pipefd[1]);
}



/*********************************************************************
  function:      closeuserfolder
  description:   close the user-specified mail folder.
  arguments:     
    fd           mail folder descriptor.

  return value:  zero if success else -1.
  calls:         none.
  globals:       none.
 *********************************************************************/

int closeuserfolder(fd)
int fd;
{
  int err;

  if (fd != 1) {   /* not stdout */
    err = close(fd);
  }   
  else
    err = 0;
  
  if (err)
    perror("popclient: closeuserfolder: close");

  return(err);
}



/*********************************************************************
  function:      closemailpipe
  description:   close pipe to the mail delivery agent.
  arguments:     
    options      fully-determined options record
    fd           pipe descriptor.

  return value:  0 if success, else -1.
  calls:         none.
  globals:       none.
 *********************************************************************/

int closemailpipe (fd)
int fd;
{
  int err;
  int childpid;

  err = close(fd);
  childpid = wait((union wait *) 0);
  if (err)
    perror("popclient: closemailpipe: close");

  return(err);
}



/*********************************************************************
  function:      parseMDAargs
  description:   parse the argument string given in MDA_ARGS into
                 a regular *argv[] array.
  arguments:
    options      fully-determined options record pointer.

  return value:  none.
  calls:         none.
  globals:       writes mda_argv.
 *********************************************************************/

int parseMDAargs (options)
struct optrec *options;
{
  int argi;
  char *argp;

  /* first put the MDA alias in as argv[0] */
  mda_argv[0] = MDA_ALIAS;
  
#ifdef MDA_ARGS

  /* make a writeable copy of MDA_ARGS */
  argp = strcpy((char *) malloc(strlen(MDA_ARGS)+1), MDA_ARGS);
  
  while (*argp != '\0' && isspace(*argp))	/* skip null first arg */
    argp++;					

  /* now punch nulls into the delimiting whitespace in the args */
  for (argi = 1;  
       *argp != '\0';
       argi++) {

    mda_argv[argi] = argp;     /* store pointer to this argument */

    /* find end of this argument */
    while (!(*argp == '\0' || isspace(*argp)))
      argp++;

    /* punch in a null terminator */
    if (*argp != '\0')
      *(argp++) = '\0';  
 
    /* check for macros */
    if (strcmp(mda_argv[argi],"$u") == 0)
      mda_argv[argi] = 
        strcpy((char *) malloc(strlen(options->loginid)+1),options->loginid);
    else
      ;  /* no macros to expand */

  }
  mda_argv[argi] = (char *) 0;

#else 

  mda_argv[1] = (char *) 0;

#endif

}



/*********************************************************************
  function:      clearbiffbit
  description:   clear the owner execute bit on the controlling tty, so
                 that biff messages do not appear on the terminal with
                 the program output.

  arguments:     none.
  return value:  none.
  calls:         none.
  globals:       writes biffwas
 *********************************************************************/

int clearbiffbit ()
{
  struct stat buf;
 
  if (fstat(0,&buf) < 0) {
    perror("popclient: clearbiffbit: fstat");  /* fuss but don't quit */
    return;
  }

  if (buf.st_mode & S_IFCHR) {
    buf.st_mode &= 0777;        /* isolate the device permissions */
    biffwas = buf.st_mode;	/* save for restore */
    buf.st_mode &= ~S_IEXEC;    /* clear the execute bit */
    if (fchmod(0,buf.st_mode) < 0) {
      perror("popclient: clearbiffbit: fchmod");  /* fuss but don't quit */
      return;
    }
  }
}




/*********************************************************************
  function:      restorebiffbit
  description:   restore the previous state of the owner execute bit
                 on the controlling tty.
  arguments:     none.
  return value:  none.
  calls:         none.
  globals:       reads biffwas
 *********************************************************************/

int restorebiffbit ()
{
  if (biffwas == -1)
    return;    /* not a tty */
  else {
    if (fchmod(0,biffwas) < 0) 
      perror("popclient: restorebiffbit: fchmod");   /* fuss but don't quit */
  }
}



/*********************************************************************
  function:      cleanup
  description:   signal catcher.
  arguments:     none.
  return value:  none.
  calls:         restorebiff
  globals:       reads biffwas.
 *********************************************************************/
    
int cleanup ()
{
  if (biffwas != -1) 
    restorebiffbit();
}


