#! /usr/bin/perl -w
##
## ChangeLog Viewer/Editor
## $Id: changelog.pl,v 1.3 2008/11/08 23:45:57 mina86 Exp $
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

use warnings;
use strict;

use Pod::Usage;
use File::Temp qw/ tempfile /;
use Text::Wrap;


sub err (@) { print STDERR join(': ', 'changelog', @_), "\n"; }
sub d   (@) { die          join(': ', 'changelog', @_), "\n"; }


## The directory must exist before first run of changelog.pl
#my $DIR = '/srv/changelog';
#my $DIR = '/usr/local/share/changelog';
#my $DIR = '/usr/share/changelog';
my $DIR = '/var/changelog';


##
## Add entry
##
if (@ARGV && ($ARGV[0] eq '-a' || $ARGV[0] eq '--add')) {
	my (undef, undef, undef, $mday, $mon, $date) = localtime();
	$date = sprintf '%02d%02d%02d', $date % 100, $mon + 1, $mday;
	my $message;

	shift;
	if (@ARGV) {
		# Message given at command line
		$message = join ' ', @ARGV;
	} else {
		$File::Temp::KEEP_ALL = 1;
		$File::Temp::DEBUG = 1;



		# Temporary file
		my ($fh, $fn) = tempfile();
		d 'tempfile: unable to create temporary file' unless defined $fn;

		# Run editor
		my $prog = $ENV{'EDITOR'} || $ENV{'VISUAL'} || 'vi';
		system $prog, $fn;
		if ($?==-1) {
			d 'system', $prog, "$!";
		} elsif ($? & 127) {
			d $prog, 'died with signal ' . ($? & 127);
		} elsif ($?) {
			d $prog, 'exited with ' . ($? >> 8);
		}

		# Read file
		$message = do { local $/; <$fh> };
		$message =~ s/^\s+|\s+$//g;
		close $fh;
		d 'no message' if $message eq '';
	}

	# Add entry
	d 'open', $DIR . '/' . $date, "$!"
		unless open FH, '>>', $DIR . '/' . $date;
	print FH 'Time: ', time() , "\n";
	print FH 'User: ', (getpwuid($<))[0], "\n" if $<;
	$message =~ s/\n/\n /;
	print FH $message, "\n";
	print FH "---------- END ----------\n";
	close FH;

	exit 0;
}



##
## Usage
##
if (@ARGV) {
	if ($ARGV[0] eq '--help' || $ARGV[0] eq '-h') {
		pod2usage( -exitval => 0, -verbose => 1 );
	} elsif ($ARGV[0] eq '--man') {
		pod2usage( -exitval => 0, -verbose => 2 );
	} elsif (@ARGV >=3 ) {
		pod2usage( -exitval => 1, -verbose => 0, -output => \*STDERR );
	}
}



##
## Home directory
##
sub _dir ($) { defined $_[0] && -d $_[0] ? $_[0] : undef; }
my $HOME = (
	_dir $ENV{'HOME'} or
	_dir $ENV{'LOGDIR'} or
	_dir eval { local $SIG{'__DIE__'} = ''; (getpwuid($<))[7]; } or
	_dir eval {
		use File::Glob ':glob';
		my $h = File::Glob::bsd_glob('~', GLOB_TILDE);
		$h eq '~' ? undef : $h;
	} or
	_dir $ENV{'USERPROFILE'} or
	_dir do {
		if ($ENV{HOMEDRIVE} && $ENV{HOMEPATH}) {
			File::Spec->catpath($ENV{HOMEDRIVE}, $ENV{HOMEPATH}, '');
		} else {
			undef;
		}
	} or
	_dir eval {
		require Mac::Files;
		Mac::Files::FindFolder(
			Mac::Files::kOnSystemDisk(),
			Mac::Files::kDesktopFolderType(),
			);
	}
) or err 'could not localize home directory';



##
## Parse arguments
##
my ($print_date, $date, $num, $new, @messages);

if (@ARGV == 0) {
	$num = 5;
}

if (@ARGV == 2) {
	($date, $num) = @ARGV;

	# Number
	d $num, 'invalid number' unless $num =~ /^\d+$/ && $num != 0;
}

if (@ARGV == 1) {
	$_ = $ARGV[0];
	my ($mday, $mon);

	# Number
	if (!defined $num && /^\d{1,3}$/) {
		$num = $_;
		d $num, 'invalid number' unless $num != 0;

	# Date
	} elsif (lc eq 'today') {
		(undef, undef, undef, $mday, $mon, $date) = localtime();
		$date = sprintf '%02d%02d%02d', $date % 100, $mon + 1, $mday;
		$print_date = lc;
	} elsif (lc eq 'yesterday') {
		(undef, undef, undef, $mday, $mon, $date) =
			localtime time() - 24*3600;
		$date = sprintf '%02d%02d%02d', $date % 100, $mon + 1, $mday;
		$print_date = lc;
	} elsif (lc eq 'new') {
		undef $date;
		$new = 1;
	} elsif (m#^(?:\d\d)(\d\d)([ -/]\d|[ -/]?\d\d)([ -/]\d|[ -/]?\d\d)$#) {
		($date, $mon, $mday) = ($1, $2, $3);
		s/^\D// for ($mon, $mday);
		$print_date = sprintf '20%02d/%02d/%02d', $date, $mon, $mday;
		$date = sprintf '%02d%02d%02d', $date, $mon, $mday;
	} else {
		d $_, 'invalid argument';
	}
}



##
## Read records
##
sub read_messages ($) {
	return () unless -f $DIR . '/' . $_[0] && -s _;
	local (*FH, $?);
	d 'open', $DIR.'/'.$_[0], "$!" unless open FH, '<', $DIR.'/'.$_[0];
	$/ = "---------- END ----------\n";
	my (@messages) = (<FH>);
	close FH;
	return @messages;
}


if ($date) {
	# Sinble message
	@messages = read_messages $date;
	unless (@messages) {
		print "No ChangLog entries for $print_date.\n";
		exit 0;
	}


} else {
	# Read New
	my $ndate;
	if ($new) {
		if (stat $HOME . '/.changelog') {
			$new = (stat _)[9];
			my ($mday, $mon);
			(undef, undef, undef, $mday, $mon, $ndate) = localtime $new;
			$ndate = sprintf '%02d%02d%02d', $ndate % 100, $mon + 1, $mday;
		} else {
			$ndate = 0;
		}
	}


	# Read maching file names
	d 'opendir', $DIR, "$!" unless opendir DIR, $DIR;
	my @days = grep {
		m/^[01][0-9][01][0-9][0-3][0-9]$/ && (!$new || $_ >= $ndate);
	} readdir DIR;
	closedir DIR;

	# No entries
	unless (@days) {
		print "No new ChangeLog entries\n";
		exit 0;
	}


	# Read messages
	@days = sort { $b cmp $a } @days;
	if ($num) {
		@messages = ( read_messages(shift @days), @messages )
			while @days && @messages < $num;
	} else {
		@messages = ( read_messages($_), @messages ) for (@days);
	}
}

# Mark new
if (($new || !$date) && $HOME && (open FH, '>', $HOME . '/.changelog')) {
	print FH time();
	close FH;
}



##
## Wrtie entries
##

# No entries
my $printed = 0;
unless (@messages) {
	print "No new ChangeLog entries\n";
	exit 0;
}

# Print all
$num = $num ? @messages - $num : 0;
for ($num = $num < 0 ? 0 : $num; $num < @messages; ++$num) {
	$_ = $messages[$num];
	my ($time, $user);

	# Parse headers
	if (s/^Time:\s*(\d+)\s*\n//m) {
		$time = $1;
	}
	if (s/^User:\s*(.*\S)\s*\n//m) {
		$user = $1;
	}
	s/\n---+\s*END\s*---+$//m;

	# Not new?
	next if defined $time && $new && $time < $new;

	# Print date
	if ($time) {
		my (undef, undef, undef, $mday, $mon, $year) = localtime $time;
		$time = sprintf '%02d%02d%02d', $year % 100, $mon + 1, $mday;
		if (!defined($date) || $time != $date) {
			print "\n" if $printed;
			printf "------- 20%02d/%02d/%02d ", $year % 100, $mon+1, $mday;
			for (my $i = 19; $i<=74; ++$i) { print '-'; }
			print "\n";
			$date = $time;
		}
	}

	# Print user name
	print "\n";
	if ($user && length $user<8) {
		printf '%-7s ', $user;
	} else {
		print $user, "\n" if $user;
		print '        ';
	}

	# Print entry
	s/\s+$//mg;
	s/\n{2,}/\n /mg;
	$Text::Wrap::columns = 64;
	$_ = fill '', '', $_;
	s/\n(.)/\n        $1/g;
	print $_, "\n";
	$printed = 1;
}


print "No new ChangeLog entries\n" unless $printed;


__END__

=head1 NAME

changelog.pl - Displays and adds change log entries

=head1 SYNOPSIS

changelog.pl -h | --help | --man

getlyrics.pl [ I<day> | new ] [ I<number> ]

changelog.pl -a [ I<message> ]

=head1 DESCRIPTION

The general idea of changelog.pl is to provide a global change log for
given machine or network.  In particular, system administrator (or
anyone else who has sufficient privileges) adds entries using
B<changelog -a> command which than can be displayed in B</etc/profile>
so that all users get the news.

=head1 OPTIONS

=over 8

=item B<-h> B<--help>

Displays usage information.

=item B<--man>

Displays a whole command's man page.

=item B<-a> B<--add>

If this is the only argument program will run your editor (it will
first try VISUAL environmental variable, then EDITOR and if both are
empty will run B<vi>) to let you enter message; otherwise the rest of
the arguments are treated as a message.

=item I<day>

Specifies the day to display entries for.  It can be either
B<yesterday>, B<today> or date in the format B<YYYYMMDD>.

=item B<new>

Tells changelog to display only the new entries since the last run.

=item I<number>

Application will display at most I<number> entries.

=back

=head1 EXAMPLES

=over 8

=item B<changelog.pl -a A new version of foo installed>

Adds an entry saying new version of foo has been installed.

=item B<changelog.pl>

Displays last 5 entries.

=item B<changelog.pl 20060101>

Displays all entries for 2006/01/01.

=item B<changelog.pl new>

Displays all new entries.

=item B<changelog.pl 20060101 5>

Displays last 5 entries for 2006/01/01.

=back

=head1 AUTHOR

Michal Nazarewicz <mina86@mina86.com>

=cut
