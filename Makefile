##
## Tiny Aplication Collection Makefile
## $Id: Makefile,v 1.1 2005/07/02 15:30:00 mina86 Exp $
##


##
## Configuration
##
CC           = cc
CFLAGS      ?= -Os -pipe -DNDEBUG -DG_DISABLE_ASSERT -s -fomit-frame-pointer
LDFLAGS     ?= -s -z combreloc
X11_INC_DIR  = /usr/X11R6/include
X11_LIB_DIR  = /usr/X11R6/lib


##
## List of all applications
##
all: FvwmTransFocus cdiff cutcom load malloc mpd-state quotes \
     the-book-of-mozilla timer tuptime


##
## Make rules
##
FvwmTransFocus: FvwmTransFocus.c
	${CC} ${CFLAGS} -I${X11_INC_DIR} -L${X11_LIB_DIR} -lX11 -o $@ $<

mpd-state: mpd-state.o libmpdclient.o
	${CC} ${LDFLAGS} $< libmpdclient.o -o $@

quotes: quotes.txt
	egrep -v ^\# $< >$@

the-book-of-mozilla: the-book-of-mozilla.txt
	sed -e '/^#/d' -e '/^%/c%' $< >$@


##
## Clean rules
##
clean:
	xargs rm <.cvsignore 2>/dev/null || true
	rm *.o *~ 2>/dev/null || true

distclean: clean
