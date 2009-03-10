#!/bin/sh

##
## ivona.sh - synth. human speach using Ivona[tm]
## Copyright 2006 by Michal Nazarewicz (mina86/AT/mina86.com>
## Based on script by Damian "Rush" Kaczmarek - mail: rush@vox.one.pl
##
## This software is OSI Certified Open Source Software.
## OSI Certified is a certification mark of the Open Source Initiative.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, see <http://www.gnu.org/licenses/>.
##
## This is part of Tiny Applications Collection
##   -> http://tinyapps.sourceforge.net/
##


##
## Default values
##
set -e
QUALITY=16
SPEED=89
URL='http://www.ivo.pl/?page=syntezator_mowy_ivona'
PLAYER=auto
OUTPUT=
PLAY=y
CACHE=y
GENCACHE=y


HELP="usage: ${0##*/} [ -R ] <options> [ -- ] <text>
 -R                   ignores files found in ~/.ivona/

<options> are:
 -h --help            shows this help screen
 -p --player=player   sets player (%s indicates file name) [$PLAYER]
 -q --qulity=16|22|8  sets quality                         [$QUALITY]
 -s --speed=<0..99>   sets speed (0 is fastes)             [$SPEED]
 -u --url=url         requests MP3 file from URL
 -n --no-play         does not play downloaded file
 -o --output=file     saves output to <file>
 -O --Output=file     alias of  -n -o file
 -g --no-gencache     does not generate cache files
 -c --no-cache        does not use cache files

<text> is
 -f --file=file       reads text from a file
 -                    synonym of  -f -
 -F --play=file       appends given mp3 file to output file
 -t --text=word       uses word as text
 text                 speaks text

<text> can be any combination of the above 4 options and if so each
part will be treated separatly so for example 'ala ma kota' will be
something different then '-t ala -t ma -t kota'.  It is useful as the
script caches downloaded files so you can run script as '${0##*/} -t
\"\$USER\" zalogowa³ siê'.  On the other hand, it won't sound as good
as one would want it to.

Script saves cache files in ~/.ivona/cache or /var/cache/ivona/ if
run as root.
"


##
## Load config
##
ROOTMODE=
if [ X"$1" = X-R ]; then
	shift
	ROOTMODE=y
fi
if [ -f /etc/ivona/config ]; then . /etc/ivona/config; fi
if [ -z "$ROOTMODE" ] && [ -f ~/.ivona/config ]; then
	. ~/ivona/config
fi



##
## Helper functions
##
err () {
	__FMT="$1"; shift
	if [ X"$1" = X-e ]; then __EXIT=1; shift; else __EXIT=; fi
	printf "%s: $FMT\n" "${0##*/}" "$@" >&2
	if [ -n "$__EXIT" ]; then exit $__EXIT; fi
}

isInt () {
	expr X"$1" : X'[[:space:]]*[0-9][0-9][[:space:]]**$' >/dev/null 2>/dev/null
}

toInt () {
	expr 0$(expr X"$1" : X'[[:space:]]*\([0-9]*\)' 2>/dev/null || true) + 0
}


##
## Parse args
##
while [ X"${1#-}" != X"$1" ] && [ X"$1" != X- ] && [ X"$1" != X-- ] && \
	[ X"$1" != X-t ] && [ X"$1" != X-f ] && [ X"$1" != X-F ]; do

	## Short option
	PARAM=
	if [ X"${1#--}" = X"$1" ]; then
		A="${1#?}"; shift; OPTS=; ARG=
		while [ -n "$A" ]; do
			case "$A" in
			(h*) OPTS="$OPTS --help"      ;;
			(p*) OPTS="$OPTS --player"    ; PARAM="${A#?}"; A=;;
			(q*) OPTS="$OPTS --quality"   ; PARAM="${A#?}"; A=;;
			(s*) OPTS="$OPTS --speed"     ; PARAM="${A#?}"; A=;;
			(s*) OPTS="$OPTS --url"       ; PARAM="${A#?}"; A=;;
			(n*) OPTS="$OPTS --no-play"   ;;
			(o*) OPTS="$OPTS --output"    ; PARAM="${A#?}"; A=;;
			(O*) OPTS="$OPTS --Output"    ; PARAM="${A#?}"; A=;;
			(g*) OPTS="$OPTS --no-gencahe";;
			(c*) OPTS="$OPTS --no-cache"  ;;
			(*) ARG="${A#?}"; A="${A%$ARG}";
				err -e 'invalid option: -%c' "$A";;
			esac
			A="${A#?}"
		done
		set -- $OPTS "$@"
	fi

	## Long option
	SET=; ARG="${1#--}"; shift
	case "$ARG" in
	(help)               printf %s "$HELP"; exit 0;;
	(player|player=*)    SET=PLAYER;;
	(quality|quality=*)  SET=QUALITY;;
	(speed|speed=*)      SET=SPEED;;
	(url|url=*)          SET=URL;;
	(no-play)            PLAY=;;
	(output|output=*)    SET=OUTPUT;;
	(Output|Output=*)    SET=OUTPUT; PLAY=;;
	(no-gencahe)         GENCACHE=;;
	(no-cache)           CACHE=;;
	(*)                  err -e 'invalid option: --%s' "$ARG";;
	esac

	## Option with parameter
	if [ -n "$SET" ]; then
		if [ -n "$PARAM" ]; then
			eval $SET='"$PARAM"'
		elif [ X"${ARG#*=}" != X"$ARG" ]; then
			eval $SET='"${ARG#*=}"'
		elif [ $# -ne 0 ]; then
			eval $SET='"$1"'
			shift
		else
			err -e '--%s requires an agument' "$ARG"
		fi
	fi
done
if [ X"$1" = X-  ] && [ $# -eq 1 ]; then INPUT=-; fi


##
## Validate
##
if ! isInt "$QUALITY"; then err -e "quality must be 22, 16 or 8"; fi
QUALITY=$(toInt "$QUALITY")
if [ $QUALITY -ne 8 ] && [ $QUALITY -ne 16 ] && [ $QUALITY -ne 22 ]; then
	err -e "quality must be 22, 16 or 8"
fi

if ! isInt "$SPEED"; then err -e "speed must be betwen 0 and 99"; fi
SPEED=$(toInt "$SPEED")
if [ $SPEED -lt 0 ] || [ $SPEED -gt 99 ]; then
	err -e "speed must be betwen 0 and 99"
fi


##
## Downloads a file
##
get () {
	__F=

	##
	## Format text
	##
	__T="$(printf %s "$1" |
		tr 'QWERTYUIOPASDFGHJKLZXCVBNM¡ÆÊ£ÑÓ¦¬¯;-' \
		   'qwertyuiopasdfghjklzxcvbnm±æê³ñó¶¼¿,,')"
	__T="$(printf %s "$__T" | \
		sed -e 's/[[:space:]]*,[[:space:]]*/, /g' \
		    -e 's/[[:space:]][[:space:]]*\([?!.]\)/\1/g' \
		    -e 's/[[:space:]][[:space:]]*/ /g' \
		    -e 's/^[[:space:]][[:space:]]*//' \
		    -e 's/[[:space:]][[:space:]]*$//')"
	if [ -f /etc/ivona/sed ]; then
		__T="$(printf %s "$__T" | sed -f /etc/ivona/sed)"
	fi
	if [ -z "$ROOTMODE" ] && [ -f ~/.ivona/sed ]; then
		__T="$(printf %s "$__T" | sed -f ~/.ivona/sed)"
	fi


	if [ ${#__T} -gt 100 ]; then
		err -e 'Text is too long, script does not yet support splitting'
	fi

	if [ -z "$__T" ]; then return 0; fi


	##
	## Cached?
	##
	if [ -n "$CACHE" ]; then
		if [ -z "$ROOTMODE" ] && \
			[ -f "$HOME/.ivona/cache/jacek.$QUALITY.$SPEED.$__T.mp3" ]; then
			__F="$HOME/.ivona/cache/jacek.$QUALITY.$SPEED.$__T.mp3"
		elif [ -f "/var/cache/ivona/jacek.$QUALITY.$SPEED.$__T.mp3" ]; then
			__F="/var/cache/ivona/jacek.$QUALITY.$SPEED.$__T.mp3"
		fi
	fi
	if [ -n "$__F" ]; then
		if [ -n "$3" ]
		then cat -- "$__F" >>"$3"
		else printf %s "$__F"
		fi
		return 0
	fi


	##
	## Download
	##
	__PD="tresc=$__T&submit=ivonaform&glos=jacek&jakosc=$QUALITY"
	__PD="$__PD&format=mp3&szybkosc=$SPEED"

	if which wget >/dev/null 2>&1; then
		wget --post-data="$__PD" -O "$2.part" -- "$URL"

	#elif which curl >/dev/null 2>&1; then
	#	curl -fd "$POSTDATA" -- "$URL" >"$OUTPUT.part"

	elif which lynx >/dev/null 2>&1; then
		if ! printf %s "$___PD" | \
			lynx -source -post_data -- "$URL" >"$2.part"; then
			err -e 'unable to download file'
		fi

	else
		err -e "don't know how to download file; install wget or lynx"
	fi


	##
	## Generate cache ?
	##
	if [ -n "$GENCACHE" ]; then
		if [ $(id -u) -eq 0 ]; then
			mkdir -p /var/cache/ivona
			__F="/var/cache/ivona/jacek.$QUALITY.$SPEED.$__T.mp3"
		elif [ -z "$ROOTMODE" ]; then
			mkdir -p "$HOME/.ivona/cache"
			__F="$HOME/.ivona/cache/jacek.$QUALITY.$SPEED.$__T.mp3"
		fi

		mv -f -- "$2.part" "$__F"
		if [ -n "$3" ]
		then cat -- "$__F" >>"$3"
		else printf %s "$__F"
		fi

	else
		if [ -n "$3" ]
		then cat -- "$2.part" >>"$3"; rm -f -- "$2.part"
		else mv -f -- "$2.part" "$2"; printf %s "$2"
		fi
	fi

}


##
## Generate temporary file
##
TMPFILE=ivona.$UID.$$
if [ -n "$TMPDIR" ] && [ -d "$TMPDIR" ] && [ -w "$TMPDIR" ]; then
	TMPFILE="$TMPDIR/$TMPFILE";
elif [ -d /tmp ] && [ -w /tmp ]; then
	TMPFILE="/tmp/$TMPFILE"
fi
trap 'rm -f -- "$TMPFILE" "$TMPFILE.part" "$TMPFILE.out"' 0


##
## Download all files and generate output
##
rm -f -- "$TMPFILE.out"
TEXT=
while [ $# -ne 0 ]; do
	ARG="$1"; shift

	if [ X"${ARG#-}" != X"$ARG" ] && [ -n "$TEXT" ]; then
		get "$TEXT" "$TMPFILE" "$TMPFILE.out"
		TEXT=
	fi

	case "$ARG" in
	(--)          : ;;

	(-)           TEXT="$(cat)"            ; set -- -- "$@";;
	(-f?*)        TEXT="$(cat "${ARG#-f}")"; set -- -- "$@";;
	(--file=*)    TEXT="$(cat "${ARG#*=}")"; set -- -- "$@";;
	(-f|--file)
		if [ $# -eq 0 ]; then err -e '%s requires an argument' --file; fi
		TEXT="$(cat "$1")"; shift; set -- -- "$@";;

	(-F?*)        cat "${ARG#-F}" >>"$TMPFILE.out";;
	(--play=*)    cat "${ARG#*=}" >>"$TMPFILE.out";;
	(-F|--play)
		if [ $# -eq 0 ]; then err -e '%s requires an argument' --play; fi
		cat "$1" >>"$TMPFILE.out"; shift;;

	(-t?*)        TEXT="${ARG#-t}"; set -- -- "$@";;
	(--text=*)    TEXT="${ARG#*=}"; set -- -- "$@";;
	(-t|--text)
		if [ $# -eq 0 ]; then err -e '%s requires an argument' --text; fi
		TEXT="$2"; set -- -- "$@"; shift;;

	(--*)         err -e 'invalid option: %c' "${ARG%%=*}";;
	(-*)
		A="${ARG#??}"; ARG="${ARG%$A}";
		err -e 'invalid option: %c' "${ARG%%=*}";;
	(*)           TEXT="$TEXT $ARG"
	esac
done
get "$TEXT" "$TMPFILE" "$TMPFILE.out"


##
## Get player
##
if [ -z "$PLAYER" ] || [ X"$PLAYER" = Xauto ]; then
	if which mpg321 >/dev/null 2>&1; then
		PLAYER="mpg321 -g1000 --"
	elif which mpg123 >/dev/null 2>&1; then
		PLAYER="mpg123 -g1000 --"
	elif which mplayer >/dev/null 2>&1; then
		PLAYER="mplayer -af volnorm,volume=10:0 --"
	elif which lame >/dev/null 2>&1 && which aplay >/dev/null 2>&1; then
		PLAYER="lame --decode %s - | aplay"
	else
		err -e "don't know how to play music"
	fi
fi


##
## Play
##
if [ -n "$OUTPUT" ]; then
	mv -f -- "$TMPFILE.out" "$OUTPUT"
	PLAYFILE="$OUTPUT"
else
	PLAYFILE="$TMPFILE.out"
fi

if [ -z "$PLAY" ]; then exit 0; fi

if ! expr X"$PLAYER" : '.*[^%]\(%%\)*%s' >/dev/null 2>&1; then
	exec $PLAYER "$PLAYFILE"
	exit $?
fi

PLAYFILE="'$(printf %s "$PLAYFILE" | sed -e "s/'/'\\\\''/")'"
exec eval $(printf "$PLAYER" "$PLAYFILE")
