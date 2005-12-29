#!/usr/bin/perl
##
## Get lyrics from Internet for specified song
## $Id: getlyrics.pl,v 1.2 2005/12/29 18:11:49 mina86 Exp $
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
use Getopt::Long;
use LWP::UserAgent;

my $VERSION='0.2';

my $artist;
my $songname;

pod2usage() unless GetOptions(
    'artist=s' => \$artist,
    'song=s' => \$songname,
    'help|?' => sub { pod2usage(-verbose => 1); });

if (!$artist && !$songname && @ARGV) {
  my @foo = split(/\s-\s/, "@ARGV");
  if (@foo == 2) {
    $artist   = $foo[0];
    $songname = $foo[1];
  }
}

pod2usage() if (!$artist || !$songname);

my $lyrics = getlyricspage($artist, $songname);
if ($lyrics) {
  # If page contains forms, than lyrics is not found...
  if ($lyrics =~ /.*<form.*/ig) {
    die("Lyrics not found!\n");
  }
  print html2txt($lyrics);
};

sub getlyricspage {
  my ($artist, $songname) = @_;
  my $lyricspage = 'http://www.lyrc.com.ar/en/tema1en.php';

  my $ua = LWP::UserAgent->new();

  my $response = $ua->post($lyricspage, [
                        artist => $artist,
                        songname => $songname]);

  if ($response->is_success) {
    return $response->content;
  }
  else {
    return undef;
  }
}

sub html2txt {
  my $html = shift;

  $html =~ s/<head>.*<\/head>/ /igs;  # Remove HTML Heading
  $html =~ s/<scrip.*?script>/ /igs;  # Remove Script blocks
  $html =~ s/<a.*?\/a>/ /igs;         # Remove Links
  $html =~ s/&nbsp;/ /g;              # Replace &nbsp with spaces
  $html =~ s/\s\s*/ /g;               # Replace repeated space characters with spaces
  $html =~ s/<p[^>]*>/\n\n/gi;        # Replace paragraph tags with new line mark
  # Replace some formating html tags witConverth new line marks
  $html =~ s/<br.*?>|<\/*h[1-6][^>]*>|<li[^>]*>|<dt[^>]*>|<dd[^>]*>|<\/tr[^>]*>/\n/gi;
  $html =~ s/(<[^>]*>)+//g;
  $html =~ s/\n\s*\n\s*/\n\n/g;
  $html =~ s/\n *| *\n/\n/g;
  $html =~ s/^\n\n//mg;

  $html =~ s/^([^\n]+\n[^\n]+\n)/$1\n\n/g;
  return "$html";
}

__END__

=head1 NAME

getlyrics - Get lyrics from Internet for specified song

=head1 DESCRIPTION

The getlyrics utility retrieves lyrics from the Internet for the specified
artist and song name. Internaly, this utility uses http://www.lyrc.com.ar/
to search for lyrics.

=head1 SYNOPSIS

getlyrics --artist="Artist" --song="Song Name"

getlyrics Artist - Song Name

=head1 OPTIONS

=over 8

=item B<--artist="Artist">

Specifies artist for the searched lyrics.

=item B<--song="Song Name">

Specifies song name for the searched lyrics.

=item B<Artist - Song Name>

Specifies both, artist and song name for the searched lyrics.  May be
given as any number of arguments.

=item B<--help>

Display this help and exit.

=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>

=cut
