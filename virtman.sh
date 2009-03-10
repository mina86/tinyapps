#!/bin/sh
##
## virtman.sh - Adds virtual hosts to Apache configuration file
## Copyright 2006 by Michal Nazarewicz (mina86/AT/mina86.com)
##
## This software is OSI Certified Open Source Software.
## OSI Certified is a certification mark of the Open Source Initiative.
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
## Defaults
##
set -e

HOST="$(hostname -f)"
IP='*'
PORT=80
PUBLIC_HTML=public_html
HTTPD_CONF=auto
LOGDIR=log

## Zero args
PREFIX=
SERVERNAME=
DOCROOT=
FULL_LOGDIR=

## Helper
err () { __FMT="$1"; shift; printf "%s: $__FMT\n" "${0##*/}" "$@" >&2; }


##
## Usage
##
if [ X"$1" = X--help ] || [ $# -eq 0 ]; then
	cat <<EOF
usage: virtman.sh [ -c httpd.conf ] [ -b ip:port ] [ -h host ] [ -v prefix ]
                  [ -V servername ] [ -d public_html ] [ -D docroot ]
                  [ -- ] user [ user ... ]

 -c http.conf    path to httpd.conf                      [auto]
 -b ip:port      an IP and port to listen to             [$IP:$PORT]
 -h host         hostname to suffix user name            [$HOST]
 -v prefix       prefix added to host name               [same as user]
 -V servername   full server name (overwrites -h and -v)
 -d public_html  name of user's document root            [$PUBLIC_HTML]
 -D docroot      full path to document root (overwrites -d)
 -l logdir       name of logdir in user's directory      [$LOGDIR]
 -L logdir       full path to log directory (overwrites -l)
    user         username to add virutal host for. May be used several
                 times if none of -v, -V, -D and -L were given.
EOF
	exit
fi


##
## Parse args
##
while [ X"${1#-}" != X"$1" ] && [ X"$1" != X-- ]; do

	## Split to arg and option
	if [ -z "${1#-?}" ]; then
		if [ $# -eq 1 ]; then
			err 'argument missing for %s' "$1"
			exit 1
		fi
		OPT="${1#-}"
		ARG="$2"
		shift 2
	else
		ARG="${1#??}"
		OPT="${1%$ARG}"
		OPT="${OPT#-}"
		shift
	fi

	## Check option
	case "$OPT" in
	(c) HTTPD_CONF="$ARG";;
	(h) HOST="$ARG";;
	(v) PREFIX="$ARG";;
	(V) SERVERNAME="$ARG";;
	(d) PUBLIC_HTML="$ARG";;
	(D) DOCROOT="$ARG";;
	(l) LOGDIR="$ARG";;
	(L) FULL_LOGDIR="$ARG";;
	(b)
		I="${ARG%:*}"
		P="${ARG##*:}"
		if [ -n "$I" ]; then IP="$I"; fi
		if [ X"$P" != X"$ARG" ]; then PORT="$P"; fi
		;;
	(*)
		err 'invalid option -%s' "$OPT"
		exit 1
		;;
	esac
done
if [ X"$1" = X-- ]; then shift; fi


##
## Validate number of users
##
if [ $# -eq 0 ]; then
	err 'user name required'
	exit 2
fi
if [ $# -gt 1 ] && [ -n "$SERVERNAME$DOCROOT$FULL_LOGDIR$PREFIX" ]; then
	err 'several user names cannot be used with -v, -V, -D or -L'
	exit 2
fi


##
## Find httpd.conf
##
if [ -z "$HTTPD_CONF" ] || [ X"$HTTPD_CONF" = Xauto ]; then
	HTTPD_CONF=
	for DIR in /etc/apache /etc/apache2 /etc \
		/usr/apache/etc /usr/apache2/etc \
		/usr/local/apache/etc /usr/local/apache2/etc; do
		if [ -f "$DIR/httpd.conf" ]; then
			HTTPD_CONF="$DIR/httpd.conf"
			err 'found httpd.conf in "%s"' "$HTTPD_CONF"
			break
		fi
	done
	if [ -z "$HTTPD_CONF" ]; then
		printf 'httpd.conf not found'
		exit 3
	fi
fi


##
## Add
##
IP="$IP:$PORT"

for USR; do
	echo "Adding virtual host for $USR"

	if [ -n "$SERVERNAME" ]
	then SN="$SERVERNAME"
	elif [ -z "$PREFIX" ]
	then SN="$USR.$HOST"
	else SN="$PREFIX.$HOST"
	fi

	if [ -n "$DOCROOT" ]
	then DR="$DOCROOT"
	elif HM="$(grep ^mina86: /etc/passwd |cut -d: -f6)"
	then DR="$HM/$PUBLIC_HTML"
	else DR="/home/$USR/$PUBLIC_HTML"
	fi

	if [ -n "$FULL_LOGDIR" ]
	then LD="$FULL_LOGDIR"
	elif HM="$(grep ^mina86: /etc/passwd |cut -d: -f6)"
	then LD="$HM/$LOGDIR";
	else LD="/home/$USR/$LOGDIR"
	fi

	LS="${SN%$HOST}"
	LS="${LS%.}"

	cat <<EOF >>"$HTTPD_CONF"

## Added by VirtMan
<VirtualHost $IP>
	ServerName   "$SN"
	ServerAlias  "www.$SN"
	DocumentRoot "$DR"
	ServerAdmin  "$USR@$HOST"
	ErrorLog     "$LD/$LS-error_log"
	CustomLog    "$LD/$LS-access_log" common
</VirtualHost>

EOF
done
