#!/bin/sh
##
## Pings specified host and runs specified program if no responding
## Copyright (c) 2005-2008 by Michal Nazarewicz (mina86/AT/mina86.com)
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
	echo 'Check v0.2  (c) 2005,2006 by Michał Nazarewicz'
	echo
}


##
## Usage
##
usage () {
	cat <<EOF
usage: check.sh [ <options> ] [ [--] <action> [ <arg> ... ] ]
<options> are:
  -h --help            Displays help screen and exits
  -H --long-help       Displays long help screen and exits
  -V --version         Displays vesion and exits
  -q --quiet           Displays less messages
  -r --retry=<count>   Runs <action> after <count> failed checkings       [10]
  -i --interval=<int>  Waits <int> seconds betwenn each checking          [10]
  -c --check=<check>   Runs <check> to check       [check_ping www.google.com]
  -k --keep-going      Restarts after running action                      [no]
  -t --trap=<trpap>    Executes <trap> when signal is trapped         [exit 0]
  -T --ignore-sig      Ignores signals                                    [no]

  Single letter options may not be given as one argument (eg. -kTq).

<action>  Program to be run when check faileds                          [exit]
<arg> ... Arguments to the <action>                                        [1]

EOF
	if [ "X$1" != "Xlong" ]; then return 0; fi
cat <<EOF

<check> must exit with 0 exit code if check succeeded or non-zero otherwise.
Script contains the fallowing built-in check functions:
 check_ping <host>             -- checks if <host> answers to ping
 check_proc_load <proc> <load> -- checks if <proc>'s CPU load is below <load>
                                  (<load> must be an integer betwen 1 and 100)
 check_proc <proc>             -- checks if <proc> is running
Each built-in function has also an alias with the first c letter removed.

The script exports variables CHECK_QUIET (empty if false, y if true),
CHECK_INGORESIG (empty if false, y if true), (in case of <action>)
CHECK_EXIT_CODE (exit code of <check>) and CHECK_TMP (path to a temporary
file) which both <check> and <action> should take into account.

Examples:

  ./check.sh '-check_proc_load X 95' -k -- killall X -q &
    Kills X each time it uses more then 95% of CPU for 5 minutes.  Usefull
    when executed from system start scripts since it keeps running silently in
    the background.

  ./check.sh || /sbin/halt
    Halts computer if www.google.com stops responding to pings (most likely
    connection was lsot).  May be canceled with Ctrl+C.

  ./check.sh '-check_proc foo' -k -q -r100 -i1 -- /path/to/foo' &
    Checks whether foo is runing and if not executes it.  Useful when executed
    from system start scripts since it keeps running silently in the
    background.

EOF
}


##
## Init variables
##
umask 077
SLEEP=10
RETRY=10

KEEPGOING=
CHECK_QUIET=
CHECK_IGNORESIG=
export CHECK_QUIET CHECK_IGNORESIG
TRAPCMD="exit 0"
CMD="check_ping www.google.com"


##
## Parse arguments
##
while [ $# -ne 0 ]; do
	case "$1" in
	-h|--help)      version; usage     ; exit 0; ;;
	-H|--long-help) version; usage long; exit 0; ;;
	-V|--version)   version;             exit 0; ;;

	-q|--quiet)      CHECK_QUIET=yes  ; ;;
	-k|--keep-going) KEEPGOING=yes    ; ;;
	-T|--ignore-sig) CHECK_IGNORESIG=y; ;;

	-r|--retry)    RETRY=$2  ; shift; ;;
	-i|--interval) SLEEP=$2  ; shift; ;;
	-c|--count)    CMD=$2    ; shift; ;;
	-t|--trap)     TRAPCMD=$2; shift; ;;

	-r*) RETRY=${1#-?}  ; ;; (--retry=*)    RETRY=${1#-*=}  ; ;;
	-i*) SLEEP=${1#-?}  ; ;; (--interval=*) SLEEP=${1#-*=}  ; ;;
	-c*) CMD=${1#-?}    ; ;; (--check=*)    CMD=${1#-*=}    ; ;;
	-t*) TRAPCMD=${1#-?}; ;; (--trap=*)     TRAPCMD=${1#-*=}; ;;

	--) shift; break; ;;
	-*) echo Unknown option: "$1"; exit 1; ;;
	*) break; ;;
	esac
	shift
done
[ $# -eq 0 ] && set -- exit 1


[ -n "$CHECK_QUIET" ] || version


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
## Checking function
##
check_ping () {
	ping -c 1 "$1" 2>/dev/null >"$CHECK_TMP" || return 1
	[ -n "$CHECK_QUIET" ] || head -n 2 "$CHECK_TMP" |tail -n 1
	return 0
}
heck_ping () { check_ping "$@"; }

check_proc_load () {
	LOAD="$(ps -o %cpu -C "$1" |tail -n+2)"
	if [ ! "$LOAD" ]; then
		[ -n "$CHECK_QUIET" ] || printf '%s: not running\n' "$1"
		return 0
	fi
	[ -n "$CHECK_QUIET" ] || printf '%s: CPU Load %5s%%\n' "$1" "$LOAD"

	[ "${LOAD%%.*}" -lt "$2" ]
}
heck_proc_load () { check_proc_load "$@"; }

check_proc () {
	if [ -z "$__CHECK_PROC_PKILL_PRESENT" ]; then
		which pkill >/dev/null 2>&1
		__CHECK_PROC_PKILL_PRESENT=$?
	fi

	if [ $__CHECK_PROC_PKILL_PRESENT -eq 0 ]; then
		if ! pkill -0 "^$1\$"; then return 1; fi
	elif ! ps -C "$1" >/dev/null 2>/dev/null; then
		return 1
	fi

	if [ -n "$CHECK_QUIET" ]; then return 0; fi
	case "$__CHECK_PROC_GLOBAL_VAR" in
	(1) printf '%s: running  (/)\r'  "$1"; __CHECK_PROC_GLOBAL_VAR=2; ;;
	(2) printf '%s: running  (-)\r'  "$1"; __CHECK_PROC_GLOBAL_VAR=3; ;;
	(3) printf '%s: running  (\\)\r' "$1"; __CHECK_PROC_GLOBAL_VAR=0; ;;
	(*) printf '%s: running  (|)\r'  "$1"; __CHECK_PROC_GLOBAL_VAR=1; ;;
	esac
	return 0
}
heck_proc () { check_proc "$@"; }


##
## Temporary file
##

# Find temp dir
CHECK_TMP=.
for N in "$TMPDIR" "$TMP" "$TEMP" ~/tmp /tmp; do
	if [ -n "$N" ] && [ -d "$N" ] && [ -w "$N" ]; then
		CHECK_TMP="$N"
		break
	fi
done

# tempfile exists
if which mktemp >/dev/null 2>&1; then
	export CHECK_TMP=$(mktemp "$CHECK_TMP/check-XXXXXXXX")
elif which tempfile >/dev/null 2>&1; then
	export CHECK_TMP=$(tempfile -d "$CHECK_TMP" -p "check-")
else
	echo unable to create temporary file, mktemp or tempfile required >&2
	exit 0
fi

trap '[ -f "$CHECK_TMP" ] && rm -f -- "$CHECK_TMP"' 0


##
## Signals
##
catch_sig () {
	if [ -n "$CHECK_IGNORESIG" ] || [ "X$1" = XUSR1 ] ||
		[ "X$1" = XUSR2 ] || [ "X$1" = XALRM ]; then
		[ -n "$CHECK_QUIET" ] || echo "Got SIG$1; ignoring..."
		return 0
	fi

	[ -n "$CHECK_QUIET" ] || echo "Got SIG$1"
	$TRAPCMD
}

for e in HUP INT QUIT ABRT SEGV PIPE ALRM TERM USR1 USR2; do
	# shellcheck disable=SC2064
	trap "catch_sig $e" "$e"
done



##
## Loop
##
count=0
while :; do
	## OK
	if $CMD; then
		count=0
		sleep "$SLEEP"
		continue
	fi

	## Not OK
	export CHECK_EXIT_CODE=$?
	count=$(( count + 1 ))
	[ -n "$CHECK_QUIET" ] || printf " !!! %3d / %3d\r" "$count" "$RETRY"

	## Limit reached
	if [ "$count" -ge "$RETRY" ]; then
		[ -n "$CHECK_QUIET" ] || echo ' !!! Checking failed.'
		[ -f "$CHECK_TMP" ] && rm -f -- "$CHECK_TMP"
		"$@"
		[ -n "$KEEPGOING" ] || exit 0;
		count=0
	fi

	sleep "$SLEEP"
done
