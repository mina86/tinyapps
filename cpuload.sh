#!/bin/sh
##
## Displays the CPU load.
## $Id: cpuload.sh,v 1.5 2005/07/11 00:20:58 mina86 Exp $
## By Michal Nazareicz (mina86/AT/tlen.pl)
## Released to Public Domain
##

while true; do
	head -n 1 /proc/stat
	sleep 1 || exit
done | while true; do
	read IGNORE A B C D IGNORE2
	read IGNORE A B C D IGNORE2 <<<"`cat /proc/stat`"
	LOAD=$(( $A + $B + $C ))
	TOTAL=$(( $LOAD + $D ))

	if [ -z "$OTOTAL" -o $TOTAL == "$OTOTAL" ]; then CPULOAD=0; else
		CPULOAD=$((10000 * ($LOAD-$OLOAD) / ($TOTAL-$OTOTAL)))
	fi

	printf " %3d.%02d%%\r" $(($CPULOAD / 100)) $(($CPULOAD % 100))

	OTOTAL=$TOTAL
	OLOAD=$LOAD
	sleep 1
done
