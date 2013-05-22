#!/usr/bin/perl
##
## Generates a random password (or some other key)
## Copyright (c) 2005,2007,2011,2013 by Michal Nazarewicz (mina86/AT/mina86.com)
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
## You can specify number of characters password should have as
## a parameter, eg.;
##     ./getnpass 100
##

use warnings;
use strict;

## Forward declarations
sub buildChars($);
sub randomBytes($);
sub encode($$);
sub transform($$$);

## Modifiers
my %modifiers = (
	'hex'   => '0123456789abcdef',
	'HEX'   => '0123456789ABCDEF',
	'alpha' => 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ',
	'al'    => '--alpha',
	'alnum' => 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789',
	'lower' => 'abcdefghijklmnopqrstuvwxyz',
	'lw'    => '--lower',
	'upper' => 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',
	'up'    => '--upper',
	'b64'   => '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/',
	'all'   => join('', map chr, 33..126),

	'guard'      => { 'prefix' => '/!' },
	'noguard'    => { 'prefix' => '' },
	'g' => '--guard',
	'G' => '--noguard',

	'strength'   => { 'strength' => 1 },
	'nostrength' => { 'strength' => 0 },
	's' => '--strength',
	'S' => '--nostrength',

	'clear'   => { 'prefix' => '', 'strength' => 0 },
	'c'       => '--clear',
);

# Help
if (@ARGV == 1 && $ARGV[0] =~ /^(?:(?:--)?help|-h)$/) {
	print <<'END';
usage: $0 [ <options> ... ] [ <length> ]
Choosing characters set:
    hex          Use lower case hexadecimal characters in the password.
    HEX          Use upper case hexadecimal characters in the password.
    al alpha     Use lower and upper case letters.
    lw lower     Use lower case letters.
    up upper     Use upper case letters.
  * b64          Use characters used in base64 encoding (letters, digits,
                 plus and slash).
    all          Use all printable ASCII characters (codes from 33 to 126
                 inclusively).
    ch:<spec>    Custom character list.

Additional options:
  * g guard      Add "/!" at the beginning of generated password.
                 Slash prevents accidental pasting of the password
                 into IRC clients, and exclamation mark prevents
                 accidental pasting of the password on shell command
                 line.  This prefix is not counted towards password length.
    G noguard    Do not add "/!" at the beginning of generated password.

  * s strength   Ensure password has one upper case letter, one lower
                 case letter, and one digit.  This option causees
                 those characters to be added regardless of chosen
                 characters sets.  This suffix is added to the password and
                 is not counted towards its length as specified by <length>.
    S nostrength Disable the above behaviour.

    c clear      Synonym of "noguard nostrength".

<length> specifies desired password length.  The default is to choose
a length such that the password has at least 80 bits of entropy.

* indicates default option.
END
}

# Parse arguments
my ($chars, $length, %O, $excess);
unshift @ARGV, qw(b64 guard strength);

do {
	$_ = shift @ARGV;

	if (/^\d+$/ && $_ != 0) {
		$length = int $_;
	} elsif (/^ch:/) {
		$chars = buildChars substr $_, 3;
	} elsif (!exists $modifiers{$_}) {
		die "$_: unknown modifier\n";
	} elsif ('HASH' eq ref $modifiers{$_}) {
		my $modifier = $modifiers{$_};
		for (keys %$modifier) {
			$O{$_} = $modifier->{$_};
		}
	} elsif ($modifiers{$_} =~ /^--/) {
		unshift @ARGV, substr $modifiers{$_}, 2;
	} else {
		$chars = $modifiers{$_};
	}
} while (@ARGV);

# Do the work
if (defined $length) {
	$_ = randomBytes $O{'length'} + 3;  # TODO
} else {
	$_ = randomBytes 13;
}

$_ = encode $_, $chars;

if (defined $length) {
	$excess = substr $_, $length;
	$_ = $O{'prefix'} . substr $_, 0, $length;
} else {
	$excess = substr $_, -3;
	$_ = $O{'prefix'} . substr $_, 0, -3;
}

if ($O{'strength'}) {
	if (m/[A-Z]/) {
		$_ .= substr $excess, 0, 1;
	} else {
		$_ .= chr(ord('A') + int rand 26);
	}
	if (m/[a-z]/) {
		$_ .= substr $excess, 1, 1;
	} else {
		$_ .= chr(ord('a') + int rand 26);
	}
	if (m/[0-9]/) {
		$_ .= substr $excess, 2, 1;
	} else {
		$_ .= chr(ord('0') + int rand 10);
	}
}

print $_, "\n";


## Generate chars
sub buildChars($) {
	my (@chars, $prev, $range, $ch);
	for $ch (unpack 'C*', $_[0]) {
		if (defined $range) {
			pop @chars;
			if ($prev < $ch) {
				push @chars, $prev..$ch;
			} else {
				die sprintf("%c-%c: invalid character range\n",
				            $prev, $ch);
			}
			undef $prev;
			undef $range;
		} elsif (defined $prev && $ch == ord '-') {
			$range = 1;
		} else {
			push @chars, $ch;
			$prev = $ch;
		}
	}
	my $chars = '';
	undef $prev;
	for $ch (sort @chars) {
		if (!defined $prev || $prev != $ch) {
			$chars .= chr $ch;
		}
		$prev = $ch;
	}
	if (!length $chars) {
		die "empty character specification\n";
	} elsif (length $chars == 1) {
		die "$chars: only one character specified\n";
	}
	$chars;
}

## Read random data
sub readRandomBytes($$);
sub genRandomBytes($);

sub randomBytes($) {
	my $bytes = $_[0];
	my $data =
	    readRandomBytes($bytes, '/dev/urandom') //
	    genRandomBytes($bytes);
	substr $data, 0, $bytes;
}

sub readRandomBytes($$) {
	my ($bytes, $file, $fd) = @_;
	if (!open $fd, '<', $file) {
		return;
	}

	my $data = '';

	do {
		$bytes -= read $fd, $data, $bytes, length $data;
	} while ($bytes);

	close $fd;
	$data;
}

sub genRandomBytes($) {
	warn "unable to open /dev/*random, will use week software generator\n";
	my $bytes = $_[0];
	my $data = '';
	do {
		my $num = int rand 0x10000;
		$data .= chr($num % 256) . chr($num / 256);
		$bytes -= 2;
	} while ($bytes > 0);
	$data;
}

## Encoder
sub transform($$$) {
	my ($str, $from, $to) = @_;
	if ($from ne $to) {
		if (length $from != length $to) {
			die "internal (<<$from>> <<$to>>)\n";
		}
		eval "\$str =~ tr[$from][$to], 1" or die $@;
	}
	$str;
}

sub encode($$) {
	my ($bytes, $chars) = @_;
	if (length $chars == 16) {
		transform unpack('h*', $bytes), '0123456789abcdef', $chars;
	} elsif (length $chars == 64) {
		require MIME::Base64;
		MIME::Base32->import('RFC');
		my $str = MIME::Base64::encode($bytes);
		chomp $str;
		$str =~ s/=+$//;
		transform $str, '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/', $chars;
	} else {
		my $ret = '';
		my $group_size = length $chars;
		my ($val, $ent) = (0, 0);
		for my $ch (unpack 'C*', $bytes) {
			$val = ($val * 256) | $ch;
			$ent = ($ent * 256) | 255;
			while ($ent >= $group_size) {
				$ret .= substr $chars, $val % $group_size, 1;
				$ent = int $ent / $group_size;
				$val = int $val / $group_size;
			}
		}
		if ($ent > 0) {
			$ret .= substr $chars, $val, 1;
		}
		return $ret;
	}
}
