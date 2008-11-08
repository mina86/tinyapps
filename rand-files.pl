#!/usr/bin/perl
##
## Chooses a random set of files from directories
## $Id: rand-files.pl,v 1.2 2008/11/08 23:45:57 mina86 Exp $
## Copyright (c) 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
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

use File::Find;
use Getopt::Long;
use Pod::Usage;


my ($left, $sep, $regexp, @match, @exts, @dirs) = ( 1000, "\n" );


# Parse options
Getopt::Long::Configure(qw/bundling/);
exit 1 unless GetOptions(
	'zero|0'        => sub { $sep = "\0"; },
	'size|s=i'      => \$left,
	'match|m=s'     => sub {
		eval { qr/(?:${_[1]})/; } or die "$_[1]: invalid regexp\n";
		push @match, '(?:' . $_[1] . ')';
	},
	'extension|e=s' => \@exts,
	'<>'            => sub {
		push @dirs, $_[0];
	},

	'help|h' => sub { pod2usage(-exitstatus => 0, -verbose => 1); },
	'man'  => sub { pod2usage(-exitstatus => 0, -verbose => 2); }
);


# Validate command line arguments
if (!@dirs) {
	pod2usage(-exitstatus => 1, -verbose => 0);
}


# Interprete command line arguments
if (@exts) {
	$regexp = '';
	for (@exts) {
		$regexp .= "|\Q$_\E"
	}
	push @match, '(?:' . substr($regexp, 1) . ')$';
} elsif (!@match) {
	push @match, '(?:ogg|mp3)';
}

$regexp = join '|', @match;
undef @match;
undef @exts;
$left <<= 8;


# Find source files
my @files;
File::Find::find {
	'follow_fast' => 1,
	'dangling_symlinks' => 0,
	'no_chdir' => 1,
	'wanted' => sub {
		return unless -f _ && m/$regexp/o;
		my @s = stat _;
		my $s = $s[7];
		$s += 4096 if $s & 4095;
		$s >>= 12;
		if ($s <= $left) {
			push @files, [ $File::Find::name, $s ];
		}
	},
}, @dirs;
undef @dirs;
@files = sort { $a->[1] <=> $b->[1] } @files;


# Choose pool
my @pool;

while (@files) {
	my $idx = int rand @files;

	push @pool, $files[$idx][0];
	$left -= $files[$idx][1];

	$_ = @files;
	while ($_ && $files[$_ - 1][1] > $left) {
		--$_;
	}
	last unless $_;

	$#files = $_ - 1;
	if ($idx < @files) {
		splice @files, $idx, 1;
	}
}


# Exit
print join $sep, @pool, '';
exit !@pool;


__END__
=head1 NAME

rand-mp3.pl  - Chooses random files from directiry trees

=head1 SYNOPSIS

rsz.pl [ I<options> ] I<source-dir> [ I<source-dir> ... ]

=head1 DESCRIPTION

Script searches for files in given source directories (recursivly),
choses random set of them such taht total sum of their size is no
bigger then given number of megabytes and prints them to standard
output.  The size of a files is rounded up to full 4KiB.

It's main use is probably copying a random set of music files to
a portable MP3 player which has storage too small to hold all our
music collection.  For that purpose you can use GNU xargs and mv:

  ./rand-mp3.pl -0 | xargs -0 --no-run-if-empty mv -t/media/pen --

=head1 OPTIONS

Long options may be abbreviated as long as they are unique.

=over 8

=item B<--help>

Prints a brief help message and exits.

=item B<--man>

Prints long help message and exits.

=item B<-0 --zero>

Ends each printed file name with a NUL byte instead of new line
character.

=item B<-s --size>=I<size>

Size in MiB not to exceed.  By default it is 1000.

=item B<-m --match>=I<regexp>

If a file matches this I<regexp> it will be added to a set of files to
choose from.

=item B<-e --extension>=I<ext>

If a file has I<ext> (that is ends with a dot followed by I<ext>) it
will be added to a set of files to choose from.  By default, if no
B<-e> nor B<-m> option is given program searches for B<mp3> and B<ogg>
files.

=item I<source-dir>

Start of source directory tree to search for files.

=item I<dest-dir>

A destination directory to copy files to.  B<WARNING!> Files in that
directory will be deleted!

=back
