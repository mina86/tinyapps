#!/bin/sh
##
## splitlines - Splits input, line-by-line, into several files
## Copyright (c) 2006 by Michal Nazarewicz (mina86/AT/mina86.com)
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
BACKUP=false
APPEND=false

while [ $# -ne 0 ]; do
	case "$1" in
	(-h|--help)    version; usage; exit ;;
	(-V|--version) version;        exit ;;

	(-n)          NUM="$2"     ; shift ;;
	(-n*)         NUM="${1#-?}";       ;;
	(--num=*)     NUM="${1#*=}";       ;;
	(-f)          FMT="$2"     ; shift ;;
	(-f*)         FMT="${1#-?}";       ;;
	(--format=*)  FMT="${1#*=}";       ;;
	(-b|--backup) BACKUP=:; ;;
	(-a|--append) APPEND=:; ;;

	(--) shift; break ;;
	(-*) printf '%s: %s\n' "${0##*/}" "invalid argument: '$1'" >&2; exit 1 ;;
	(*)  break ;;
	esac
	shift
done



##
## Prepare files
##
if $BACKUP; then
	if $APPEND; then
		backup () { if [ -e "$1" ]; then cp -- "$1" "$1~"; fi; }
	else
		backup () { if [ -e "$1" ]; then mv -- "$1" "$1~"; fi; }
	fi
else
	backup () {
		:
	}
fi
unset BACKUP

if $APPEND; then
	MODE='>>'
else
	MODE='>|'
fi
unset APPEND

I=0
while [ $I -lt $NUM ]; do
	NAME=$(printf "$FMT" $I)
	backup "$NAME"
	eval "exec $(($I + 3))$MODE\$NAME"
	I=$(($I + 1))
done

unset FMT MODE NAME



##
## Do the job
##
I=0
cat -- "$@" | while IFS= read LINE; do
	printf %s\\n "$LINE" >&$(($I + 3))
	I=$(( ($I + 1) % $NUM ))
done
