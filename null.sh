#!/bin/sh
##
## Discards standard input
## Copyright (c) 2005 by Stanislaw Klekot (dozzie/AT/irc.pl)
## Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/mina86.com)
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
