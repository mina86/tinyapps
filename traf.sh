#!/bin/bash

# Shows TX/RX for eth0 over 1sec
# $Id: traf.sh,v 1.3 2005/07/10 23:05:51 mina86 Exp $
# By Michal Nazareicz (mn86/AT/o2.pl)
# Released to Public Domain

eval "$(
	awk '/eth0/{ sub(/^.*:/, ""); print "TX1=" $9 "\nRX1=" $1 }' /proc/net/dev
)"
while sleep 1; do
	eval "$(
		awk '/eth0/{ sub(/^.*:/, ""); print "TX2=" $9 "\nRX2=" $1 }' /proc/net/dev
	)"

	TX=$(( $TX2 - $TX1 ))
	RX=$(( $RX2 - $RX1 ))
	PROGRESS=""
	I=0
	while [ $I -lt $TX -o $I -lt $RX ]; do
		if [ $I -gt $TX ]; then
			PROGRESS="$PROGRESS'"
		elif [ $I -gt $RX ]; then
			PROGRESS="$PROGRESS."
		else
			PROGRESS="$PROGRESS:"
		fi
		I=$(( $I + 512 ))
	done
	printf "< %6d > %6d  %s\n" $TX $RX "$PROGRESS"

	TX1=$TX2
	RX1=$RX2
done
