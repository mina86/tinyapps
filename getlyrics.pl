#!/usr/bin/perl
##
## Get lyrics from Internet for specified song
## Copyright (c) 2005-2010 by Michal Nazarewicz (mina86/AT/mina86.com)
## Copyright (c) 2009      by Mirosław "Minio" Zalewski <miniopl@gmail.com>
##                         http://minio.xt.pl (lyrics retrieving code)
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

use LWP::UserAgent;
use HTML::TreeBuilder;

use encoding "utf8";
use Encode;
use Text::Unidecode;

my $VERSION = '0.5';
my $GLOBAL_CACHE = '/usr/share/lyrics/';
my $LOCAL_CACHE  = $ENV{'HOME'} . '/.lyrics/';


## editFile ($file, $editor, $delete)
sub editFile ($$$);


##
## Parse args
##
my ($edit, $editor) = ( 0, '' );
if (@ARGV && $ARGV[0] =~ /^--edit(?:=(.*))?$/) {
	$edit = 1;
	$editor = $1 ? $1 : '';
	shift @ARGV;
}

if (!@ARGV) {
	pod2usage( -exitval => 1, -verbose => 0, -output => \*STDERR );
} elsif ($ARGV[0] eq '--help' || $ARGV[0] eq '-h' || $ARGV[0] eq '-?') {
	pod2usage( -exitval => 0, -verbose => 1 );
} elsif ($ARGV[0] eq '--man') {
	pod2usage( -exitval => 0, -verbose => 2 );
}


my ($src, $arg, $foo, @foo) = ('args');
if ($ARGV[0] =~ /^--(.*)$/) {
	$src = $1;
	shift;
}
$arg = "@ARGV";


sub run($) {
	my $ret = `$_[0]`;
	if ($?) {
		die $_[1] . ": failed\n";
	}
	$ret;
}


##
## Source: mpc
##
if ($src eq 'mpc') {
	$foo[0] = run 'mpc --format %artist% | head -n 1';
	$foo[1] = run 'mpc --format %title%  | head -n 1';


##
## Source: audacious
##
} elsif ($src eq 'audacious' || $src eq 'aud' || $src eq 'audtool') {
	$foo[0] = run 'audtool current-song-tuple-data performer' ;
	$foo[1] = run 'audtool current-song-tuple-data track_name';


##
## Source: mpd
##
} elsif ($src eq 'mpd') {
	my $port = undef;
	if ($arg =~ m/(.*)(?::(\d+))/) {
		$arg = $1;
		$port = $2;
	}

	eval { require Audio::MPD; } or
		die "You are missing Audio::MPD module.\n";

	$arg = new Audio::MPD($arg, $port);
	if ($arg) {
		eval { # New API
			my $song = $arg->song();
			@foo = ( $song->artist(), $song->title() );
			1;
		} or eval { # Old API
			$foo = $arg->get_title();
			$foo =~ s#^.*/##;
			$arg->close_connection();
		}
	} else {
		die "Could not connect to MPD.\n";
	}


##
## Source: pipe
##
} elsif ($src eq 'pipe') {
	$foo = run $arg;


##
## Source: file or read
##
} elsif ($src eq 'file' || $src eq 'read') {
	$foo = join '', <>;


##
## Source: xmms
##
} elsif ($src eq 'xmms') {

	$foo = eval {
		require XMMS::InfoPipe;
		my $xmms = XMMS::InfoPipe->new();
		return unless $xmms->is_running;
		return $xmms->{'info'}->{'Title'} if defined $xmms->{'info'}->{'Title'};
		return $xmms->{'info'}->{'File'}  if defined $xmms->{'info'}->{'File'} ;
		return; # unusual event

	} || eval {
		my ($in, $out, $ret);
		return unless sysopen($out, $ENV{'HOME'} . '/.xmms/inpipe' , 1);
		return unless sysopen($in , $ENV{'HOME'} . '/.xmms/outpipe', 0);
		syswrite $out, "\nout flush\nreport title\n";
		return unless sysread $in, $ret, 4096;
		return $ret;
	};

	if ($foo) {
		$foo =~ s#.*[/\\]##;
	} else {
		die "Could not obtain song from XMMS\n";
	}


##
## Source: args
##
} elsif ($src eq 'args' || $src eq '') {
	if (!@ARGV) {
		die "Artist and song name required\n";
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

$_ =~ s/\s+/ /mg for @foo;
$_ =~ s/^ | $//g for @foo;
$_ =~ s/--+/-/g  for @foo;


##
## Try cache
##
for ($LOCAL_CACHE  . '/' . lc($foo[0]) . '/' . lc($foo[1]),
     $GLOBAL_CACHE . '/' . lc($foo[0]) . '/' . lc($foo[1])) {
	if (-e) {
		if ($edit) {
			editFile $_, $editor, 0;
			exit 0;
		} elsif (open FH, '<', $_) {
			print while <FH>;
			close FH;
			exit 0;
		} else {
			warn "$_: $!\n";
		}
	}
}



##
## Download page
##

sub sane_str($) {
	my $string = shift;
	$string = decode('UTF-8', $string) if utf8::is_utf8($string) != 1;
	$string = unidecode($string);
	$string =~ tr/ /-/;
	$string =~ s/[,'\.]//gi;
	$string;
}


sub get_page($) {
	my $url = shift;

	print STDERR $url . "\n";

	my $ua = new LWP::UserAgent;
	my $r = $ua->get($url);

	if (!$r->is_success) {
		if (int($r->code / 100) == 5) {
			print STDERR $r->code, ", trying again in 5 s\n";
			sleep 5;
			$r = $ua->get($url);
		}
		if (!$r->is_success) {
			print STDERR $r->code, ", aborting\n";
		}
	}

	$r->is_success ? $r->content : undef;
}


my @sites = (
	sub { # http://www.lyricsondemand.com/
		my $artist = sane_str shift;
		my $title  = sane_str shift;

		my $f = substr($artist, ($artist =~ m/^the /i) ? 4 : 0, 1);
		$artist =~ s/[ \(\)-]//g;
		$title  =~ s/[ \(\)-]//g;

		my $url = 'http://www.lyricsondemand.com/' . lc($f) . '/' .
			lc($artist) . 'lyrics/' . lc($title) . 'lyrics.html';

		my $content = get_page($url);
		return unless $content;

		my $text = HTML::TreeBuilder->new_from_content($content)->look_down(
			"_tag", "font",
			"face", "Verdana",
			"size", "2",
			)->as_HTML('<>&');

		my @splitted = split(/<br \/>/i, $text);
		$text = '';

		foreach my $line (@splitted) {
			$line =~ s:<.+?>::gi;
			$line =~ s:^\s*::gi;
			$text .= $line . "\n";
		}
		$text;
	},

	sub { # http://www.azlyrics.com/
		my $artist = sane_str shift;
		my $title  = sane_str shift;

		$artist =~ s/[ \(\)-]//g;
		$title  =~ s/[ \(\)-]//g;

		my $url = 'http://www.azlyrics.com/lyrics/' . lc($artist) . '/' .
			lc($title) . '.html';

		my $content = get_page($url);
		return unless $content;

		my $text = HTML::TreeBuilder->new_from_content($content)->look_down(
			"_tag", "font",
			"face", "Verdana",
			"size", "5",
			)->look_down(
			"_tag", "font",
			"size", "2",
			)->as_HTML('<>&');

		my @splitted = split(/<br \/>/i, $text);
		$text = '';

		my $begin;
		foreach my $line (@splitted) {
			if ($line =~ m:<b>.+</b>:i) {
				$begin = 1;
				next;
			}
			if ($line =~ m:\[Thanks to:i) {
				last;
			}
			if ($begin == 1) {
				$line =~ s:^\s*::gi;
				$text .= $line ."\n";
			}
		}
		$text;
	},

	sub { # http://www.tekstowo.pl/
		my ($artist, $title) = @_;
		my $text;

		# Tekstowo zamiast ń chce mieć podkreślnik. Z tego powodu nie
		# mogę użyć sane_str, gdyż po nim ń byłoby nie do odróżnienia
		# od zwykłęgo n, i nie wiadomo co wtedy powinno być zamieniane
		# na podkreślnik. Niby można stworzyć funkcję która
		# przyjmowałaby tekst przed transliteracją, wyszukiwała ń,
		# i w tych miejscach zamieniała znak na podkreślnik w tekstach
		# po transliteracji, ale przysporzyłoby to więcej roboty niż
		# to warte. Dlatego dokonuję prostej transliteracji.
		for my $str ($artist, $title) {
			$str = decode('UTF-8', $str) if utf8::is_utf8($str) != 1;
			$str =~ tr/ĄĆĘŁŃÓŚŻŹąćęłńóśćżź /ACEL_OSZZacel_oszz_/;
			$str =~ s/[,'\.]//gi;
		}

		my $url = 'http://www.tekstowo.pl/piosenka,' . lc($artist) . ',' .
			lc($title) . '.html';

		my $content = get_page($url);
		return unless $content;

		# Tekstowo zwraca 200 OK nawet jeżeli nie znaleziono szukanego tekstu.
		eval {
			$text = HTML::TreeBuilder->new_from_content($content)->look_down(
				"_tag", "div",
				"id", "tex",
				)->look_down(
				"_tag", "div",
				)->as_HTML('<>&');
		};
		return if $@;

		$text = decode('ISO-8859-2', $text);
		$text =~ s/<br \/>\s?/\n/gi;
		$text =~ s:<.+?>::gi;
		$text;
	}
);



print STDERR "Getting lyrics for ${foo[0]} - ${foo[1]}\n";

my $res;
for (@sites) {
	$res = $_->(@foo);
	last if defined $res;
}

die "Could not find lyrics.\n" unless defined $res;

$res =~ s/^\s+|\s+$//g;
$res .= "\n";

unless ($edit) {
	print $res;
}


##
## Save in cache
##
my ($dir, $file);
for ($GLOBAL_CACHE, $LOCAL_CACHE) {
	$dir  = $_ . '/' . lc $foo[0];
	$file = $dir . '/' . lc $foo[1];

	if (-d $dir) {
		next if ! -w _;
	} else {
		next if ! -d || ! -w _;
		if (!mkdir $dir) {
			warn "$dir: $!\n";
			next;
		}
	}

	if (open FH, '>', $file) {
		print FH $res;
		close FH;
		if ($edit) {
			editFile $file, $editor, 0;
		}
		exit 0;
	}

#	warn "$file: $!\n";
}


##
## Save in /tmp and edit
##
if ($edit) {
	$file = $foo[0] . ' - ' . $foo[1];
	for ($ENV{'TEMP'}, $ENV{'TMP'}, '/tmp') {
		next unless -d && -w _;
		unless (open FH, '>', "$_/$file") {
			warn "$_/$file: $!\n";
			next;
		}
		print FH $res;
		close FH;
		editFile "$_/$file", $editor, 1;
	}
}



sub editFile ($$$) {
	my ($file, $editor, $delete) = @_;

	for my $e ($editor, $ENV{'VISUAL'}, $ENV{'EDITOR'}, 'vi') {
		if (defined $e && $e) {
			$editor = $e;
			last;
		}
	}

	$ENV{'GETLYRICS_LYRICS_FILE'} = $file;
	unless ($delete) {
		exec "$editor \"\$GETLYRICS_LYRICS_FILE\"";
		die "exec: $!\n";
	}

	system "$editor \"\$GETLYRICS_LYRICS_FILE\"";
	unlink $file;
	exit $?;
}


__END__

=head1 NAME

getlyrics - Get lyrics from Internet for specified song

=head1 DESCRIPTION

The getlyrics utility retrieves lyrics from the Internet for the specified
artist and song name.

=head1 SYNOPSIS

getlyrics.pl --help | --man

getlyrics.pl [ --edit[=<editor>] ] --I<scheme> I<arguments>

=head1 OPTIONS

Script need to know artist and song name of the track you want lyrics
for. There are several schemes which instructs getlyrics.pl how should
it obtain this information. Some schemes requires arguments and if
they do you shall specify them as command line arguments just after the
B<-->I<scheme> part.

If no scheme is given (ie. the first command line argument (or the
second if first was B<--edit>) does not start with B<-->) or empty
scheme (ie. the first command line argument is B<-->) B<--args> is
assumed.

If B<--edit> option was given instead of printing lyrics script will
run an editor.  If lyrics were not in cache and script didn't manage
to save them there it will save it in temporary location ($TEMP, $TMP
or "/tmp") and delete it after editor exists.  As an editor script
will try argument given to B<--edit> option, $VISUAL environment
variable, $EDITOR environment variable or "vi" whatever is set.

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

=item B<--read>

Reads content of files given as arguments or from standard input if no
files where given.

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

Runs B<mpc --format %artist%> to obtain artist name and then B<mpc
--format %title%> to obtain title.

=item B<--audacious>

=item B<--audtool>

=item B<--aud>

Runs B<audtool current-song-tuple-data performer> to obtain artist
name and then B<audtool current-song-tuple-data track_name> to obtain
title.

=item B<--xmms>

Instructs the script to get the song artist from XMMS.  Script first
tries to use XMMS::InfoPipe module.  If it doesn't exist or
xmms-infopipe is not running, script tries to open
B<$HOME/.xmms/inpipe> and B<$HOME/.xmms/outpipe> pipes to use xmmspipe
plugin.

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

=head1 CACHE

Starting from version 0.5 this script supports caching.  That is,
lyrics once downloaded don't get downloaded again.  There are two
places where cached lyrics are kept: a global cache under
B</usr/share/lyrics/> and a local, per-user cache under B<~/.lyrics>.
When reading lyrics or saving them script first tries global and then
local cache.  To enable caching at least one of those directories have
to be created.  Note also, that the global cache is meant to be filled
by root and that lyrics kept there are available for all users to read
but only for root (or some other privileged user) to write.


=head1 AUTHOR

Berislav Kovacki <beca@sezampro.yu>,
Michal Nazarewicz <mina86@mina86.com>

=cut
