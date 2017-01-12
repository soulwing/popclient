# /* Copyright 1993,1994 by Carl Harris, Jr.
#  * All rights reserved
#  *
#  * Distribute freely, except: don't remove my name from the source or
#  * documentation (don't take credit for my work), mark your changes (don't
#  * get me blamed for your possible bugs), don't alter or remove this
#  * notice.  May be sold if buildable source is provided to buyer.  No
#  * warrantee of any kind, express or implied, is included with this
#  * software; use at your own risk, responsibility for damages (if any) to
#  * anyone resulting from the use of this software rests entirely with the
#  * user.
#  *
#  * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
#  * I'll try to keep a version up to date.  I can be reached as follows:
#  * Carl Harris <ceharris@vt.edu>
#  */


#######################################################################
#  POPclient project Makefile
#
#  SCCS ID:     @(#)Makefile	2.5  3/31/94
#  programmer:  Carl Harris, ceharris@vt.edu
#  date:        29 December 1993
#
 

#######################################################################
#  Installer configuration section
#  (be sure to edit config.h, too)
BINDIR=/usr/local/bin
MANDIR=/usr/local/man
MANSECTION=1

#######################################################################
#  Compiler configuration section
#  K & R compilers may want to set CFLAGS=-DNO_PROTO
#
CFLAGS=
CC=cc
#  System V may require 'LIBS=-lbsd' here.
LIBS=
TARGET=popclient
OBJECTS=popclient.o pop2.o pop3.o socket.o


#######################################################################
#  Term configuration section.  For term connection to internet only.
#
# TERM_PRIORITY = 0
# TERM_COMPRESS = 0
# TERMDIR = $(HOME)/term
# CFLAGS += -DUSE_TERM -DTERM_PRIORITY=$(TERM_PRIORITY) -DTERM_COMPRESS=$(TERM_COMPRESS) -I$(TERMDIR)
# OBJECTS2 = $(TERMDIR)/client.a


#######################################################################
#  Down to the real business...
#
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(OBJECTS2) -o $(TARGET) $(LIBS)

popclient.o: popclient.c popclient.h socket.h config.h

pop2.o: pop2.c popclient.h socket.h config.h

pop3.o: pop3.c popclient.h socket.h config.h

socket.o: socket.c socket.h config.h

install:
	install -c -m 0755 popclient $(BINDIR)
	install -c -m 0644 popclient.$(MANSECTION)L $(MANDIR)/man$(MANSECTION)

clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET)
	rm -f core
