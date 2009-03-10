#!/bin/sh
##
## Displays the CPU load.
## By Michal Nazareicz (mina86/AT/mina86.com)
## Released to Public Domain
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##

if [ "X$1" == X-h ] || [ "X$1" == "X--help" ]; then
	cat <<EOF
usage: ${0##*/} [ -n | --nice ]
    -n --nice  includes nice value
EOF
fi


if [ "X$1" == X-n ] || [ "X$1" == "X--nice" ]; then NICE=y; else NICE=; fi


while true; do
	head -n 1 /proc/stat
	sleep 1 || exit
done | while true; do
	read IGNORE A B C D IGNORE2
	if [ -z "$NICE" ]; then
		LOAD=$(( $A + $B + $C ))
		TOTAL=$(( $LOAD + $D ))
	else
		LOAD=$(( $A + $C ))
		TOTAL=$(( $LOAD + $B + $D ))
	fi

	if [ -z "$OTOTAL" ] || [ "X$TOTAL" == "X$OTOTAL" ]; then
		CPULOAD=0
	else
		CPULOAD=$((10000 * ($LOAD-$OLOAD) / ($TOTAL-$OTOTAL)))
	fi

	printf " %3d.%02d%%\r" $(($CPULOAD / 100)) $(($CPULOAD % 100))

	OTOTAL=$TOTAL
	OLOAD=$LOAD
	sleep 1
done
