##
## Tiny Aplication Collection Makefile
## $Id: Makefile,v 1.5 2005/08/13 22:21:03 mina86 Exp $
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
                       /opt/X11R6 /usr /usr/local/include; do [ -f		\
                       "$$DIR/lib/libX11.so" ] && echo "$$DIR/lib" &&	\
                       break; done}

EUID        := ${shell echo $$EUID}
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
	@echo '  tinyapps.tgz         synonym of package'
	@echo '  uninstall            uninstalls all utilities'
	@echo
	@echo '  <utility>            builds <utility>'
	@echo '  install-<utility>    installs <utility>'
	@echo '  uninstall-<utility>  uninstalls <utility>'
	@echo
	@echo 'Possible options:'
	@echo '  V=0|1                0 - quiet build (default), 1 - verbose build'
	@echo '  DEST_DIR=<dir>       install to/uninstall from <dir>'


##
## List of all applications
##
all: FvwmTransFocus cdiff cutcom infinite-logger installkernel.8.gz		\
     load malloc mpd-state null quotes the-book-of-mozilla rot13 timer	\
     tuptime umountiso


install: install-FvwmTransFocus install-add install-ai install-cdiff	\
         install-cdiff.sed install-check.sh install-checkmail			\
         install-cpuload.sh install-cutcom install-get_mks_vir_bases	\
         install-installkernel install-load install-malloc				\
         install-moz2elinks.pl install-mountiso install-mpd-state		\
         install-null install-fortune install-rot13 install-settitle	\
         install-timer install-traf.sh install-xcolor2rgb


uninstall: uninstall-FvwmTransFocus uninstall-add uninstall-ai			\
           uninstall-cdiff uninstall-cdiff.sed uninstall-check.sh		\
           uninstall-checkmail uninstall-cpuload.sh uninstall-cutcom	\
           uninstall-get_mks_vir_bases uninstall-installkernel			\
           uninstall-load uninstall-malloc uninstall-mountiso			\
           uninstall-moz2elinks.pl uninstall-mountiso					\
           uninstall-mpd-state uninstall-null uninstall-fortune			\
           uninstall-rot13 uninstall-settitle uninstall-timer			\
           uninstall-traf.sh uninstall-xcolor2rgb



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
	${Q}${CC} ${LDFLAGS} -L${X11_LIB_DIR} -lX11 -o $@ $<

mpd-state: mpd-state.o libmpdclient.o
	@echo '  LD     $@'
	${Q}${CC} ${LDFLAGS} $< libmpdclient.o -o $@

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



##
## Clean rules
##
clean:
	@echo '  CLEAN  compiled files'
	${Q}xargs rm <.cvsignore 2>/dev/null || true
	@echo '  CLEAN  temporary files'
	${Q}rm *.o *~ 2>/dev/null || true
	@echo '  CLEAN  package'
	${Q}[ -d package ] && rm -rf package || true
	${Q}[ -f tinyapps.tgz ] && rm -f tinyapps.tgz || true

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
	${Q}rm -f -- ${DEST_DIR}/usr/local/bin/state-save		\
	             ${DEST_DIR}/usr/local/bin/state-restore	\
	             ${DEST_DIR}/usr/local/bin/state-sync		\
	             ${DEST_DIR}/usr/local/bin/state-amend
	@echo '  RM     umountiso'
	${Q}rm -f -- ${PWD}/package/bin/umountiso

	@echo '  GEN    install/doinst.sh'
	${Q}mkdir -p -- ${DEST_DIR}/install
	${Q}echo 'cd usr/local/bin; for FILE in state-save state-restore state-sync state-amend; do ln -fs mpd-state-wrapper.sh $FILE; done' >${DEST_DIR}/install/doinst.sh
	${Q}echo 'cd ../../../bin; ln -s mountiso umountiso; chown root:root mountiso; chmod u+s mountiso; cd ..' >>${DEST_DIR}/install/doinst.sh
	${Q}chmod 755 -- ${DEST_DIR}/install/doinst.sh
	@echo '  CP     slack-desc'
	${Q}cp -- slack-desc ${DEST_DIR}/install

	@echo '  TAR    tinyapps.tar'
	${Q}${RT}tar c -C ${DEST_DIR} --format=v7 install bin usr >tinyapps.tar
	${Q}${NR}tar c -C ${DEST_DIR} --owner=root --group=root --format=v7 install bin usr >tinyapps.tar
	@echo '  GZIP   tinyapps.tgz'
	${Q}gzip -9 <tinyapps.tar >tinyapps.tgz
	${Q}rm -f -- tinyapps.tar

	@echo '  RM     package'
	${Q}rm -rf -- ${DEST_DIR}
