#!/bin/bash
##
## Wrapper for mpd-state to make it behave like state-utils.
## Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
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
