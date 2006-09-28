#!/bin/sh
##
## splitlines - Splits input, line-by-line, into several files
## $Id: splitlines.sh,v 1.2 2006/09/28 15:06:19 mina86 Exp $
## Copyright (c) 2006 by Michal Nazarewicz (mina86/AT/mina86.com)
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
## Version & usage
##
version () {
	echo 'splitlines 0.1   (c) 2006 by Micha³ Nazarewicz (mina86/AT/mina86.com)'
	echo Licensed under the GNU General Public License
	echo
}

usage () {
	cat <<EOF
usage: splitlines [ <options> ] [ [--] <files> ]
<options>:
  -h --help       Prints this screen and exits
  -V --version    Prints version and exits

  -n --num=NUM    Splits input to NUM files                           [2]
  -f --format=FMT Names output files using given printf format   [out-%d]
  -b --backup     Backups output files
  -a --append     Appends lines to output files

<files> are files to read from.  If not specified stdin is used.

The script reads input line-by-line and outputs 1st line to the 1st
file, 2nd line to the 2nd file, ..., NUMth line to the NUMth file,
NUM+1st line to 1st line, and so on.
EOF
}



##
## Parse arguments
##
set -e
NUM=2
FMT=out-%d
BACKUP=
APPEND=

while [ $# -ne 0 ]; do
	case "$1" in
	(-h|--help)    version; usage; exit; ;;
	(-V|--version) version;        exit; ;;

	(-n)          NUM="$2"     ; shift; ;;
	(-n*)         NUM="${1#-?}";        ;;
	(--num=*)     NUM="${1#*=}";        ;;
	(-f)          FMT="$2"     ; shift  ;;
	(-f*)         FMT="${1#-?}";        ;;
	(--format=*)  FMT="${1#*=}";        ;;
	(-b|--backup) BACKUP=y; ;;
	(-a|--append) APPEND=y; ;;

	(--) break; ;;
	(-*) printf "%s: %s\n" "${0##*/}" "invalid argument: '$1'" >&2; exit 1; ;;
	(*)  break; ;;
	esac
	shift
done

if [ "X$1" = X-- ]; then
	shift
fi



##
## Prepare files
##
if [ -z "$APPEND" ]; then
	I=0
	while [ $I -lt $NUM ]; do
		NAME=$(printf "$FMT" $I)
		if [ -e "$NAME" ]; then
			if [ "$BACKUP" ]
			then mv -- "$NAME" "$NAME~"
			else rm -f -- "$NAME"
			fi
		fi
		I=$(($I + 1))
	done
fi



##
## Do the job
##
I=0
cat -- "$@" | while IFS= read LINE; do
	printf "%s\n" "$LINE" >>$(printf "$FMT" $I)
	I=$(( ($I + 1) % $NUM ))
done
