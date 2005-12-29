#!/usr/bin/perl -w
##
## Extracts links from a HTML page
## $Id: extractlinks.pl,v 1.2 2005/12/29 18:47:30 mina86 Exp $
## Copyright (C) 2005 by Berislav Kovacki (beca/AT/sezampro.yu)
## Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/tlen.pl)
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
use File::Spec;

#program version
my $VERSION='0.2';

#For CVS , use following line
#my $VERSION=sprintf("%d.%02d", q$Revision: 1.2 $ =~ /(\d+)\.(\d+)/);

my $url = '';
my $linktag = 'all';
my @links = ();

pod2usage() unless GetOptions(
    'tag=s' => \$linktag,
    'help|?' => sub { pod2usage(-verbose => 1); });
pod2usage() if (@ARGV == 0);

my $error = 0;

while (@ARGV) {
  $url = shift(@ARGV);
  if ($url !~ /^[a-z]+:\/\//) {
    if (File::Spec->file_name_is_absolute($url)) {
      $url = "file://$url";
    } else {
      $url = 'file://' . File::Spec->rel2abs($url);
    }
  }

  my $ua = LWP::UserAgent->new;
  my $parser = HTML::LinkExtor->new(\&linkcallback);
  my $res = $ua->request(HTTP::Request->new(GET => $url),
                         sub { $parser->parse($_[0]) });

  if (!$res->is_success) {
    print('Parse failed: ');
    print($res->status_line);
    print("\n");
    $error = 1;
  } else {
    my $base = $res->base;
    @links = map { $_ = url($_, $base)->abs; } @links;

    print(join("\n", @links), "\n");
  }
}

exit($error);

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

extractlinks [--tag=all|img|a|link|...] url [ ... ]

=head1 OPTIONS

=over 8

=item B<--tag=all|a|img|link|...>

Specifies type of HTML tag to search for link. The default option
is all, so that all HTML tag links are extracted.

=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>

=cut
