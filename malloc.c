/*
 * Allocates specified amount of memory.
 * Copyright (c) 2005,2007 by Michal Nazareicz <mina86@mina86.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * This is part of Tiny Applications Collection
 *   -> http://tinyapps.sourceforge.net/
 */

/*
 * This may be used to try to free some memory allocated for I/O
 * buffers (which could lead to some flushes as well), as well as it
 * can be used for testing the system performance when lots of memory
 * is consumed.
 */


/********** Config **********/

/* Comment the next line out if your platform doesn't have sbrk()
   function */
#define HAVE_SBRK


/********** Includes **********/
#define _BSD_SOURCE
#define _SVID_SOURCE
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#ifdef HAVE_SBRK
# include <errno.h>
# include <unistd.h>
#endif


/********** Variables **********/
static volatile sig_atomic_t signum = 0;


/********** Function declarations **********/
static unsigned long parse_arg(char *arg);
static void usage(void);
static void handle_all(void (*handler)(int signum));
static void signal_handler(int sig);
static int  alloc(unsigned long num);
static void progress(unsigned long allocated, unsigned long to_allocate);

static unsigned long min(unsigned long x, unsigned long y) {
	return x < y ? x : y;
}


/********** Main **********/
int main(int argc, char **argv) {
	unsigned long to_allocate, allocated = 0, dot = 1024;

	switch (argc) {
	case 3:
		if (!strcmp(argv[1], "-w")) {
			++argv;
			/* FALL THROUGH */
	case 2:
			to_allocate = parse_arg(argv[1]);
			if (to_allocate) {
				break;
			}
		}
	default:
		usage();
		return 2;
	}

	handle_all(signal_handler);

	if (to_allocate < dot) {
		dot = 4;
	}

	progress(0, to_allocate);
	while (!signum &&
	       allocated < to_allocate &&
	       alloc(min(dot, to_allocate - allocated))) {
		allocated += min(dot, to_allocate - allocated);
		progress(allocated, to_allocate);
	}
	putchar('\n');

	handle_all(SIG_DFL);

	if (argc == 3) {
		char buffer[1024];
		while (fgets(buffer, sizeof buffer, stdin) && !strchr(buffer, '\n'))
			/* nop */;
	}

	return signum ? -signum : (allocated < to_allocate ? 1 : 0);
}


/********** Command line arguments **********/
static unsigned long parse_arg(char *arg) {
	double ret = strtod(arg, &arg);
	switch (*arg) {
	case 'K':               ++arg; break;
	case 'M': ret *= 1<<10; ++arg; break;
	case 'G': ret *= 1<<20; ++arg; break;
	}
	return *arg ? 0 : ret;
}

static void usage(void) {
	puts("usage: malloc [ -w ] <bytes>\n"
	     " -w       wait for new line after allocating\n"
	     " <bytes>  number of bytes to allocates;\n"
	     "          can be fallowed by K (the default), M or G\n");
}


/********** Signal handler **********/
static void signal_handler(int sig) {
	signum = sig;
	signal(sig, signal_handler);
}

static void handle_all(void (*handler)(int signum)) {
	int i = 31;
	do {
		switch (i) {
		case SIGCONT: /* Continue if stopped */
		case SIGSTOP: /* Stop process */
		case SIGTSTP: /* Stop typed at tty */
		case SIGTTIN: /* tty input for background process */
		case SIGTTOU: /* tty output for background process */
			break;

		default:
			signal(i, handler);
		}
	} while (--i);
}


/********** Allocates memory **********/
static int alloc(unsigned long num) {
	char *ptr;
#ifdef HAVE_SBRK
	errno = 0;
	ptr = sbrk(num <<= 10);
	if (errno) {
		return 0;
	}
#else
	ptr = malloc(num <<= 10);
	if (!ptr) {
		return 0;
	}
#endif
	do {
		*ptr++ = --num;
	} while (!signum && num);
	return !signum;
}


/********** Prints progress **********/
static void progress(unsigned long allocated, unsigned long to_allocate) {
	/* Zi an Yi are unofficial; Yi is 2^80 so there is no way we will
	   ever allocat that amout of memoty ;) therefore we don't need to
	   check whether there are some units available */
	static const char *const units = "KMGTPEZY";

	static char dots[] = "=============================="
		"==============================>";

	static unsigned long old_size = ~0UL, old = ~0UL;
	static unsigned old_unit = ~0U;

	unsigned long size = allocated;
	unsigned i;

	/* Format size in a friendly way */
	for (i = 0; size > 102400; size >>= 10) {
		++i;
	}
	if (old_size == size && old_unit == i) {
		return;
	}

	old_size = size;
	old_unit = i;

	if (size < 1024) {
		printf("\r%5lu %ciB", size, units[i]);
	} else {
		printf("\r%3lu.%lu %ciB", size / 1024, size * 10 / 1024 % 10,
			   units[i + 1]);
	}

	/* Percentage */
	printf(" (%3lu%%)", allocated * 100 / to_allocate);

	/* Slider */
	if ((allocated * 100 / to_allocate) != old * 100 / to_allocate) {
		old = allocated;

		i = allocated * 50 / to_allocate;
		dots[i] = '>';
		printf("  [%-50.*s]", i + (allocated != to_allocate), dots);
		dots[i] = '=';
	}

	putchar('\r');
	fflush(stdout);
}
