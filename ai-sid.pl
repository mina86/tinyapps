#!/usr/bin/perl -wWtT
while (<>) {
	if (/[aeiouyAEIOUY][^a-zA-Z]*$/) {
		print("Yes.\n");
	} elsif (!/^\s*$/) {
		print("No.\n");
	}
}
