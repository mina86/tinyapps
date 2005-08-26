#!/bin/bash
##
## Changes user's password to a random one
## $Id: one-time-pass.sh,v 1.1 2005/08/26 16:13:57 mina86 Exp $
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

set -e
ARG0="${0##*/}"


##
## Generates a password from random stream
##
genpass () {
	if which uuencode >/dev/null 2>&1; then
		PASS=`echo -n "$1" | uuencode -m pass | tail -n +2 | head -n 1`
	elif which md5sum >/dev/null 2>&1; then
		P1=`echo -n "${1:0:32}"  | md5sum | cut -c1-32`
		P2=`echo -n "${1:32:32}" | md5sum | cut -d1-32`
		PASS="$P1$P2"
	else
		printf "%s: unable to generate random password" "$ARG0" >&2
		exit 1
	fi
}


##
## Generate random password
##
if [ -f /dev/urandom ]; then
	genpass "`head -c 64 /dev/urandom`"
elif [ -f /dev/random ]; then
	genpass "`head -c 64 /dev/random`"
elif [ "$RANDOM" -a "$RANDOM" -ne "$RANDOM" ]; then
	LOOP=16; PASS=
	while [ $LOOP -gt 0 ]; do
		PASS="$PASS`printf %04x $RANDOM`"
		LOOP=$(( $LOOP - 1 ))
	done
else
	printf "%s: unable to generate random password" "$ARG0" >&2
	exit 1
fi


##
## Change password
##
read -s -p 'Enter old password: ' || exit 0
cat <<EOF
$REPLY
$PASS
$PASS
EOF | passwd "$@"
