#!/bin/sh
# Discards standard input
# $Id: null.sh,v 1.1 2005/07/10 18:06:59 mina86 Exp $
# By Stanislaw Klekot (dozzie/at/irc.pl)
# Released to Public Domain

exec cat > /dev/null
