##
## Tiny Aplication Collection Makefile
## $Id: Makefile,v 1.14 2005/11/27 16:57:38 mina86 Exp $
## Copyright (c) 2005 by Michal Nazareicz (mina86/AT/tlen.pl)
## Licensed under the Academic Free License version 2.1.
##


##
## Configuration
##
CC          ?= cc
CXX         ?= c++
CFLAGS      ?= -Os -pipe -DNDEBUG -DG_DISABLE_ASSERT -s -fomit-frame-pointer
CXXFLAGS    ?= -Os -pipe -DNDEBUG -DG_DISABLE_ASSERT -s -fomit-frame-pointer
LDFLAGS     ?= -s -z combreloc
X11_INC_DIR  = ${shell for DIR in /usr/X11R6 /usr/local/X11R6 /X11R6	\
                       /opt/X11R6 /usr /usr/local/include; do [ -f		\
                       "$$DIR/include/X11/X.h" ] && echo				\
                       "$$DIR/include" && break; done}
X11_LIB_DIR  = ${shell for DIR in /usr/X11R6 /usr/local/X11R6 /X11R6	\
                       /opt/X11R6 /usr /usr/local/include; do  for LIB	\
                       in lib64 lib; do [ -f "$$DIR/$$LIB/libX11.so" ]	\
                       && echo "$$DIR/lib" && break; done; done}

ifndef      RELEASE
RELEASE     := $(shell if [ -f .release ]; \
                       then printf %s $$(cat .release); \
                       else date +%Y%m%d; fi)
endif


EUID        := ${id -u}
ifeq (${EUID}, 0)
  RT        :=
  NR        := \#
else
  RT        := \#
  NR        :=
endif

Q           := @
ifeq ("$(origin V)", "command line")
  ifeq ("$V", "1")
    Q       :=
  endif
endif


##
## Help
##
.PHONY: help all install uninstall package help

help:
	@echo 'usage: make [options] [targets]'
	@echo
	@echo 'Possible targets:'
	@echo '  all                  compiles all utilities'
	@echo '  clean                removes all builds and temporary files'
	@echo '  distclean            at the moment synonym of clean'
	@echo '  install              installs all utilities'
	@echo '  package              creates tinyapps.tgz binary package'
	@echo '  release              prepares a release to publish'
	@echo '  uninstall            uninstalls all utilities'
	@echo
	@echo '  <utility>            builds <utility>'
	@echo '  install-<utility>    installs <utility>'
	@echo '  uninstall-<utility>  uninstalls <utility>'
	@echo
	@echo 'Possible options:'
	@echo '  V=0|1                0 - quiet build (default), 1 - verbose build'
	@echo '  DEST_DIR=<dir>       install to/uninstall from <dir>'
	@echo '  RELEASE=<YYYYMMDD>   release date of the package'


##
## List of all applications
##
all: FvwmTransFocus cdiff cutcom drun infinite-logger				\
     installkernel.8.gz load malloc mpd-state null quotes			\
     the-book-of-mozilla rot13 timer tuptime umountiso xgetclass


install: install-FvwmTransFocus install-add install-ai install-cdiff	\
         install-cdiff.sed install-check.sh install-checkmail			\
         install-cpuload.sh install-cutcom install-drun					\
         install-extractlinks.pl install-fortune install-genpass.sh		\
         install-get_mks_vir_bases install-getlyrics.pl install-gz2bz	\
         install-installkernel install-load install-malloc				\
         install-moz2elinks.pl install-mountiso install-mp3rip			\
         install-mpd-state install-null install-pingrange.pl			\
         install-rot13 install-settitle install-timer install-tpwd		\
         install-traf.sh install-xcolor2rgb install-xgetclass


uninstall: uninstall-FvwmTransFocus uninstall-add uninstall-ai			\
           uninstall-cdiff uninstall-cdiff.sed uninstall-check.sh		\
           uninstall-checkmail uninstall-cpuload.sh uninstall-cutcom	\
           uninstall-drun uninstall-extractlinks.pl uninstall-fortune	\
           uninstall-genpass.sh uninstall-get_mks_vir_bases				\
           uninstall-getlyrics.pl uninstall-gz2bz						\
           uninstall-installkernel uninstall-load uninstall-malloc		\
           uninstall-moz2elinks.pl uninstall-mountiso uninstall-mp3rip	\
           uninstall-mpd-state uninstall-null uninstall-pingrange.pl	\
           uninstall-rot13 uninstall-settitle uninstall-timer			\
           uninstall-tpwd uninstall-traf.sh uninstall-xcolor2rgb		\
           uninstall-xgetclass


##
## Make rules
##
%.o: %.c
	@echo '  CC     $@'
	${Q}${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

%.o: %.cpp
	@echo '  CXX    $@'
	${Q}${CXX} ${CXXFLAGS} ${CPPFLAGS} -c -o $@ $<

%: %.o
	@echo '  LD     $@'
	${Q}${CC} ${LDFLAGS} -o $@ $<

%: %.c
	@echo '  CC     $@.o'
	${Q}${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@.o $<
	@echo '  LD     $@'
	${Q}${CC} ${LDFLAGS} -o $@ $@.o
	${Q}rm -f -- $@.o

%: %.sh

FvwmTransFocus: FvwmTransFocus.o
	@echo '  LD     $@'
	${Q}${CC} ${LDFLAGS} "-L${X11_LIB_DIR}" -lX11 -o $@ $<

drun: null
	@echo '  LN     $@'
	${Q}ln -s null drun

mpd-show: mpd-show.o libmpdclient.o
	@echo '  LD     $@'
	${Q}${CC} ${LDFLAGS} $^ -o $@

mpd-state: mpd-state.o libmpdclient.o
	@echo '  LD     $@'
	${Q}${CC} ${LDFLAGS} $^ -o $@

quotes: quotes.txt
	@echo '  GEN    $@'
	${Q}egrep -v ^\# $< >$@

the-book-of-mozilla: the-book-of-mozilla.txt
	@echo '  GEN    $@'
	${Q}sed -e '/^#/d' -e '/^%/c%' $< >$@

installkernel.8.gz: installkernel.8
	@echo '  GZIP   $@'
	${Q}gzip -9 <$< >$@

umountiso: mountiso
	@echo '  LNK    $@'
	${Q}[ -f umountiso ] || ln -s mountiso umountiso

xgetclass: xgetclass.o
	@echo '  LD     $@'
	${Q}${CC} ${LDFLAGS} "-L${X11_LIB_DIR}" -lX11 -o $@ $<



##
## Install rules
##

# install owner,group,mode,dir,file
ifeq ("${shell which install >/dev/null 2>&1 && echo yes}", "yes")
  define install
	@echo '  INST   ${notdir $5}'
	${Q}${RT}install -D -o $1 -g $2 -m $3 $5 ${DEST_DIR}$4/${notdir $5}
	${Q}${NT}install -D             -m $3 $5 ${DEST_DIR}$4/${notdir $5}
  endef
 else
  define install
	@echo '  INST   ${notdir $5}'
	${Q}mkdir -p -m 0755 -- ${DEST_DIR}$4
	${Q}cp -f -- $5    ${DEST_DIR}$4/${notdir $5}
	${Q}${RT}chown $1:$2 -- ${DEST_DIR}$4/${notdir $5} 2>/dev/null || true
	${Q}chmod $3    -- ${DEST_DIR}$4/${notdir $5}
  endef
endif


install-%: %
	${call install,root,bin,0755,/usr/local/bin,$<}

install-FvwmTransFocus: FvwmTransFocus
	${warning FvwmTransFocus installer is not yet ready.}
	${warning You have to manualy copy FvwmTransFocus to the directory with FVWM modules.}

install-ai: install-ai-pitr.pl install-ai-sid.pl

install-ai-pitr.pl: ai-pitr.pl
	${call install,root,root,0755,/usr/games,$<}

install-ai-sid.pl: ai-sid.pl
	${call install,root,root,0755,/usr/games,$<}

install-installkernel: installkernel installkernel.8.gz
	${call install,root,bin,0755,/usr/local/bin,$<}
	${call install,root,root,0644,/usr/local/man/man8,${addprefix $<,.8.gz}}

install-mountiso: mountiso umountiso
	${call install,root,bin,4755,/bin/,mountiso}
	${call install,root,bin,4755,/bin/,umountiso}

install-mpd-state: mpd-state
	${call install,root,bin,0755,/usr/local/bin,$<}
	${call install,root,bin,0755,/usr/local/bin,${addprefix $<,-wrapper.sh}}
	@echo '  LNK    state-save'
	${Q}ln -fs -- mpd-state-wrapper.sh ${DEST_DIR}/usr/local/bin/state-save
	@echo '  LNK    state-restore'
	${Q}ln -fs -- mpd-state-wrapper.sh ${DEST_DIR}/usr/local/bin/state-restore
	@echo '  LNK    state-sync'
	${Q}ln -fs -- mpd-state-wrapper.sh ${DEST_DIR}/usr/local/bin/state-sync
	@echo '  LNK    state-amend'
	${Q}ln -fs -- mpd-state-wrapper.sh ${DEST_DIR}/usr/local/bin/state-amend

install-fortune: install-quotes install-the-book-of-mozilla

install-quotes: quotes
	${call install,root,root,0644,/usr/share/games/fortunes,$<}

install-the-book-of-mozilla: the-book-of-mozilla
	${call install,root,root,0644,/usr/share/games/fortunes,$<}

install-xgetclass: xgetclass
	${call install,root,root,0755,/usr/X11/bin,$<}



##
## Uninstall rules
##
define uninstall
	@echo '  UNINST ${notdir ${1}}'
	${Q}rm -f -- ${DEST_DIR}${1} 2>/dev/null || true
endef

uninstall-%:
	${call uninstall,/usr/local/bin/$*}

uninstall-FvwmTransFocus:
	${warning FvwmTransFocus uninstaller is not yet ready.}
	${warning You have to manualy remove FvwmTransFocus from the directory with FVWM modules.}

uninstall-ai: uninstall-ai-pitr.pl uninstall-ai-sid.pl

uninstall-ai-pitr.pl:
	${call uninstall,/usr/games/ai-pitr.pl}

uninstall-ai-sid.pl:
	${call uninstall,/usr/games/ai-sid.pl}

uninstall-installkernel:
	${call uninstall,/usr/local/bin/installkernel}
	${call uninstall,/usr/local/man/man8/installkernel.8.gz}

uninstall-mountiso:
	${call uninstall,/bin/mountiso}
	${call uninstall,/bin/umountiso}

uninstall-mpd-state:
	${call uninstall,/usr/local/bin/mpd-state}
	${call uninstall,/usr/local/bin/mpd-state-wrapper.sh}
	${call uninstall,/usr/local/bin/state-save}
	${call uninstall,/usr/local/bin/state-restore}
	${call uninstall,/usr/local/bin/state-sync}
	${call uninstall,/usr/local/bin/state-amend}

uninstall-fortune: uninstall-quotes uninstall-the-book-of-mozilla

uninstall-quotes:
	${call uninstall,/usr/share/games/fortunes/quotes}

uninstall-the-book-of-mozilla:
	${call uninstall,/usr/share/games/fortunes/the-book-of-mozilla}

uninstall-xgetclass: xgetclass
	${call install,root,root,0755,/usr/X11/bin,$<}



##
## Clean rules
##
clean:
	@echo '  CLEAN  compiled files'
	${Q}rm -f -- ${shell cat .cvsignore}
	@echo '  CLEAN  temporary files'
	${Q}rm -f -- *.o *~ 2>/dev/null
	@echo '  CLEAN  package release'
	${Q}rm -rf -- package tinyapps*/

distclean: clean



##
## Make package
##
tinyapps.tgz: package

package: DEST_DIR = ${PWD}/package
package: install
ifneq (${EUID}, 0)
	@${warning It is best to create package as root.}
endif

	@echo '  RM     state-save state-restore state-sync state-amend'
	${Q}rm -f -- '${DEST_DIR}/usr/local/bin/state-save'		\
	             '${DEST_DIR}/usr/local/bin/state-restore'	\
	             '${DEST_DIR}/usr/local/bin/state-sync'		\
	             '${DEST_DIR}/usr/local/bin/state-amend'
	@echo '  RM     umountiso'
	${Q}rm -f -- '${DEST_DIR}/bin/umountiso'

	@echo '  GEN    usr/doc/tinyapps-${RELEASE}'
	${Q}mkdir -p -- '${DEST_DIR}/usr/doc/tinyapps-${RELEASE}'
	${Q}cp -- LICENSE LICENSE.AFL README TODO ChangeLog '${DEST_DIR}/usr/doc/tinyapps-${RELEASE}/'
	${Q}cp -- LICENSE.gpl '${DEST_DIR}/usr/doc/tinyapps-${RELEASE}/LICENSE.GPL'

	@echo '  GEN    install/doinst.sh'
	${Q}mkdir -p -- '${DEST_DIR}/install'
	${Q}echo 'cd usr/local/bin; for FILE in state-save state-restore state-sync state-amend; do ln -fs mpd-state-wrapper.sh $FILE; done' >'${DEST_DIR}/install/doinst.sh'
	${Q}echo 'cd ../../../bin; ln -s mountiso umountiso; chown root:root mountiso; chmod u+s mountiso; cd ..' >>'${DEST_DIR}/install/doinst.sh'
	${Q}chmod 755 -- '${DEST_DIR}/install/doinst.sh'
	@echo '  CP     slack-desc'
	${Q}cp -- slack-desc '${DEST_DIR}/install'

	@echo '  TAR    tinyapps.tar'
	${Q}${RT}tar c -C '${DEST_DIR}' --format=v7 install bin usr >tinyapps.tar
	${Q}${NR}tar c -C '${DEST_DIR}' --owner=root --group=root --format=v7 install bin usr >tinyapps.tar
	@echo '  GZIP   tinyapps-{$RELEASE}.tgz'
	${Q}gzip -9 <'tinyapps.tar' >'tinyapps-${RELEASE}.tgz'
	${Q}mv -f -- tinyapps.tar

	@echo '  RM     package'
	${Q}rm -rf -- '${DEST_DIR}'



##
## Make release
##
release: distclean
	@echo '  CP     *'
	${Q}mkdir -p -- 'tinyapps-${RELEASE}'
	${Q}for FILE in *; do [ "X$$FILE" != XCVS ] && \
		[ "X$$FILE" != 'Xtinyapps-${RELEASE}' ] && \
		cp -Rf -- "$$FILE" 'tinyapps-${RELEASE}'; done
	${Q}echo '${RELEASE}' >'tinyapps-${RELEASE}/.release'

	@echo '  TARBZ  tinyapps-${RELEASE}.tar.bz2'
	${Q}tar c 'tinyapps-${RELEASE}' | bzip2 -9 >'tinyapps-${RELEASE}.tar.bz2'

	@echo '  RM     tinyapps-${RELEASE}'
	${Q}rm -rf -- 'tinyapps-${RELEASE}'
