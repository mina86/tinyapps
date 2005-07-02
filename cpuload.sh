#!/bin/sh

# Displays the CPU load
# $Id: cpuload.sh,v 1.1 2005/07/02 00:30:43 mina86 Exp $
# Copyright (C) 2005 by Michal Nazareicz (mn86/AT/o2.pl)
# Licensed under the Academic Free License version 2.1.

while true; do
	read IGNORE A B C D IGNORE2 <<<"`cat /proc/stat`"
	LOAD=$(( $A + $B + $C ))
	TOTAL=$(( $LOAD + $D ))

	if [ -z "$OTOTAL" -o $TOTAL == "$OTOTAL" ]; then CPULOAD=0; else
		CPULOAD=$((100 * ($LOAD-$OLOAD) / ($TOTAL-$OTOTAL)))
	fi

	printf " %3d%%\r" $CPULOAD

	OTOTAL=$TOTAL
	OLOAD=$LOAD
	sleep 1
done
