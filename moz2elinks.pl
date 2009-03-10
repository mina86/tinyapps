#!/usr/bin/perl -wnl
##
## Converts Mozilla's to Elinks' bookmarks.
## Copyright (c) 2005 by Stanislaw Klekot (dozzie/AT/irc.pl)
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

# Script converts bookmarks from Mozilla's format to format accepted by ELinks
# (http://elinks.or.cz/)
#
# Usage: $0 path/to/mozilla/bookmarks.html > ~/.elinks/bookmarks

BEGIN {
  our @bookmarks = ();
  our $depth = -1;
}

# Mozilla's bookmarks are written in UTF, but I use ISO-8859-2 encoding
use encoding "utf8", STDIN => "latin2";

next unless m{</?D[LT]>};
++$depth, next if /<DL>/;
--$depth, next if m{</DL>};

# /<DT>/
my @x = split /[<>]+/;
$x[3] =~ s/&amp;/&/g;
$x[3] =~ s/&lt;/</g;
$x[3] =~ s/&gt;/>/g;

if ($x[2] =~ /^H3/) {
  push @bookmarks, "$x[3]\t\t$depth\tF"
} else {
  $x[2] =~ /HREF="([^"]*)"/;
  push @bookmarks, "$x[3]\t$1\t$depth\t"
}

END {
  print foreach @bookmarks;
}
