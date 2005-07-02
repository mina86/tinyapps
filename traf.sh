#!/bin/bash

# Shows TX/RX for eth0 over 1sec
# $Id: traf.sh,v 1.2 2005/07/02 18:31:50 mina86 Exp $
# By Michal Nazareicz (mn86/AT/o2.pl)
# Released to Public Domain

TX1=`cat /proc/net/dev | grep "eth0" | cut -d: -f2 | awk '{print $9}'`
RX1=`cat /proc/net/dev | grep "eth0" | cut -d: -f2 | awk '{print $1}'`
while sleep 1; do
	TX2=`cat /proc/net/dev | grep "eth0" | cut -d: -f2 | awk '{print $9}'`
	RX2=`cat /proc/net/dev | grep "eth0" | cut -d: -f2 | awk '{print $1}'`

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
