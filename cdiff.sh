#!/bin/sh
##
## Colorize (adds ANSI codes) diff output.
## Usage: diff ARG... | cdiff
##    or: cdiff ARG...
##
## With no arguments, the context or unified diff on stdin is colorized.
## Otherwise, ${DIFF:-/usr/bin/diff} is run and its output colorized.
##
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

CDIFF_BOLD="$(tput bold)"
CDIFF_DELETE="$CDIFF_BOLD$(tput setaf 1)"
CDIFF_INSERT="$CDIFF_BOLD$(tput setaf 2)"
CDIFF_CHANGE="$CDIFF_BOLD$(tput setaf 3)"
CDIFF_MISC="$CDIFF_BOLD$(tput setaf 6)"
CDIFF_COMMENT="$CDIFF_BOLD$(tput setaf 5)"
CDIFF_SGR0="$(tput sgr0)"

colorize() {
	sed -e '
		$ s/$/'"$CDIFF_SGR0"'/

		s/^--- .* ----$/'"$CDIFF_MISC"'&/
		s/^\*\*\* .* \*\*\*\*$/'"$CDIFF_MISC"'&/
		s/^[0-9,]\+[acd][0-9,]\+$/'"$CDIFF_MISC"'&/
		s/^@@ -[0-9]\+,[0-9]\+ +[0-9]\+,[0-9]\+ @@/'"$CDIFF_MISC"'&/
		s/^\*\{15\}/'"$CDIFF_MISC"'&/
		t
		s/^\(---\|+++\|\*\*\*\)/'"$CDIFF_MISC"'&/
		s/^Index: /'"$CDIFF_MISC"'&/
		s/^Only in /'"$CDIFF_MISC"'&/

		t
		s/^!/'"$CDIFF_CHANGE"'&/
		s/^[+>]/'"$CDIFF_INSERT"'&/
		s/^[-<]/'"$CDIFF_DELETE"'&/
		s/^#/'"$CDIFF_COMMENT"'&/

		t
		s/^/'"$CDIFF_SGR0"'/
	'
}

if [ $# -ne 0 ]; then
	${DIFF:-/usr/bin/diff} "$@" | colorize
else
	colorize
fi
