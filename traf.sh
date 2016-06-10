#!/bin/sh
##
## Shows TX/RX over 1sec
## By Michal Nazareicz (mina86/AT/mina86.com)
##  & Stanislaw Klekot (dozzie/AT/irc.pl)
## Released to Public Domain
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##

# usage: ./traf.sh <interface>
# <interface> can be a BRE


set -e
INT="${1-eth0}"

# shellcheck disable=SC2120
get_traffic () {
	TX2=0; RX2=0;
	while read LINE; do
		expr X"${LINE%%:*}" : X".*\\($INT\\)" >/dev/null || continue
		# shellcheck disable=SC2086
		set -- ${LINE#*:}
		RX2=$(( RX2 + $1 ))
		TX2=$(( RX2 + $9 ))
	done </proc/net/dev
}

# shellcheck disable=SC2119
get_traffic

while sleep 1; do
	TX1=$TX2
	RX1=$RX2
	# shellcheck disable=SC2119
	get_traffic

	TX=$(( TX2 - TX1 ))
	RX=$(( RX2 - RX1 ))

	PT=
	PR=
	I=0
	while [ "$I" -lt 15360 ]; do
		if [ "$I" -lt "$TX" ]; then PT="#$PT"; else PT=" $PT"; fi
		if [ "$I" -lt "$RX" ]; then PR="$PR#"; else PR="$PR "; fi
		I=$(( I + 512 ))
	done
	if [ "$TX" -gt 2048 ]; then TX="$(( TX / 1024 )) K"; fi
	if [ "$RX" -gt 2048 ]; then RX="$(( RX / 1024 )) K"; fi
#	printf "%s < %6s | %6s > %s\n" "$PT" "$TX" "$RX" "$PR"
	printf "%6s  %s | %s  %6s\n" "$TX" "$PT" "$PR" "$RX"

#	PROGRESS=""
#	I=0
#	while [ $I -lt $TX -o $I -lt $RX ]; do
#		if [ $I -gt $TX ]; then
#			PROGRESS="$PROGRESS'"
#		elif [ $I -gt $RX ]; then
#			PROGRESS="$PROGRESS."
#		else
#			PROGRESS="$PROGRESS:"
#		fi
#		I=$(( $I + 512 ))
#	done
#	if [ $TX -gt 2048 ]; then TX="$(( $TX / 1024 )) K"; fi
#	if [ $RX -gt 2048 ]; then RX="$(( $RX / 1024 )) K"; fi
#	printf "< %6s > %6s  %s\n" "$TX" "$RX" "$PROGRESS"
done
