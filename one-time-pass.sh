#!/bin/sh
##
## Changes user's password to a random one
## By Michal Nazarewicz (mina86/AT/mina86.com)
## Released to Public Domain
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##

set -e

if ! which genpass.sh >/dev/null 2>&1; then
	echo "`basename \"$0\"`: genpass.sh required"
	exit 1
fi

PASS=`printf %s $(genpass.sh)`
read -s -p 'Enter old password: ' || exit 0

cat <<EOF
$REPLY
$PASS
$PASS
EOF | passwd "$@"
