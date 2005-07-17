#!/bin/sh
##
## Pings specified host and runs specified program if no responding
## $Id: check-connection.sh,v 1.1 2005/07/17 13:46:35 mina86 Exp $
## Copyright (c) 2005 by Michal Nazareicz (mina86/AT/tlen.pl)
## Licensed under the Academic Free License version 2.1.
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
	echo 'Check Connection v0.1  (c) 2005 by Micha³ Nazarewicz'
	echo '$Id: check-connection.sh,v 1.1 2005/07/17 13:46:35 mina86 Exp $'
	echo
}


##
## Usage
##
usega () {
	echo <<EOF
usage: check-connction.sh [ <options> ] [ [--] <action> [ <arg> ... ] ]
<options> are:
  -h --help                 Displays this screen and exits
  -V --version              Displays vesion and exits
  -c --count=<count>        Number of failed pings befor runing <action>  [10]
  -i --interval=<interval>  Delay (in seconds) between each ping          [10]
  -h --host=<host>          Host to ping                      [www.google.com]
  -k --keep-going           Restart after running action

  If you use short option which requires an arumgnet (eg. '-c') the argument
  must be specified just after the option without whitespace or anything
  (eg. '-c10')

<action>  Program to be run when there is no connection           [/sbin/halt]
<arg>     Arguments to the <action>
EOF
}


##
## Init variables
##
HOST=www.google.com
SLEEP=10
RETRY=10
KEEPGOING=
QUIET=


##
## Parse arguments
##
while [ $# -ne 0 ]; do
	case "$1" in
	(-h|--help) version; usage; exit 0; ;;
	(-V|--version) version; exit 0; ;;

	(-q|--quiet) QUIET=yes; ;;
	(-k|--keep-going) KEEPGOING=yes; ;;

	(-c*) RETRY="${1:2}"; ;;
	(--count=*) RETRY="${1:8}"; ;;

	(-i*) SLEEP="${1:2}"; ;;
	(--interval=*) SLEEP="${1:11}"; ;;

	(-h*) HOST="${1:2}"; ;;
	(--host=*) HOST="${1:7}"; ;;

	(--) shift; break; ;;
	(-*) echo Unknown option: "$1"; exit 1; ;;
	(*) break; ;;
	esac
	shift;
done


[ "$QUIET" ] || version


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
TMP=check-connection-$$-$RANDOM
[ -d /tmp -a -w /tmp ] && TMP=/tmp/$TMP
COUNT=0


##
## Signal
##
sig_int () {
	[ -f "$TMP" ] && rm -f -- "$TMP"
	exit 0
}
trap sig_int INT


##
## Loop
##
while true; do
	if ping -c 1 "$HOST" 2>/dev/null >"$TMP"; then
		if [ ! $QUIET ]; then
			head -n 2 "$TMP" |tail -n 1
		fi
		COUNT=0

	else
		COUNT=$(( $COUNT + 1 ))
		[ $QUIET ] || printf " !!! %3d / %3d\r" "$COUNT" "$RETRY"

		if [ $COUNT -ge "$RETRY" ]; then
			[ $QUIET ] || echo ' !!! No connection.'
			rm -f -- "$TMP"
			if [ $# -eq 0 ]; then /sbin/halt; else "$@"; fi
			[ $KEEPGOING ] || exit 0;
			COUNT=0
		fi
	fi

	sleep -- "$SLEEP"
done
