#!/usr/bin/perl
##
## Pings specified range of IP addresses
## Copyright (c) 2005 by Berislav Kovacki (beca/AT/sezampro.yu)
## Copyright (c) 2006 by Michal Nazarewicz (mina86/AT/mina86.com)
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

#
# Documentation at the end of file.
#

use strict;
use warnings;

use Pod::Usage;
use Net::Ping;
use Getopt::Long;




my $VERSION='0.2';
$| = 1;



##
## Arguments
##
my $timeout = 3;
my $protocol = 'tcp';
pod2usage() unless GetOptions(
    't|timeout=i'  => \$timeout,
    'p|protocol=s' => \$protocol,
    'h|help|?'     => sub { pod2usage(-verbose => 1); },
    'man'          => sub { pod2usage(-verbose => 2); });

pod2usage( -exitval => 1, -verbose => 0, -output => \*STDERR )
	 unless @ARGV;



##
## Decode
##
my ($error, @ranges, @decoded);
LOOP: while ($_ = shift @ARGV) {
	my ($ip, $mask);

	# nnn.nnn.nnn.nnn/nn
	if (/^(?:\d+\.){3}\d+\/\d+$/) {
		($ip, $mask) = split(/\//);
		next if $mask >= 32;
		$mask = 0xffffffff ^ ((1 << $mask) - 1);
		$_ = $ip . '/' . ($mask>>24) . '.' .  (($mask>>16) & 0xff) . '.' .
			(($mask>>16) & 0xff) . '.' . ($mask & 0xff);
		# pass thrugh
	}

	# nnn.nnn.nnn.nnn/nnn.nnn.nnn.nnn
	if (/^(?:\d+\.){3}\d+\/(?:\d+\.){3}\d+$/) {
		($ip, $mask) = split(/\//);
		my @ip   = split(/\./, $ip  );
		my @mask = split(/\./, $mask);
		for $ip (@ip) {
			$mask = shift @mask;
			next LOOP if $ip > 255 || $mask > 255;
			push @decoded, $ip & $mask, $ip | ($mask ^ 0xff);
		}
		next;
	}

	# nnn-nnn.nnn-nnn.nnn-nnn.nnn-nnn
	if (/^(?:\d+(?:-\d+)?\.){3}\d+(?:-\d+)?$/) {
		for (split(/\./)) {
			if (/^(\d+)-(\d+)$/) {
				next LOOP if $1 > 255 || $2>255 || $1>$2;
				push @decoded, $1, $2;
			} else {
				next LOOP if $_ > 255;
				push @decoded, $_, $_;
			}
		}
		next;
	}

} continue {
	if (@decoded==8) {
		push @ranges, [ @decoded ];
	} else {
		print STDERR "pingrange: $_: invalid IP range\n";
		$error = 1;
	}
}

exit 1 if $error;


##
## Ping
##
$error = 1;
my $ping = Net::Ping->new($protocol, $timeout);
for (@ranges) {
	for my $ip1 ($_->[0]..$_->[1]) {
		for my $ip2 ($_->[2]..$_->[3]) {
			for my $ip3 ($_->[4]..$_->[5]) {
				for my $ip4 ($_->[6]..$_->[7]) {
					next unless $ping->ping("$ip1.$ip2.$ip3.$ip4");
					print "$ip1.$ip2.$ip3.$ip4 [ping]\n";
					$error = 0;
				}
			}
		}
	}
}
$ping->close();
exit $error;


__END__

=head1 NAME

pingrange - Sends ping to a range of hosts.

=head1 DESCRIPTION

The pingrange utility sends ping to a range of network hosts.

=head1 SYNOPSIS

pingrange [OPTIONS] I<address-range> [ I<address-range> ... ]

=head1 OPTIONS

=over 8

=item B<-h> B<--help>

Display help screen and exit.

=item B<--man>

Display man page and exit.

=item B<-t> B<--timeout=>I<sec>

Sets the ping timeout to I<sec> seconds.  Default is 3 second.

=item B<-p> B<--protocol=tcp|udp|icmp>

Sets the type of ping protocol.  The default protocol is tcp.  You may
have to be root to use icmp ping.

=item I<address_range>

I<address_range> may be specified several times and must be of one of the following forms:

=over 4

=item a.b.c.d/mask

=item a.b.c.d/A.B.C.D

Pins all hosts from given network.

=item a[-A].b[-B].c[-C].d[-D]

Pings all hosts from given range.

=back

For example:

=over 4

=item 192.168.0-2.0-128

=item 192.168.2.0/9

=item 192.168.2.0/255.255.254.0

=back

=back

=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>,
Michal Nazarewicz <mina86@mina86.com>
