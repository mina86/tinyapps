#!/usr/bin/perl -wWtTn
if (/[aeiouyAEIOUY][^a-zA-Z]*$/) {
	print("No!\n");
} elsif (!/^\s*$/) {
	print("Yes.\n");
}
