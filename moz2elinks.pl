#!/usr/bin/perl -wnl
##
## Converts Mozilla's to Elinks' bookmarks.
## $Id: moz2elinks.pl,v 1.2 2005/07/10 18:06:59 mina86 Exp $
## Copyright (c) 2005 by Stanislaw Klekot (dozzie/at/irc.pl)
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
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
