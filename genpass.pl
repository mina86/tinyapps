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
sub encode16($;$);
sub encode32($;$);
sub encode64($;$);
sub encode85($;$);

sub randomBytes($);

sub transform($$$);


## Modifiers
my %modifiers = (
	'hex' => {
		'bytes' => [ 1, 2 ],
		'encode' => \&encode16,
		'characters' => undef,
		'transform' => undef,
	},

	'HEX' => {
		'bytes' => [ 1, 2 ],
		'encode' => \&encode16,
		'characters' => undef,
		'transform' => \&uc,
	},

	'alpha' => {
		'bytes' => [ 1, 2 ],
		'encode' => \&encode32,
		'characters' => undef,
		# result is all upper case plus some lower case
		'transform' => [ '0123456789', 'abcdefghij' ],
	},
	'al' => 'alpha',

	'b64' => {
		'bytes' => [ 3, 4 ],
		'encode' => \&encode64,
		'characters' => undef,
		'transform' => undef,
	},

	'a85' => {
		'bytes' => [ 4, 5 ],
		'encode' => \&encode85,
		'characters' => undef,
		'transform' => undef,
	},

	'guard'      => { 'prefix' => '/!' },
	'noguard'    => { 'prefix' => '' },
	'g' => 'guard',
	'G' => 'noguard',

	'strength'   => { 'strength' => 1 },
	'nostrength' => { 'strength' => 0 },
	's' => 'strength',
	'S' => 'nostrength',

	'clear'   => { 'prefix' => '', 'strength' => 0 },
	'c'       => 'clear',
    );

# Help
if (@ARGV == 1 && $ARGV[0] =~ /^(?:(?:--)?help|-h)$/) {
	print <<'END';
usage: $0 [ <options> ... ] [ <length> ]
Choosing characters set:
    hex          Use lower case hexadecimal characters in the password.
    HEX          Use upper case hexadecimal characters in the password.
    al alpha     Use only upper case letters and first 8 lower case letters.
    b64          Use characters used in base64 encoding (letters, digits,
                 plus and slash).
  * a85          Use characters used in Ascii85 encoding (letters, digits
                 and additional 23 printable characters).

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
my ($length, %O);
unshift @ARGV, qw(a85 guard strength);

do {
	$_ = shift @ARGV;

	if (/^\d+$/ && $_ != 0) {
		$length = int $_;
	} elsif (!exists $modifiers{$_} || /^-/) {
		die "$_: unknown modifier\n";
	} elsif ('HASH' eq ref $modifiers{$_}) {
		my $modifier = $modifiers{$_};
		for (keys %$modifier) {
			$O{$_} = $modifier->{$_};
		}
		undef $_;
	} elsif ('ARRAY' eq ref $modifiers{$_}) {
		unshift @ARGV, @{ $modifiers{$_} };
	} else {
		unshift @ARGV, $modifiers{$_};
	}
} while (@ARGV);

## Do the work
my $bytes = $O{'bytes'};

if (!defined $length) {
	$length = int((10 + $bytes->[0] - 1) / $bytes->[0]) * $bytes->[1];
}

$bytes = int(($length + $bytes->[1] + 2) / $bytes->[1]) * $bytes->[0];


$_ = $O{'encode'}(randomBytes $bytes, $O{'characters'});
if (defined $O{'transform'}) {
	if ('ARRAY' eq ref $O{'transform'}) {
		$_ = transform $_, $O{'transform'}[0], $O{'transform'}[1];
	} else {
		$_ = $O{'transform'}($_);
	}
}

my $excess = substr $_, $length;
$_ = $O{'prefix'} . substr $_, 0, $length;

if ($O{'strength'}) {
	my $i = 0;
	if (m/[A-Z]/) {
		$_ .= substr $excess, $i++, 1;
	} else {
		$_ .= chr(ord('A') + int rand 26);
	}
	if (m/[a-z]/) {
		$_ .= substr $excess, $i++, 1;
	} else {
		$_ .= chr(ord('a') + int rand 26);
	}
	if (!m/[0-9]/) {
		$_ .= substr $excess, $i++, 1;
	} else {
		$_ .= chr(ord('0') + int rand 10);
	}
}

print $_, "\n";


## Helper functions
sub transform($$$) {
	my ($str, $from, $to) = @_;
	if (defined $to) {
		if (length $from != length $to) {
			die "internal (<<$from>> <<$to>>)\n";
		}
		eval "\$str =~ tr[$from][$to], 1" or die $@;
	}
	$str;
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

## Encoders
sub encode16($;$) {
	my ($bytes, $chars) = @_;
	transform unpack('h*', $bytes), '0123456789abcdef', $chars;
}

sub encode32($;$) {
	my ($bytes, $chars) = @_;
	require MIME::Base32;
	MIME::Base32->import('RFC');
	transform MIME::Base32::encode($bytes), 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567', $chars;
}

sub encode64($;$) {
	my ($bytes, $chars) = @_;
	require MIME::Base64;
	MIME::Base32->import('RFC');
	transform MIME::Base64::encode($bytes), '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/', $chars;
}

sub encode85($;$) {
	my ($bytes, $chars) = @_;
	$chars = $chars // '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&()*+-;<=>?@^_/{|}~';

	my ($ret, $len) = ('', length $bytes);
	while ($len >= 4) {
		my ($num, $i) = (0, 4);
		do {
			$num = ($num << 8) | ord(substr($bytes, --$len));
		} while (--$i);

		$i = 5;
		do {
			$ret .= substr $chars, ($num % 85), 1;
			$num = int($num / 85);
		} while (--$i);
	}
	$ret;
}
