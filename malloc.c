/*
 * Allocates specified amount of memory
 * $Id: malloc.c,v 1.1 2005/06/30 10:04:10 mina86 Exp $
 * Copyright (C) 2005 by Michal Nazareicz (mn86/AT/o2.pl)
 * Licensed under the Academic Free License version 2.1.
 */

/*
 * This may be used to try to free some memory, eg. if there are many
 * unfreed buffers or something.  It may help if your PC is running
 * slow however it is not certain and many would disagree.
 */

#include <stdlib.h>
#include <stdio.h>


/**** Prints size in friendly way ****/
void print_size(int size) {
	static const char *const units = "KMGTPEZY";
	/* Zi an Yi are unofficial; Yi is 2^80 so there is no way we will
	   ever allocat that amout of memoty ;) therefore we don't need to
	   check whether there are some units available */

	int i = 0;
	for (;; ++i, size >>= 10) {
		if (size<1024) {
			printf(" %4d %ciB\n", size, units[i]);
			return;
		} else if (size<102400) {
			printf(" %4.1f %ciB\n", size/1024.0, units[i+1]);
			return;
		}
	}
}


/**** Main ****/
int main(int argc, char **argv) {
	char *ptr;
	long to_allocate, allocated = 0;
	int d = 0, dot;

	/* Parse arguments */
	to_allocate =  argc!=2 || !*argv[1] ? -1 : strtol(argv[1], &ptr, 0);
	if (to_allocate<0 || *ptr) {
		puts("usage: malloc <number>");
		puts("where <number> is number of KiBs to allocate");
		return 1;
	}

	/* Allocate */
	dot = to_allocate>2048 ? to_allocate>>11 : 1;
	setvbuf(stdout, NULL, _IONBF, 0);

	while (allocated<to_allocate && calloc(dot, 1024)!=NULL) {
		putchar('.');
		allocated += dot;
		if ((d = (d+1) & 63)==0) print_size(allocated);
	}

	/* We didn't menage :( */
	if (allocated < to_allocate) {
		putchar('!');
		return 1;
	}

	/* Return */
	if (d) {
		print_size(allocated);
	}
	return 0;
}
