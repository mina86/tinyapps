#!/bin/bash
##
## Wrapper for mpd-state to make it behave like state-utils.
## $Id: mpd-state-wrapper.sh,v 1.2 2005/07/11 00:20:58 mina86 Exp $
## Copyright (c) 2005 by Michal Nazareicz (mina86/AT/tlen.pl)
## Licensed under the Academic Free License version 2.1.
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


case "`basename "$0"`" in
	(state-save)
	mkdir -p ~/.mpd_states && mpd-state >~/.mpd_states/${1-default}
	;;

	(state-restore)
	if [ -f "~/.mpd_states/${1-default}" ]; then
		mpd-state -r <"~/.mpd_sates/${1-default}"
	else
		echo File "~/.mpd_states/${1-default}" does not exist >&2
		exit 1
	fi
	;;

	(state-sync)
	mpd-state | mpd-state -r "${1-localhost}" "${2-6600}"
	;;

	(state-amend)
	if [ -f "~/.mpd_states/${1-default}" ]; then
		mpd-state -raP <"~/.mpd_sates/${1-default}"
	else
		echo File "~/.mpd_states/${1-default}" does not exist >&2
		exit 1
	fi
	;;

	(*)
	echo This script should be run as one of:
	echo state-save, state-restore, state-sync or state-amend
	exit 1
	;;
esac
