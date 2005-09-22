#!/usr/bin/perl
##
## Pings specified range of IP addresses
## $Id: pingrange.pl,v 1.1 2005/09/22 16:41:28 mina86 Exp $
## Copyright (C) 2005 by Berislav Kovacki (beca/AT/sezampro.yu)
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##

#
# Documentation at the end of file.
#

use strict;
use warnings;

use English;
use Pod::Usage;
use Net::Ping;
use Getopt::Long;

my $VERSION='0.2';

my $timeout = 3;
my $protocol = 'tcp';
my $address = '';

$OUTPUT_AUTOFLUSH = 1;

pod2usage() unless GetOptions(
    'timeout=i' => \$timeout,
    'protocol=s' => \$protocol,
    'help|?' => sub { pod2usage(-verbose => 1); });
pod2usage() if ($#ARGV != 0);

$address = shift @ARGV;

my @lowip;
my @hiip;

($lowip[0], $lowip[1], $lowip[2], $lowip[3],
 $hiip[0],  $hiip[1],  $hiip[2],  $hiip[3] ) = getiprange($address);

my ($ip1, $ip2, $ip3, $ip4);
my ($ping, $host);

foreach $ip1 ($lowip[0]..$hiip[0]) {
  foreach $ip2 ($lowip[1]..$hiip[1]) {
    foreach $ip3 ($lowip[2]..$hiip[2]) {
      foreach $ip4 ($lowip[3]..$hiip[3]) {
        $host = "$ip1.$ip2.$ip3.$ip4";
        $ping = Net::Ping->new($protocol, $timeout);
        print "$host [ping]\n" if $ping->ping($host, 1);
        $ping->close();
      }
    }
  }
}

sub getiprange {
  my $range = shift;
  my @lowip;
  my @hiip;

  if (!($range =~ /^(\d{1,3}|\d{1,3}-\d{1,3})\.(\d{1,3}|\d{1,3}-\d{1,3})\.(\d{1,3}|\d{1,3}-\d{1,3})\.(\d{1,3}|\d{1,3}-\d{1,3})$/)) {
    pod2usage();
  }

  ($lowip[0], $hiip[0]) = decoderange($1);
  ($lowip[1], $hiip[1]) = decoderange($2);
  ($lowip[2], $hiip[2]) = decoderange($3);
  ($lowip[3], $hiip[3]) = decoderange($4);

  return (@lowip, @hiip);
}

sub decoderange
{
  my $range = shift;
  my @retval = (0, 255);

  if ($range =~ /^(\d+)-(\d+)$/) {
    @retval = ($1, $2);
  }
  else
  {
    @retval = ($range, $range);
  }

  return @retval;
}

__END__

=head1 NAME

pingrange - Sends ping to a range of hosts.

=head1 DESCRIPTION

The pingrange utility sends ping to a range of network hosts.

=head1 SYNOPSIS

pingrange [OPTIONS] address_range

=head1 OPTIONS

=over 8

=item B<--timeout=sec>

Sets the ping timeout to number of seconds. Default is 3 seconds.

=item B<--protocol=tcp|udp|icmp>

Sets the type of ping protocol. The default protocol is tcp.
You may have to be root to use icmp ping.

=item B<--help>

Display this help and exit.

=item B<address_range>

address_range format must be of a1[-a2].b1[-b2].c1[-c2].d1[-d2] form.
For example: 192.168.0-2.0-128

=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>

=cut
