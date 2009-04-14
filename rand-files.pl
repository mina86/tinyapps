#!/usr/bin/perl
##
## Chooses a random set of files from directories
## Copyright (c) 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
##
## This software is OSI Certified Open Source Software.
## OSI Certified is a certification mark of the Open Source Initiative.
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

use Getopt::Long;
use Pod::Usage;


my ($left, $out_sep) = ( 1000, "\n" );
my (@rules, @exts);


# Adds single rule
sub add_rule ($$) {
	my ($result, $regexp) = @_;
	if ($result eq 'exclude' || $result eq 'x') {
		$result = 0;
	} elsif ($result eq 'include' || $result eq 'i') {
		$result = 1;
	}

	my $match = 1;
	if ($regexp =~ /!(.*)$/) {
		$match = 0;
		$regexp = $1;
	}

	eval {
		$SIG{'__WARN__'} = sub { die $_[0]; };
		$result += 0;
	};
	die "$result: invalid rank (must be a number)\n" if $@;

	eval {
		$regexp = qr/$regexp/;
	} or die "$regexp: invalid regexp\n";

	push @rules, [ $result, $match, $regexp ];
}


# Ranks a file
sub rank_file ($) {
	for my $r (@rules) {
		return $r->[0] if ($_[0] =~ $r->[2]) == $r->[1];
	}
	1;
}


# Parse options
Getopt::Long::Configure(qw/bundling/);
exit 1 unless GetOptions(
	'zero|0'        => sub { $/ = $out_sep = "\0"; },
	'zero-out|z'    => sub { $out_sep = "\0"; },
	'zero-in|Z'     => sub { $/ = "\0"; },

	'size|s=i'      => \$left,

	'extension|e=s' => \@exts,
	'exclude|x=s'   => sub { add_rule 0, $_[1]; },
	'include|i=s'   => sub { add_rule 0, $_[1]; },
	'rule|r=s'      => sub {
		my @arr = split /,/, $_[1], 2;
		add_rule $arr[0], $arr[1];
	},

	'help|h'        => sub { pod2usage(-exitstatus => 0, -verbose => 1); },
	'man'           => sub { pod2usage(-exitstatus => 0, -verbose => 2); }
);


# Interprete arguments
@exts = ( 'ogg', 'mp3' ) unless @exts || @rules;
if (@exts) {
	@exts = split(/,/, join(',', @exts));
	my $regexp = '';
	$regexp .= "|\Q$_\E" for (@exts);
	$regexp = substr $regexp, 1;
	unshift @rules, [ 0, 0, qr/(?:$regexp)$/ ];
}
undef @exts;
$left <<= 8;


# Find source files
my ($ranks_sum, @files) = ( 0 );
while (<>) {
	chomp;
	my @stat = stat $_;
	$stat[7] = ($stat[7] + 4095) >> 12;
	if ($stat[7] > 0  && $stat[7] <= $left && -f _) {
		my $rank = rank_file $_;
		if (defined $rank && $rank > 0) {
			push @files, [ $_, $stat[7], $rank ];
			$ranks_sum += $rank;
		}
	}
}
@files = sort { $a->[1] <=> $b->[1] } @files;

# Choose pool
my @pool;

while (@files) {
	# Choose random index
	my $idx = 0;
	for ($_ = rand $ranks_sum; $idx < @files && $_ >= $files[$idx][2]; ++$idx) {
		$_ -=  $files[$idx][2];
	}

	# Add it to the pool
	push @pool, $files[$idx][0];
	$left -= $files[$idx][1];

	# Remove too big files
	$_ = @files;
	while ($_ && $files[$_ - 1][1] > $left) {
		$ranks_sum -= $files[--$_][2];
	}
	last unless $_;
	$#files = $_ - 1;

	# Remove $idx if it's still in the array
	if ($idx < @files) {
		$ranks_sum -= $files[$idx][2];
		splice @files, $idx, 1;
	}
}


# Exit
print join $out_sep, @pool, '';
exit !@pool;


__END__
=head1 NAME

rand-mp3.pl  - Chooses random files from standard input

=head1 SYNOPSIS

rsz.pl [ I<options> ]

=head1 DESCRIPTION

Script reads file names from standard input (for example output of
find command) and choses random set of them such that total sum of
their size is no bigger then given number of megabytes and prints them
to standard output.  The size of a files is rounded up to full 4KiB.

Throught set of options you can rank files.  The higher the rank the
bigger the probability that it will be chosen to be part of the output
set.

It's main use is probably copying a random set of music files to
a portable MP3 player which has storage too small to hold all our
music collection.  For that purpose you can use GNU find, GNU xargs
and mv:

  find /path/to/music -type f -print0 | \
    ./rand-files.pl -0 | \
    xargs -0 --no-run-if-empty mv -t /media/pen --

=head1 OPTIONS

Long options may be abbreviated as long as they are unique.

=over 8

=item B<--help>

Prints a brief help message and exits.

=item B<--man>

Prints long help message and exits.

=item B<-0 --zero>

Synonym of B<-z -Z> which see.

=item B<-z --zero-out>

When printing out file list end each file with a NUL byte instead of
new line character.  This way output may be sent to GNU xargs run with
B<-0> option.

=item B<-Z --zero-in>

When reading files from standard input assume file names are ended
with a NUL byte and not new line character.  This way the script may
read output of a GNU find command with B<-print0> action.

=item B<-s --size>=I<size>

Size in MiB not to exceed.  By default it is 1000.

=item B<-e --extension>=I<ext>

If a file has I<ext> (that is ends with a dot followed by I<ext>) it
will be added to a set of files to choose from.  By default, if no
B<-e>, B<-x>, B<-i> or B<-r> option is given program searches for
B<mp3> and B<ogg> files.

File extension is checked prior to any other rules.  If file's
extension does not match on of the extensions given on command line it
won't be taken into consideration.

=item B<-x --exclude>=I<regexp>

The same as B<--rule=0,>I<regexp> which see.

=item B<-i --include>=I<regexp>

The same as B<--rule=1,>I<regexp> which see.

=item B<-r --rule>=I<rank>,I<regexp>

Adds a rule for ranking files.  If file name matches I<regexp> it is
assigned given I<rank> and no other rules will be taken into
consideration (ie. first rule that matches wins).

I<rank> may be any real number.  If it is less then or equal zero the
image won't be added to the set of files to choose from

=back
