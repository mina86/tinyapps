##
## Tiny Aplication Collection Makefile
## Copyright (c) 2005-2007,2013 by Michal Nazarewicz (mina86/AT/mina86.com)
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
## This software is OSI Certified Open Source Software.
## OSI Certified is a certification mark of the Open Source Initiative.
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##


##
## Configuration
##
CC          ?= cc
CXX         ?= c++
ifndef      DEBUG
CFLAGS      ?= -Os -pipe -DNDEBUG -DG_DISABLE_ASSERT -fomit-frame-pointer
CXXFLAGS    ?= -Os -pipe -DNDEBUG -DG_DISABLE_ASSERT -fomit-frame-pointer
LDFLAGS     ?= -s -z combreloc
else
CFLAGS      += -O0 -g -pipe
CXXFLAGS    += -O0 -g -pipe
CPPFLAGS    += -DDEBUG
endif
CPPFLAGS    += -Wall

ifndef      RELEASE
RELEASE     := $(shell if [ -f .release ]; \
                       then printf %s $$(cat .release); \
                       else date +%Y%m%d; fi)
endif


EUID        := $(shell id -u)
ifeq ($(EUID), 0)
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
	@echo '  package              creates tinyapps.txz binary package'
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
all: FvwmTransFocus arpping cdiff cutcom drun foreach infinite-logger		\
     installkernel.8.gz load malloc mpd-show mpd-state null			\
     rot13 timer tuptime umountiso xgetclass xrun


install: install-FvwmTransFocus install-add install-ai install-arpping		\
         install-changelog.pl install-cdiff install-check.sh			\
         install-checkmail install-cpuload.sh install-cutcom			\
         install-drun install-extractlinks.pl install-foreach			\
         install-genpass install-getlyrics.pl install-gz2bz			\
         install-inplace install-installkernel install-lesspipe			\
         install-load install-moz2elinks.pl install-mp3rip			\
         install-mpd-state install-null install-pingrange.pl			\
         install-rot13 install-rand-files.pl install-settitle			\
         install-show install-timer install-tpwd install-traf.sh		\
         install-virtman.sh install-xcolor2rgb install-xgetclass install-xrun


uninstall: uninstall-FvwmTransFocus uninstall-add uninstall-ai			\
           uninstall-arpping uninstall-changelog.pl uninstall-cdiff		\
           uninstall-check.sh uninstall-checkmail uninstall-cpuload.sh		\
           uninstall-cutcom uninstall-drun uninstall-extractlinks.pl		\
           uninstall-forceach uninstall-genpass					\
           uninstall-getlyrics.pl uninstall-gz2bz uninstall-ivona.sh		\
           uninstall-inplace uninstall-installkernel				\
           uninstall-lesspipe uninstall-load uninstall-moz2elinks.pl		\
           uninstall-mp3rip uninstall-mpd-state uninstall-null			\
           uninstall-pingrange.pl uninstall-rot13				\
           uninstall-rand-files.pl uninstall-settitle uninstall-show		\
           uninstall-timer uninstall-tpwd uninstall-traf.sh			\
           uninstall-virtman.sh uninstall-xcolor2rgb uninstall-xgetclass	\
           uninstall-xrun


##
## Make rules
##
%.o: %.c
	@echo '  CC     $@'
	$(Q)exec $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.o: %.cpp
	@echo '  CXX    $@'
	$(Q)exec $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

%: %.o
	@echo '  LD     $@'
	$(Q)exec $(CC) $(LDFLAGS) -o $@ $<

%: %.c
	@echo '  CC     $@.o'
	$(Q)exec $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@.o $<
	@echo '  LD     $@'
	$(Q)exec $(CC) $(LDFLAGS) -o $@ $@.o
	$(Q)exec rm -f -- $@.o

%: %.sh

FvwmTransFocus: FvwmTransFocus.o
	@echo '  LD     $@'
	$(Q)exec $(CC) $(LDFLAGS) `pkg-config --libs xorg-server` -o $@ $< -lX11

xrun brun Brun drun Drun: null
	@echo '  LN     $@'
	$(Q)exec ln -sf -- $< $@

mpd-%: mpd-%.o libmpdclient.o
	@echo '  LD     $@'
	$(Q)exec $(CC) $(LDFLAGS) $^ -o $@

installkernel.8.gz: installkernel.8
	@echo '  GZIP   $@'
	$(Q)exec gzip -9 <$< >$@

umountiso: mountiso
	@echo '  LNK    $@'
	$(Q)[ -f umountiso ] || ln -s mountiso umountiso

xgetclass: xgetclass.o
	@echo '  LD     $@'
	$(Q)exec $(CC) $(LDFLAGS) `pkg-config --libs xorg-server` -o $@ $< -lX11

genpass: genpass.pl
	@echo '  CP     $@'
	$(Q)cp -- $^ $@


##
## Install rules
##

# install owner,group,mode,dir,file
ifeq ("$(shell which install >/dev/null 2>&1 && echo yes)", "yes")
  define install
	@echo '  INST   $(notdir $5)'
	$(Q)$(RT)exec install -D -o $1 -g $2 -m $3 $5 $(DEST_DIR)$4/$(notdir $5)
	$(Q)$(NT)exec install -D             -m $3 $5 $(DEST_DIR)$4/$(notdir $5)
  endef
 else
  define install
	@echo '  INST   $(notdir $5)'
	$(Q)exec mkdir -p -m 0755 -- $(DEST_DIR)$4
	$(Q)exec cp -f -- $5    $(DEST_DIR)$4/$(notdir $5)
	$(Q)$(RT)chown $1:$2 -- $(DEST_DIR)$4/$(notdir $5) 2>/dev/null || true
	$(Q)exec chmod $3    -- $(DEST_DIR)$4/$(notdir $5)
  endef
endif


install-%: %
	$(call install,root,bin,0755,/usr/local/bin,$<)

install-FvwmTransFocus: FvwmTransFocus
ifeq ("$(shell which fvwm-config >/dev/null 2>&1 && echo yes)", "yes")
	$(call install,root,bin,0755,$(shell fvwm-config -m),$<)
else
	$(warning fvwm-config not found. Not installing FvwmTransFocus.)
endif

install-ai: install-ai-pitr.pl install-ai-sid.pl

install-ai-pitr.pl: ai-pitr.pl
	$(call install,root,root,0755,/usr/local/games,$<)

install-ai-sid.pl: ai-sid.pl
	$(call install,root,root,0755,/usr/local/games,$<)

install-arpping: arpping
	$(call install,root,bin,0755,/usr/local/sbin,$<)

install-gz2bz: gz2bz
	$(call install,root,bin,0755,/usr/local/bin,$<)
	$(Q)for f in z gz bz xz lz; do for t in z gz bz xz lz; do \
		if [ $$f != $$t ] && [ $${f}2$$t != gz2bz ]; then \
			echo "  LNK    $${f}2$$t"; \
			ln -fs -- gz2bz $(DEST_DIR)/usr/local/bin/$${f}2$$t; \
		fi; \
	done; done

install-installkernel: installkernel installkernel.8.gz
	$(call install,root,bin,0755,/usr/local/sbin,$<)
	$(call install,root,root,0644,/usr/local/man/man8,$(addprefix $<,.8.gz))

install-mountiso: mountiso umountiso
	$(call install,root,root,4750,/usr/local/bin/,mountiso)
	$(call install,root,root,0750,/usr/local/bin/,umountiso)

install-mpd-state: mpd-state
	$(call install,root,bin,0755,/usr/local/bin,$<)
	$(call install,root,bin,0755,/usr/local/bin,$(addprefix $<,-wrapper.sh))
	$(Q)for lnk in state-save state-restore state-sync state-amend; do \
		echo "  LNK    $$lnk"; \
		ln -fs -- mpd-state-wrapper.sh $(DEST_DIR)/usr/local/bin/$$lnk; \
	done

install-show: show
	$(call install,root,bin,0755,/usr/local/sbin,$<)

install-xgetclass: xgetclass
	$(call install,root,root,0755,/usr/X11/bin,$<)



##
## Uninstall rules
##
define uninstall
	@echo '  UNINST $(notdir $(1))'
	$(Q)rm -f -- $(DEST_DIR)$(1) 2>/dev/null || true
endef

uninstall-%:
	$(call uninstall,/usr/local/bin/$*)

uninstall-FvwmTransFocus:
ifeq ("$(shell which fvwm-config >/dev/null 2>&1 && echo yes)", "yes")
	$(call uninstall,$(shell fvwm-config -m)/FvwmTransFocus)
else
	$(warning fvwm-config not found. Not uninstalling FvwmTransFocus.)
endif

uninstall-ai: uninstall-ai-pitr.pl uninstall-ai-sid.pl

uninstall-ai-pitr.pl:
	$(call uninstall,/usr/local/games/ai-pitr.pl)

uninstall-ai-sid.pl:
	$(call uninstall,/usr/local/games/ai-sid.pl)

uninstall-arpping:
	$(call uninstall,/usr/local/sbin/arpping)

uninstall-installkernel:
	$(call uninstall,/usr/local/sbin/installkernel)
	$(call uninstall,/usr/local/man/man8/installkernel.8.gz)

uninstall-mountiso:
	$(call uninstall,/usr/local/bin/mountiso)
	$(call uninstall,/usr/local/bin/umountiso)

uninstall-mpd-state:
	$(call uninstall,/usr/local/bin/mpd-state)
	$(call uninstall,/usr/local/bin/mpd-state-wrapper.sh)
	$(call uninstall,/usr/local/bin/state-save)
	$(call uninstall,/usr/local/bin/state-restore)
	$(call uninstall,/usr/local/bin/state-sync)
	$(call uninstall,/usr/local/bin/state-amend)

uninstall-show:
	$(call uninstall,/usr/local/sbin/show)

uninstall-xgetclass: xgetclass
	$(call install,root,root,0755,/usr/X11/bin,$<)



##
## Clean rules
##
clean:
	@echo '  CLEAN  compiled files'
	$(Q)exec rm -f -- $(shell cat .gitignore)
	@echo '  CLEAN  temporary files'
	$(Q)exec rm -f -- *.o *~ 2>/dev/null
	@echo '  CLEAN  package release'
	$(Q)exec rm -rf -- package tinyapps*/
	@echo '  CLEAN  empty files and directories'
	$(Q)exec find -maxdepth 1 -empty -exec rm -rf {} \;

distclean: clean



##
## Make package
##
package: DEST_DIR = $(PWD)/package
package: all install
ifneq ($(EUID), 0)
	@$(warning It is best to create package as root.)
endif

	@echo '  RM     state-save state-restore state-sync state-amend'
	$(Q)exec rm -f -- '$(DEST_DIR)/usr/local/bin/state-save'		\
	                  '$(DEST_DIR)/usr/local/bin/state-restore'		\
	                  '$(DEST_DIR)/usr/local/bin/state-sync'		\
	                  '$(DEST_DIR)/usr/local/bin/state-amend'
	@echo '  RM     umountiso'
	$(Q)rm -f -- '$(DEST_DIR)/bin/umountiso'

	@echo '  GEN    usr/doc/tinyapps-$(RELEASE)'
	$(Q)exec mkdir -p -- '$(DEST_DIR)/usr/doc/tinyapps-$(RELEASE)'
	$(Q)exec cp -- LICENSE LICENSE.AFL LICENSE.GPL3 README TODO '$(DEST_DIR)/usr/doc/tinyapps-$(RELEASE)/'

	@echo '  GEN    install/doinst.sh'
	$(Q)exec mkdir -p -- '$(DEST_DIR)/install'
	$(Q)echo 'cd usr/local/bin' >'$(DEST_DIR)/install/doinst.sh'
	$(Q)echo 'for FILE in state-save state-restore state-sync state-amend; do ln -fs mpd-state-wrapper.sh $$FILE; done' >>'$(DEST_DIR)/install/doinst.sh'
#	$(Q)echo 'ln -fs mountiso umountiso; chown root:bin mountiso' >>'$(DEST_DIR)/install/doinst.sh'
	$(Q)exec chmod 755 -- '$(DEST_DIR)/install/doinst.sh'
	@echo '  CP     slack-desc'
	$(Q)exec cp -- slack-desc '$(DEST_DIR)/install'

	@echo '  PACK   tinyapps-$(RELEASE)-'"$${ARCH:-$$(uname -m)}"'-1mn.txz'
	$(Q)TAR="`which tar-1.13 2>/dev/null`"; \
		exec "$${TAR:-tar}" c -C '$(DEST_DIR)' \
			--owner=root --group=root . | \
		xz -9 > 'tinyapps-$(RELEASE)-'"$${ARCH:-$$(uname -m)}"'-1mn.txz'

	@echo '  CLEAN  package'
	$(Q)exec rm -rf -- '$(DEST_DIR)'



##
## Make release
##
release: distclean
	@echo '  CP     *'
	$(Q)exec mkdir -p -- 'tinyapps-$(RELEASE)'
	$(Q)for FILE in *; do [ "X$$FILE" != XCVS ] && \
		[ "X$$FILE" != 'Xtinyapps-$(RELEASE)' ] && \
		cp -Rf -- "$$FILE" 'tinyapps-$(RELEASE)'; done
	$(Q)exec echo '$(RELEASE)' >'tinyapps-$(RELEASE)/.release'

	@echo '  TARBZ  tinyapps-$(RELEASE).tar.bz2'
	$(Q)exec tar c 'tinyapps-$(RELEASE)' | bzip2 -9 >'tinyapps-$(RELEASE).tar.bz2'

	@echo '  RM     tinyapps-$(RELEASE)'
	$(Q)exec rm -rf -- 'tinyapps-$(RELEASE)'
