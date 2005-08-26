#!/bin/bash
##
## Pings specified host and runs specified program if no responding
## $Id: check.sh,v 1.3 2005/08/26 16:17:52 mina86 Exp $
## Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/tlen.pl)
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##

##
## I use it to shut down my PC when my brother shuts down his PC which
## is a router.  It may be also used to run a reconnect script when
## connection is loast.  It can be also used to monitor another PC and
## send (let say) SMS when it goes down.
##

##
## Version
##
version () {
	echo 'Check v0.1  (c) 2005 by Micha³ Nazarewicz'
	echo '$Id: check.sh,v 1.3 2005/08/26 16:17:52 mina86 Exp $'
	echo
}


##
## Usage
##
usage () {
	cat <<EOF
usage: check.sh [ <options> ] [ [--] <action> [ <arg> ... ] ]
<options> are:
  -h --help            Displays this screen and exits
  -V --version         Displays vesion and exits
  -q --quiet           Displays less messages
  -r --retry=<count>   Runs <action> after <count> failed checkings       [10]
  -i --interval=<int>  Waits <int> seconds betwenn each checking          [10]
  -c --check=<check>   Runs <check> to check       [check_ping www.google.com]
  -k --keep-going      Restarts after running action                      [no]
  -t --trap=<trpap>    Executes <trap> when signal is trapped         [exit 0]
  -T --ignore-sig      Ignores signals                                    [no]

  If you use short option which requires an arumgnet (eg. '-c') the argument
  must be specified just after the option without whitespace or anything
  (eg. '-c10')

<action>  Program to be run when there is no connection                 [exit]
<arg>     Arguments to the <action>                                        [1]

<check> should exit with zero exit code if check succeeded or non-zero if not.
Use --check='check_ping <host>' to ping <host> instead of www.google.com.

The script exports variables CHECK_QUIET (empty if false, y if true),
CHECK_INGORESIG (empty if false, y if true), (in case of <action>)
CHECK_EXIT_CODE (exit code of <check>) and CHECK_TMP (path to a temporary
file) which both <check> and <action> should take into account.

By default the script caches all signals and exits with zero error code if
signal is caught so you can run './chec.sh || /sbin/halt' to automaticly shut
down the machine when connection is lost and then hit Ctrl-C to abort.

EOF
}


##
## Init variables
##
declare -i SLEEP=10 RETRY=10
KEEPGOING=
export CHECK_QUIET=
TRAPCMD="exit 0"
export CHECK_IGNORESIG=
CMD="check_ping www.google.com"


##
## Parse arguments
##
while [ $# -ne 0 ]; do
	case "$1" in
	(-h|--help)    version; usage; exit 0; ;;
	(-V|--version) version;        exit 0; ;;

	(-q|--quiet)      CHECK_QUIET=yes  ; ;;
	(-k|--keep-going) KEEPGOING=yes    ; ;;
	(-T|--ignore-sig) CHECK_IGNORESIG=y; ;;

	(-r*) RETRY="${1:2}"; ;; (--retry=*)    RETRY="${1:8}" ; ;;
	(-i*) SLEEP="${1:2}"; ;; (--interval=*) SLEEP="${1:11}"; ;;
	(-c*) CMD="${1:2}"  ; ;; (--check=*)    CMD="${1:8}"   ; ;;
	(-t*) TRAP="${1:2}" ; ;; (--trap=*)     TRAP="${1:7}"  ; ;;

	(--) shift; break; ;;
	(-*) echo Unknown option: "$1"; exit 1; ;;
	(*) break; ;;
	esac
	shift;
done
[ $# -eq 0 ] && set -- exit 1


[ "$CHECK_QUIET" ] || version


##
## Check arguments
##
if [ "$RETRY" -lt 1 ]; then
	echo '<count> must be integer >= 1'
	exit 2;
fi
if [ "$SLEEP" -lt 1 ]; then
	echo '<interval> must be integer >= 1'
	exit 2;
fi


##
## Init some more variables
##
export CHECK_TMP=check-connection-$$-$RANDOM
[ -d /tmp -a -w /tmp ] && CHECK_TMP=/tmp/$CHECK_TMP
declare -i COUNT=0


##
## Checking function
##
check_ping () {
	ping -c 1 "$1" 2>/dev/null >"$CHECK_TMP" || return 1
	[ $CHECK_QUIET ] || head -n 2 "$CHECK_TMP" |tail -n 1
	return 0
}


##
## Signals
##
catch_sig () {
	if [ "$CHECK_IGNORESIG" -o $1 == USR1 -o $1 == USR2 -o $1 == ALRM ]; then
		[ $CHECK_QUIET ] || echo "Got SIG$1; ignoring..."
		return 0
	fi

	[ $CHECK_QUIET ] || echo "Got SIG$1; terminating..."
	[ -f "$TMP" ] && rm -f -- "$TMP"
	$TRAPCMD
}

for E in HUP INT QUIT ABRT SEGV PIPE ALRM TERM USR1 USR2; do
	trap "catch_sig $E" $E
done


##
## Loop
##
while true; do
	## OK
	if $CMD; then
		COUNT=0
		sleep $SLEEP
		continue
	fi

	## Not OK
	export CHECK_EXIT_CODE=$?
	COUNT=$(( $COUNT + 1 ))
	[ $QUIET ] || printf " !!! %3d / %3d\r" "$COUNT" "$RETRY"

	## Limit reached
	if [ $COUNT -ge $RETRY ]; then
		[ $QUIET ] || echo ' !!! Checking failed.'
		[ -f "$CHECK_TMP" ] && rm -f -- "$CHECK_TMP"
		"$@"
		[ $KEEPGOING ] || exit 0;
		COUNT=0
	fi

	sleep $SLEEP
done
