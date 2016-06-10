#!/bin/sh
##
## Displays the CPU load.
## By Michal Nazareicz (mina86/AT/mina86.com)
## Released to Public Domain
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##

if [ "X$1" = X-h ] || [ "X$1" = "X--help" ]; then
	cat <<EOF
usage: ${0##*/} [ -n | --nice ]
    -n --nice  includes nice value
EOF
fi


if [ "X$1" = X-n ] || [ "X$1" = "X--nice" ]; then
	nice=true
else
	nice=false
fi


while true; do
	head -n 1 /proc/stat
	sleep 1 || exit
done | while true; do
	read _ a b c d _
	if $nice; then
		load=$(( a + b + c ))
		total=$(( load + d ))
	else
		load=$(( a + c ))
		total=$(( load + b + d ))
	fi

	if [ -z "$ototal" ] || [ x"$total" = x"$ototal" ]; then
		cpuload=0
	else
		cpuload=$((10000 * (load-oload) / (total-ototal)))
	fi

	printf " %3d.%02d%%\r" $((cpuload / 100)) $((cpuload % 100))

	ototal=$total
	oload=$load
	sleep 1
done
