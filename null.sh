#!/bin/sh
##
## Discards standard input
## $Id: null.sh,v 1.3 2005/09/03 13:27:05 mina86 Exp $
## By Stanislaw Klekot (dozzie/AT/irc.pl)
## Released to Public Domain
##

##
## You can run it as: `something | null.sh` in which case stdou of
## `something` will de disgarded or as `null.sh somtehing` in which
## case stdout and stderr will be redirected to /dev/null.
##

##
## You may also add the fallowing function to your .profile file to
## make execution of null a bit faster (every nanosecond is important)
##
## null () {
##     if [ $# -eq 0 ]
##     then cat >/dev/null
##     else "$@" >/dev/null 2>&1
##     fi
## }
##

if [ $# -eq 0 ]
then exec cat  >/dev/null
else exec "$@" >/dev/null 2>&1
fi
