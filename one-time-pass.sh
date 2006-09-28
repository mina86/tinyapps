#!/bin/sh
##
## Changes user's password to a random one
## $Id: one-time-pass.sh,v 1.3 2006/09/28 15:06:19 mina86 Exp $
## Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/mina86.com)
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

if ! which genpass.sh >/dev/null 2>&1; then
	printf "%s: genpass.sh required\n"
	exit 1
fi

PASS=`printf %s $(genpass.sh)`
read -s -p 'Enter old password: ' || exit 0

cat <<EOF
$REPLY
$PASS
$PASS
EOF | passwd "$@"
