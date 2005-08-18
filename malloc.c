/*
 * Allocates specified amount of memory.
 * $Id: malloc.c,v 1.5 2005/08/18 01:09:14 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/tlen.pl)
 * Licensed under the Academic Free License version 2.1.
 */

/*
 * This may be used to try to free some memory, eg. if there are many
 * unfreed buffers or something.  It may help if your PC is running
 * slow however it is not certain and many would disagree.
 */

/* Comment the next line out if you don't compile for platform with
   signal.h file. */
#define HAVE_SIGNAL_H


#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


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


#ifdef HAVE_SIGNAL_H
/**** Signal handler ****/
int signum = 0;
void signal_handler(int sig) {
	signum = sig;
}
#endif


/**** Main ****/
int main(int argc, char **argv) {
	long to_allocate, allocated = 0;
	int d = 0, dot;

	/* Parse arguments */
	to_allocate =  argc!=2 || !*argv[1] ? -1 : strtol(argv[1], argv + 1, 0);
	if (to_allocate<0 || *argv[1]) {
		puts("usage: malloc <number>");
		puts("where <number> is number of KiBs to allocate");
		return 2;
	}

#ifdef HAVE_SIGNAL_H
	/* Catch signals */
	for (dot = 0; dot<32; signal(++dot, &signal_handler));
#endif

	/* Allocate */
	dot = to_allocate>2048 ? to_allocate>>11 : 1;
	setvbuf(stdout, NULL, _IONBF, 0);

#ifdef HAVE_SIGNAL_H
	while (!signum && allocated<to_allocate && calloc(dot, 1024)!=NULL) {
#else
	while (allocated<to_allocate && calloc(dot, 1024)!=NULL) {
#endif
		putchar('.');
		allocated += dot;
		if ((d = (d+1) & 63)==0) {
			print_size(allocated);
		}
	}

	/* Return */
	if (allocated < to_allocate) {
		putchar('!');
		d = 1;
	}
	if (d) {
		print_size(allocated);
	}
#ifdef HAVE_SIGNAL_H
	return signum ? -signum : (allocated < to_allocate ? 1 : 0);
#else
	return allocated < to_allocate ? 1 : 0;
#endif
}
