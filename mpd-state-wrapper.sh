#!/bin/sh
## Wrapper for mpd-state to make it behave like state-utils.
## Copyright (c) 2005 by Michal Nazareicz <mina86@mina86.com>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, see <http://www.gnu.org/licenses/>.
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##

#
# To install copy this file to (eg.) /usr/local/bin and make the
# fallowing symlinks: state-save, state-restore, state-sync and
# state-amend all pointing to this wrapper.
#

if [ "$1" = "--help" ]; then
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
	mkdir -p ~/.mpd_states && mpd-state >~/.mpd_states/"${1-default}"
	exit $?
	;;

	(state-restore|restore)
	if [ -f ~/".mpd_states/${1-default}" ]; then
		mpd-state -r <~/.mpd_sates/"${1-default}"
		exit $?
	else
		# shellcheck disable=SC2088
		echo File "~/.mpd_states/${1-default}" does not exist >&2
		exit 1
	fi
	;;

	(state-sync|sync)
	mpd-state | mpd-state -r "${1-localhost}" "${2-6600}"
	exit $?
	;;

	(state-amend|amend)
	if [ -f ~/".mpd_states/${1-default}" ]; then
		mpd-state -raP <~/.mpd_sates/"${1-default}"
		exit $?
	else
		# shellcheck disable=SC2088
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
