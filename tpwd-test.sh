#!/bin/sh
##
## Test for tpwd script
## Copyright (c) 2014 by Michal Nazareicz (mina86/AT/mina86.com)
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

set -eu


self="$PWD/tpwd-test.sh"
tpwd="$PWD/tpwd"
if ! [ -x "$self" ] || ! [ -x "$tpwd" ]; then
	echo "Expected $self and $tpwd to be executable." >&2
	exit 2
fi

temp=$(mktemp -d || exit 2)
trap 'rm -r -- "$temp"' 0

mkdir "$temp/foobar" "$temp/foobar/baz"
cd "$temp/foobar"
ln -s baz qux
cd "$temp/foobar/qux"

cat >$temp/expected <<EOF
$ tpwd
<~/foobar/qux
> 0
$ tpwd 5
<...ux
> 0
$ tpwd 5 {
<{/qux
> 0
$ tpwd 5 ... 1
<.../qux
> 0
$ tpwd 5  1
</qux
> 0
$ tpwd -n
<~/foobar/qux> 0
$ tpwd -n 5
<...ux> 0
$ tpwd -n 5 {
<{/qux> 0
$ tpwd -n 5 ... 1
<.../qux> 0
$ tpwd -n 5  1
</qux> 0
$ tpwd -L
<~/foobar/qux
> 0
$ tpwd -L 5
<...ux
> 0
$ tpwd -L 5 {
<{/qux
> 0
$ tpwd -L 5 ... 1
<.../qux
> 0
$ tpwd -L 5  1
</qux
> 0
$ tpwd -L -n
<~/foobar/qux> 0
$ tpwd -L -n 5
<...ux> 0
$ tpwd -L -n 5 {
<{/qux> 0
$ tpwd -L -n 5 ... 1
<.../qux> 0
$ tpwd -L -n 5  1
</qux> 0
$ tpwd -P
<~/foobar/baz
> 0
$ tpwd -P 5
<...az
> 0
$ tpwd -P 5 {
<{/baz
> 0
$ tpwd -P 5 ... 1
<.../baz
> 0
$ tpwd -P 5  1
</baz
> 0
$ tpwd -P -n
<~/foobar/baz> 0
$ tpwd -P -n 5
<...az> 0
$ tpwd -P -n 5 {
<{/baz> 0
$ tpwd -P -n 5 ... 1
<.../baz> 0
$ tpwd -P -n 5  1
</baz> 0
EOF

run() {
	set -- tpwd "$@"
	printf "$ %s\n<" "$*"
	shift
	ec=0
	HOME=$temp "$tpwd" "$@" || ec=$?
	printf '> %d\n' $ec
}

for lp in '' -L -P; do
	for n in '' -n; do
		run $lp $n
		run $lp $n 5
		run $lp $n 5 '{'
		run $lp $n 5 ... 1
		run $lp $n 5 '' 1
	done
done >$temp/got

if cmp -s "$temp/expected" "$temp/got"; then
	echo "All good."
else
	tput setf 4; tput bold; echo 'FAILED; got difference:'
	diff -u "$temp/expected" "$temp/got"
	exit 1
fi
