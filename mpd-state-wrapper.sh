#!/bin/bash
##
## Wrapper for mpd-state to make it behave like state-utils.
## $Id: mpd-state-wrapper.sh,v 1.6 2008/11/09 00:02:21 mina86 Exp $
## Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
## Licensed under the Academic Free License version 3.0.
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##

#
# To install copy this file to (eg.) /usr/local/bin and make the
# fallowing symlinks: state-save, state-restore, state-sync and
# state-amend all pointing to this wrapper.
#

if [ "$1" == "--help" ]; then
	cat<<EOF
usage: state-save    [ <state-name> ]
       state-restore [ <state-name> ]
       state-sync    [ <mpd-host> [ <mpd-port> ] ]
       state-amend   [ <state-name> ]
state-save saves MPD state to a file.
state-restores resotres saved state to MPD.
state-sync tries to sync two MPD servers as close as possible.
state-amend adds playlist from specified state to MPD.
EOF
	exit 0;
fi


ARG0=${0##*/}
SHIFTED=
while true; do case "$ARG0" in
	(state-save|save)
	mkdir -p ~/.mpd_states && mpd-state >~/.mpd_states/${1-default}
	exit $?
	;;

	(state-restore|restore)
	if [ -f "~/.mpd_states/${1-default}" ]; then
		mpd-state -r <"~/.mpd_sates/${1-default}"
		exit $?
	else
		echo File "~/.mpd_states/${1-default}" does not exist >&2
		exit 1
	fi
	;;

	(state-sync|sync)
	mpd-state | mpd-state -r "${1-localhost}" "${2-6600}"
	exit $?
	;;

	(state-amend|amend)
	if [ -f "~/.mpd_states/${1-default}" ]; then
		mpd-state -raP <"~/.mpd_sates/${1-default}"
		exit $?
	else
		echo File "~/.mpd_states/${1-default}" does not exist >&2
		exit 1
	fi
	;;

	(*)
	if [ -z "$SHIFTED" ] && [ $# -ne 0 ]; then
		SHIFTED=y
		ARG0="$1"
		shift
	else
		echo 'This script should be run as one of:'
		echo '  state-save, state-restore, state-sync or state-amend'
		echo 'or with one of those strings as firt argument optionaly w/o state- prefix'
		exit 1
	fi
	;;
esac; done
