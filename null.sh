#!/bin/sh
##
## Discards standard input
## $Id: null.sh,v 1.2 2005/07/11 00:20:58 mina86 Exp $
## By Stanislaw Klekot (dozzie/AT/irc.pl)
## Released to Public Domain
##

exec cat > /dev/null
