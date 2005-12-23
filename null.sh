#!/bin/sh
##
## Discards standard input
## $Id: null.sh,v 1.4 2005/12/23 14:35:24 mina86 Exp $
## Copyright (c) 2005 by Stanislaw Klekot (dozzie/AT/irc.pl)
## Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/tlen.pl)
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
## usage:
##   command | null         (pipe feature)
##   null    command        (exec feature)
##   null -d command        (exec + daemonize feature)
##   drun    command        (synonym of  null -d command )
##
## Pipe feature is used to discard stdout of a command.  Exec feature
## discards both - stdout and stderr.  If -d is given or it's run as
## drun (daemon run) the command will be put into background.
##

if [ "X${0##*/}" = Xdrun ]; then DAEMONIZE=y; else DAEMONIZE=; fi
if [ "$2" ] && ( [ "X$1" = X-d ] || [ "X$1" = x-D ] ); then
	DAEMONIZE=y
	shift
fi

if [ $# -eq 0 ]
then exec cat  >/dev/null
elif [ "$DAEMONIZE" ]
then exec "$@" >/dev/null 2>&1 &
else exec "$@" >/dev/null 2>&1
fi
