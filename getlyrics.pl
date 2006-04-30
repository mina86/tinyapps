#!/usr/bin/perl
##
## Get lyrics from Internet for specified song
## $Id: getlyrics.pl,v 1.6 2006/04/30 18:45:15 mina86 Exp $
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
use LWP::UserAgent;

my $VERSION = '0.3';


##
## Parse args
##
if (!@ARGV) {
	pod2usage( -exitval => 1, -verbose => 0, -output => \*STDERR );
} elsif ($ARGV[0] eq '--help' || $ARGV[0] eq '-h' || $ARGV[0] eq '-?') {
	pod2usage( -exitval => 0, -verbose => 1 );
} elsif ($ARGV[0] eq '--man') {
	pod2usage( -exitval => 0, -verbose => 2 );
}


my ($src, $arg, $foo, @foo) = ('args');;
if ($ARGV[0] =~ /^--(.*)$/) {
	$src = $1;
	shift;
}
$arg = "@ARGV";


##
## Source: mpc
##
if ($src eq 'mpc') {
	$src = 'pipe';
	$arg = 'mpc --format \'%artist% - %title%\' | head -1';
} elsif ($src eq 'file') {
	$src = 'pipe';
	$arg = "cat $arg";
} elsif ($src eq 'read') {
	$src = 'pipe';
	$arg = "cat";
}


##
## Source: mpd
##
if ($src eq 'mpd') {
	my $port = undef;
	if ($arg =~ m/(.*)(?::(\d+))/) {
		$arg = $1;
		$port = $2;
	}

	use Audio::MPD;
	$arg = new Audio::MPD($arg, $port);
	if (!$arg) {
		die "Could not connect to MPD.\n";
	}

	$foo = $arg->get_title();
	$foo =~ s#^.*/##;
	$arg->close_connection();


##
## Source: pipe
##
} elsif ($src eq 'pipe') {
	$foo = `$arg 2>/dev/null`;
	chomp $foo;


##
## Source: args
##
} elsif ($src eq 'args' || $src eq '') {
	if (!@ARGV) {
		die "Artist nad song name required\n";
	} elsif (@ARGV == 2) {
		@foo = @ARGV;
	} elsif (@ARGV == 3 && $ARGV[1] =~ /\s*-\s*/) {
		@foo = ($ARGV[0], $ARGV[1]);
	} else {
		$foo = "@ARGV";
	}


##
## Unknown source
##
} else {
	die "Invalid argument: --$src\n";
	exit 1;
}


##
## Parse $foo and @foo
##
if (defined($foo)) {
	$_ = $foo;
	s/^[\s-]+|[\s-]*$//g;
	s/\s+/ /g;
	if (m/^(.+?) - (.+)$/ || m/^(\S+) (.+)$/ || m/^(.+?)-([^ ].*)$/) {
		@foo = ($1, $2);
	} else {
		die "Cannot parse: $foo\n";
	}
}


##
## Download page
##
$foo = LWP::UserAgent->new()->post(
	'http://www.lyrc.com.ar/en/tema1en.php', [
		artist   => $foo[0],
		songname => $foo[1]
	]);
exit 1 unless ($foo->is_success);
$foo = $foo->content;


##
## HTML to text
##
$foo =~ s/<head>.*<\/head>/ /igs;  # Remove HTML Heading
$foo =~ s/<scrip.*?script>/ /igs;  # Remove Script blocks
$foo =~ s/<a.*?\/a>/ /igs;         # Remove Links
$foo =~ s/&nbsp;/ /g;              # Replace &nbsp with spaces
$foo =~ s/\s+/ /g;                 # Squeeze blanks
$foo =~ s/<p[^>]*>/\n\n/gi;        # Replace paragraph tags with empty line
# Replace some formating html tags witConverth new line marks
$foo =~ s/<(?:br|h[1-6]|li|d[td]||tr)[^>]*>/\n/gi;
$foo =~ s/(<[^>]*>)+//g;           # Remove HTML tags
$foo =~ s/\n\s*\n\s*/\n\n/g;       # Squeeze blank lines
$foo =~ s/^ +| +$//mg;             # Trim lines
$foo =~ s/^\s+|\s$//g;             # Trime whole code
# Add 2 empty lines after song name and artist
$foo =~ s/^([^\n]+)\n+([^\n]+)\n+/$1\n$2\n\n\n/g;

print $foo, "\n";


__END__

=head1 NAME

getlyrics - Get lyrics from Internet for specified song

=head1 DESCRIPTION

The getlyrics utility retrieves lyrics from the Internet for the specified
artist and song name. Internally, this utility uses http://www.lyrc.com.ar/
to search for lyrics.

=head1 SYNOPSIS

getlyrics.pl --help | --man

getlyrics.pl --I<scheme> I<arguments>

=head1 OPTIONS

Script need to know artist and song name of the track you want lyrics
for. There are several schemes which instructs getlyrics.pl how should
it obtain this information. Some schemes requires arguments and if
they do you shall specify them as command line arguments just after the
B<-->I<scheme> part.

If no scheme is given (ie. the first command line argument does not
start with B<-->) or empty scheme (ie. the first command line argument
is B<-->) B<--args> is assumed.

=over 8

=item B<--args>

Will get artist and song name from given arguments.  If two arguments
are given the first will be used as artist and the second as song
name.  If three arguments are given and the second is a single minus
sign with optional white spaces the first argument will be used as
artist and the third as song name.  Otherwise all arguments will be
concated and parsed as described in PARSING STRING section of man page (see
B<--man>).

=item B<--pipe>

Script will run command given as arguments and parse it's output
according to the rules described in PARSING STRING section of man page
(see B<--man>).

=item B<--file>

Reads given and parses their output. B<--file >I<files> is a synonym
of B<--pipe cat >I<files>.

=item B<--read>

Synonym of B<--pipe cat>.

=item B<--mpd>

Instructs the script to get the song artist and title from MPD.
Argument to this scheme is of the fallowing format:
[password@][host][:port]. If host or port is not specified environment
variables B<MPD_HOST> and B<MPD_PORT> are checked.  Finally if all
else fails the defaults B<localhost> and B<6600> are used.

If tags for artist and song name are missing script will try to parse
current song's file name according to the rules described in PARSING
STRING section of man page (see B<--man>).

=item B<--mpc>

Synonym of B<--pipe "mpc --format '%artist% - %title%' | head -1">

=back

=head1 PARSING STRING

Output of a pipe and some other strings need to be parsed to obtain
artist and song name. First, white space and minus signs are removed
from the beginning and the end of the string. Then, all white spaces
are changed into a single space character. Finally, the string is
splited into two strings first using ' - ' and if that fails a single
space character and if that fails a single minus sign.

For example, "Metallica - One", "Metallica One" and "Metallica-One"
are splited into "Metallica" being artist and "One" being song
name. However, "Metallica -One" will be splited into "Metallica" and
"-One". Also "Black Sabbath Changes" and "Black Sabbath-Changes" won't
be splited into "Black Sabbath" and "Changes" so you have to use
"Black Sabbath - Changes".

=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>,
Michal Nazarewicz <mina86@projektcode.org>

=cut
