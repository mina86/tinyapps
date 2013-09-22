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

colorize() {
	sed -e '
		$ s/$/\x1b[0m/

		s/^--- .* ----$/\x1b[1;36m&/
		s/^\*\*\* .* \*\*\*\*$/\x1b[1;36m&/
		s/^[0-9,]\+[acd][0-9,]\+$/\x1b[1;36m&/
		s/^@@ -[0-9]\+,[0-9]\+ +[0-9]\+,[0-9]\+ @@/\x1b[1;36m&/
		s/^\*\{15\}/\x1b[1;36m&/
		t
		s/^\(---\|+++\|\*\*\*\)/\x1b[1;36m&/
		s/^Index: /\x1b[1;36m&/
		s/^Only in /\x1b[1;36m&/

		t
		s/^!/\x1b[1;33m&/
		s/^[+>]/\x1b[1;32m&/
		s/^[-<]/\x1b[1;31m&/
		s/^#/\x1b[1;35m&/

		t
		s/^/\x1b[0m/
	'
}

if [ $# -ne 0 ]; then
	${DIFF:-/usr/bin/diff} "$@" | colorize
else
	colorize
fi
