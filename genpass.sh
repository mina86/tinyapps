#!/bin/sh
##
## Generates a random password (or some other key)
## Copyright (c) 2005,2007 by Michal Nazarewicz (mina86/AT/mina86.com)
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
## You can specify number of characters password should have as
## a parameter, eg.;
##     ./getnpass 100
##


has_random() {
	# shellcheck disable=SC2039
	[ -n "$RANDOM" ] && [ "$RANDOM" -ne "$RANDOM" ]
}


if [ -r /dev/urandom ]; then
	gen_stream () {
		head -c 40 /dev/urandom
	}
elif [ -r /dev/random ]; then
	gen_stream () {
		head -c 40 /dev/random
	}
elif has_random; then
	# shellcheck disable=SC2120
	gen_stream () {
		set -- 10
		while [ "$1" -gt 0 ]; do
			# shellcheck disable=SC2039
			printf %04x "$RANDOM"
			set -- $(( $1 - 1 ))
		done
	}
else
	printf "%s: unable to collect random data\n" "${0##*/}" >&2
	exit 1
fi


got_command () { test -n "$(which "$1" 2>/dev/null)"; }

if got_command uuencode; then
	gen_pass () {
		printf %s "$(uuencode -m pass | tail -n +2 | head -n 1)"
	}
elif got_command md5sum; then
	gen_pass () {
		P1=$(head -c 32 | md5sum | cut -c1-32)
		P2=$(head -c 32 | md5sum | cut -c1-32)
		printf %s "$P1" "$P2"
	}
elif got_command sha1sum; then
	gen_pass () {
		P1=$(head -c 32 | sha1sum | cut -c1-32)
		P2=$(head -c 32 | sha1sum | cut -c1-32)
		printf %s "$P1" "$P2"
	}
else
	printf "%s: unable to generate random password\n" "${0##*/}" >&2
	exit 1
fi



len=${1:-64}
if [ "$len" -gt 0 ]; then
	l=$len
	while [ "$l" -gt 0 ]; do
		# shellcheck disable=SC2119
		gen_stream | gen_pass
		l=$(( l - 64 ))
	done | cut -c "1-$len"
fi
