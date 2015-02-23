#!/bin/sh
##
## Colorize (adds ANSI codes) diff output.
## Usage: diff ARG... | cdiff
##    or: cdiff ARG...
##
## With no arguments, the context or unified diff on stdin is colorized.
## Otherwise, ${DIFF:-/usr/bin/diff} is run and its output colorized.
##
## Copyright (c) 2005 by Michal Nazareicz <mina86@mina86.com>
## Copyright (c) 2013 by Mark Edgar <medgar123@gmail.com>
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

CDIFF_BOLD="$(tput bold)"
CDIFF_DELETE="$CDIFF_BOLD$(tput setaf 1)"
CDIFF_INSERT="$CDIFF_BOLD$(tput setaf 2)"
CDIFF_CHANGE="$CDIFF_BOLD$(tput setaf 3)"
CDIFF_MISC="$CDIFF_BOLD$(tput setaf 6)"
CDIFF_COMMENT="$CDIFF_BOLD$(tput setaf 5)"
CDIFF_SGR0="$(tput sgr0)"

colorize() {
	sed -e '
		s/^--- .* ----$/'"$CDIFF_MISC"'&/
		s/^\*\*\* .* \*\*\*\*$/'"$CDIFF_MISC"'&/
		s/^[0-9,]\+[acd][0-9,]\+$/'"$CDIFF_MISC"'&/
		s/^@@ -[0-9]\+,[0-9]\+ +[0-9]\+,[0-9]\+ @@/'"$CDIFF_MISC"'&/
		s/^\*\{15\}/'"$CDIFF_MISC"'&/
		t z

		s/^\(---\|+++\|\*\*\*\)/'"$CDIFF_MISC"'&/
		s/^Index: /'"$CDIFF_MISC"'&/
		s/^Only in /'"$CDIFF_MISC"'&/
		t z

		s/^!/'"$CDIFF_CHANGE"'&/
		s/^[+>]/'"$CDIFF_INSERT"'&/
		s/^[-<]/'"$CDIFF_DELETE"'&/
		s/^#/'"$CDIFF_COMMENT"'&/
		t z
		b

		:z
		s/$/'"$CDIFF_SGR0"'/
	'
}

if [ $# -ne 0 ]; then
	${DIFF:-/usr/bin/diff} "$@" | colorize
else
	colorize
fi
