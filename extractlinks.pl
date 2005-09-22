#!/usr/bin/perl -w
##
## Extracts links from a HTML page
## $Id: extractlinks.pl,v 1.1 2005/09/22 16:41:28 mina86 Exp $
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
use Getopt::Long;
use Pod::Usage;

use URI::URL;
use LWP::UserAgent;
use HTML::LinkExtor;

#program version
my $VERSION='0.2';

#For CVS , use following line
#my $VERSION=sprintf("%d.%02d", q$Revision: 1.1 $ =~ /(\d+)\.(\d+)/);

my $url = '';
my $linktag = 'all';
my @links = ();

pod2usage() unless GetOptions(
    'tag=s' => \$linktag,
    'help|?' => sub { pod2usage(-verbose => 1); });
pod2usage() if ($#ARGV != 0);

$url = shift(@ARGV);

my $ua = LWP::UserAgent->new;
my $parser = HTML::LinkExtor->new(\&linkcallback);
my $res = $ua->request(HTTP::Request->new(GET => $url),
                       sub { $parser->parse($_[0]) });

if (!$res->is_success) {
  print('Parse failed: ');
  print($res->status_line);
  print("\n");
  exit(1);
}

my $base = $res->base;
@links = map { $_ = url($_, $base)->abs; } @links;

print(join("\n", @links), "\n");

sub linkcallback {
  my ($tag, %attr) = @_;
  if ($linktag ne 'all') {
    if ($linktag eq $tag) {
      push(@links, values(%attr));
    }
  }
  else {
    push(@links, values(%attr));
  }
}

__END__

=head1 NAME

extractlinks - Extracts links from a HTML page

=head1 DESCRIPTION

The extractlinks utility shall search the HTML file specified by
the url parameter and extract all contained links. The url must be
specified as absolute URL.

=head1 SYNOPSIS

extractlinks [--tag=all|img|a|link|...] url

=head1 OPTIONS

=over 8

=item B<--tag=all|a|img|link|...>

Specifies type of HTML tag to search for link. The default option
is all, so that all HTML tag links are extracted.

=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>

=cut
