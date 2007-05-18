/*
 * Allocates specified amount of memory.
 * $Id: malloc.c,v 1.9 2007/05/18 21:22:24 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
 */

/*
 * This may be used to try to free some memory, eg. if there are many
 * unfreed buffers or something.  It may help if your PC is running
 * slow however it is not certain and many would disagree.
 */



/********** Config **********/

/* Comment the next line out if you don't compile for platform with
   signal.h file. */
#define HAVE_SIGNAL_H



/********** Includes **********/
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


/********** Variables **********/
#ifdef HAVE_SIGNAL_H
static volatile int signum = 0;
#else
#define signum 0
#endif


/********** Function declarations **********/
long parse_arg(char *arg);
void usage();
#ifdef HAVE_SIGNAL_H
void signal_handler(int sig);
#endif
int  alloc(int num);
void progress(long allocated, long to_allocate);



/********** Main **********/
int main(int argc, char **argv) {
	long to_allocate, allocated = 0;
	int dot;

	/* Invalid number of arguments */
	if (argc!=2) {
		usage();
		return 2;
	}

	/* Parse arguments */
	to_allocate = parse_arg(argv[1]);
	if (to_allocate<0) {
		usage();
		return 2;
	}

#ifdef HAVE_SIGNAL_H
	/* Catch signals */
	for (dot = 0; dot<32; signal(++dot, &signal_handler));
#endif

	/* Allocate */
	dot = to_allocate>2048 ? to_allocate>>11 : 1;
	setvbuf(stdout, NULL, _IONBF, 0);              /* unbuffered */

	while (!signum && allocated<to_allocate && alloc(dot)) {
		progress(allocated += dot, to_allocate);
	}

	/* Return */
	progress(allocated, to_allocate);
	putchar('\n');
	return signum ? -signum : (allocated < to_allocate ? 1 : 0);
}



/********** Parses arg **********/
long parse_arg(char *arg) {
	long ret = strtol(arg, &arg, 0);
	switch (*arg) {
	case 'K':            ++arg; break;
	case 'M': ret <<=10; ++arg; break;
	case 'G': ret <<=20; ++arg; break;
	}
	return *arg ? -1 : ret;
}



/********** Usage **********/
void usage() {
	puts("usage: malloc <bytes>\n"
		 " <bytes>  number of bytes to allocates;\n"
		 "          can be fallowed by K (the default), M or G\n");
}




#ifdef HAVE_SIGNAL_H
/********** Signal handler **********/
void signal_handler(int sig) {
	signum = sig;
}
#endif



/********** Allocates memory **********/
int alloc(int num) {
	char *ptr = malloc(num <<= 10);
	if (ptr==NULL) return 0;
	do {
		*ptr++ = --num;
	} while (!signum && num);
	return !signum;
}



/********** Prints progress **********/
void progress(long allocated, long to_allocate) {
	/* Zi an Yi are unofficial; Yi is 2^80 so there is no way we will
	   ever allocat that amout of memoty ;) therefore we don't need to
	   check whether there are some units available */
	static const char *const units = "KMGTPEZY";

	static char dots[] = "=============================="
		"==============================";

	long size = allocated;
	int i;

	/* Print dots */
	i = allocated * 50 / to_allocate;
	if (i==0) {
		i = 1;
	}
	if (allocated!=to_allocate) {
		dots[i-1] = '>';
	}
	dots[i] = 0;
	printf("\r   [%-50s]  ", dots);
	dots[i-1] = dots[i] = '=';

	/* Format size in a friendly way */
	for (i = 0; size > 102400; size >>= 10) {
		++i;
	}
	if (size<1024) {
		printf("%5ld %ciB", size, units[i]);
	} else {
		printf("%5.1f %ciB", size/1024.0, units[i+1]);
	}

	/* Percentage */
	printf(" (%5.1f%%)\r", allocated*100.0/to_allocate);
}
