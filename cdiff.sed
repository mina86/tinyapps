#!/bin/sed -f
##
## Colorize (adds ANSI codes) output of diff.
## $Id: cdiff.sed,v 1.4 2008/11/08 23:45:57 mina86 Exp $
## Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
## Licensed under the Academic Free License version 2.1.
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##

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
