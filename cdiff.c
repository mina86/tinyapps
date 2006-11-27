/*
 * Colorize (adds ANSI codes) output of diff.
 * $Id: cdiff.c,v 1.4 2006/11/27 18:58:38 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
 */

#include <stdio.h>
#include <string.h>


int main(int argc, char **argv) {
	char buf[1024];
	const char *colors[4] = {
		"1;34",
		"1;32",
		"1;31",
		"0;37",
	};

	const char **it = colors;
	while (--argc) {
		*it++ = *++argv;
	}

	while (fgets(buf, sizeof buf, stdin)) {
		switch (buf[0]) {
		case '@': printf("\x1b[%sm", colors[0]); break;
		case '+': printf("\x1b[%sm", colors[1]); break;
		case '-': printf("\x1b[%sm", colors[2]); break;
		default : printf("\x1b[%sm", colors[3]); break;
		}

		printf("%s", buf);
		while (!strchr(buf, '\n') && fgets(buf, sizeof buf, stdin)) {
			printf("%s", buf);
		}
	}
	puts("\x1b[0n");

	return 0;
}
