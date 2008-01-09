/*
 * Allocates specified amount of memory.
 * $Id: malloc.c,v 1.11 2008/01/09 18:50:58 mina86 Exp $
 * Copyright (c) 2005,2007 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
 */

/*
 * This may be used to try to free some memory, eg. if there are many
 * unfreed buffers or something.  It may help if your PC is running
 * slow however it is not certain and many would disagree.
 */



/********** Config **********/

/* Comment the next line out if your platform doesn't have sbrk()
   function */
#define HAVE_SBRK


#if __STDC_VERSION__ < 199901L
#  if defined __GNUC__
#    define inline   __inline__
#  else
#    define inline
#  endif
#endif



/********** Includes **********/
#define _BSD_SOURCE
#define _SVID_SOURCE
#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#ifdef HAVE_SBRK
# include <errno.h>
# include <unistd.h>
#endif

/********** Variables **********/
static volatile sig_atomic_t signum = 0;


/********** Function declarations **********/
static long parse_arg(char *arg);
static void usage(void);
static void signal_handler(int sig);
static inline int  alloc(int num);
static void progress(long allocated, long to_allocate);



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

	/* Catch signals */
	for (dot = 0; dot<32; signal(++dot, &signal_handler));

	/* Allocate */
	dot = to_allocate>2048 ? to_allocate>>11 : 1;
	setvbuf(stdout, 0, _IONBF, 0);              /* unbuffered */

	while (!signum && allocated<to_allocate && alloc(dot)) {
		progress(allocated += dot, to_allocate);
	}

	/* Return */
	progress(allocated, to_allocate);
	putchar('\n');
	return signum ? -signum : (allocated < to_allocate ? 1 : 0);
}



/********** Parses arg **********/
static long parse_arg(char *arg) {
	double ret = strtod(arg, &arg);
	switch (*arg) {
	case 'K':               ++arg; break;
	case 'M': ret *= 1<<10; ++arg; break;
	case 'G': ret *= 1<<20; ++arg; break;
	}
	return *arg ? -1 : ret;
}



/********** Usage **********/
static void usage(void) {
	puts("usage: malloc <bytes>\n"
		 " <bytes>  number of bytes to allocates;\n"
		 "          can be fallowed by K (the default), M or G\n");
}




/********** Signal handler **********/
static void signal_handler(int sig) {
	signum = sig;
	signal(sig, signal_handler);
}



/********** Allocates memory **********/
static inline int alloc(int num) {
	char *ptr;
#ifdef HAVE_SBRK
	errno = 0;
	ptr = sbrk(num <<= 10);
	if (errno) return 0;
#else
	ptr = malloc(num <<= 10);
	if (!ptr) return 0;
#endif
	do {
		*ptr++ = --num;
	} while (!signum && num);
	return !signum;
}



/********** Prints progress **********/
static void progress(long allocated, long to_allocate) {
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

	fflush(stdout);
}
