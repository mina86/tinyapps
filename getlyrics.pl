#!/usr/bin/perl
##
## Get lyrics from Internet for specified song
## $Id: getlyrics.pl,v 1.5 2006/04/29 23:55:09 mina86 Exp $
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
use Pod::Usage;
use Getopt::Long;
use LWP::UserAgent;

my $VERSION = '0.3';

my $artist;
my $songname;
my $mpd = undef;
my $pipe = undef;
my @foo = ();


## Parse args
pod2usage() unless GetOptions(
	'a|artist=s' => \$artist,
	's|song=s'   => \$songname,
	'm|mpd:s'    => \$mpd,
	'p|pipe=s'   => \$pipe,
	'help|h|?'   => sub { pod2usage(-verbose => 1); });


## Read title from MPD
if (defined($mpd) && $mpd eq 'mpc') {
	undef $mpd;
	$pipe = 'mpc --format \'%artist% - %title%\' | head -1';
}

if (defined($mpd)) {
	my $port;
	if ($mpd =~ m/(.*)(?::(\d+))/) {
		$mpd = $1;
		$port = $2;
	}

	use Audio::MPD;
	$mpd = new Audio::MPD($mpd, $port);
	if (!$mpd) {
		print "Could not connect to MPD\n";
		exit 1;
	}

	$songname = $mpd->get_title();
	$songname =~ s#^.*/##;
	@foo = split(/\s-\s/, $songname);
	if (@foo == 2) {
		print "MPD plays: $songname\n";
		$artist   = $foo[0];
		$songname = $foo[1];
	} else {
		print "MPD plays: $songname; could not parse\n";
		exit 1;
	}

	$mpd->close_connection();


## Read title from pipe
} elsif (defined($pipe)) {
	$songname = `$pipe 2>/dev/null`;
	chomp $songname;
	@foo = split(/\s-\s/, $songname);
	if (@foo == 2) {
		print "Pipe returned: $songname\n";
		$artist   = $foo[0];
		$songname = $foo[1];
	} else {
		print "Pipe returned: $songname; could not parse\n";
		exit 1;
	}
}


## Parse args
if (!$artist && !$songname && @ARGV) {
	@foo = split(/\s-\s/, "@ARGV");
	if (@foo == 2) {
		$artist   = $foo[0];
		$songname = $foo[1];
	}
}

## Nothing given
pod2usage() if (!$artist || !$songname);

## Get and display
my $lyrics = getlyricspage($artist, $songname);
if ($lyrics) {
  # If page contains forms, than lyrics is not found...
  if ($lyrics =~ /.*<form.*/ig) {
    die("Lyrics not found!\n");
  }
  print html2txt($lyrics);
};


## Download page
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


## Convert HTML to text
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

getlyrics [ --artist="Artist" --song="Song Name" |  Artist - Song Name | --mpd=pass@host:port | --mpd=mpc | --pipe=command ]

=head1 OPTIONS

=over 8

=item B<-a --artist="Artist">

Specifies artist for the searched lyrics.

=item B<-s --song="Song Name">

Specifies song name for the searched lyrics.

=item B<Artist - Song Name>

Specifies both, artist and song name for the searched lyrics.  May be
given as any number of arguments so no quoting is required.

=item B<-m --mpd=pass@host:port>

Instructs the script to get the song artist and title from MPD.  You
may specify a hostname and port, if none is specified then the
enviroment variables B<MPD_HOST> and B<MPD_PORT> are checked.  Finally
if all else fails the defaults B<localhost> and B<6600> are used.  An
optional password can be specified by prepending it to the hostname,
seperated an B<@> character.

=item B<-m --mpd=mpc>

Synonym of B<--pipe="mpc --format '%artist% - %title%' | head -1">

=item B<-p --pipe=cmd>

Script will run I<cmd> and take it's output as artist and song name.

=back

=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>,
Michal Nazarewicz <mina86@projektcode.org>

=cut
