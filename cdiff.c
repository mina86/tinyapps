/*
 * Colorize (adds ANSI codes) output of diff.
 * $Id: cdiff.c,v 1.2 2005/07/11 00:20:58 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/tlen.pl)
 * Licensed under the Academic Free License version 2.1.
 */

#include <stdio.h>

#define BUF_SIZE 1024
#define false 0
#define true 1

int end_with_eol(const char *str) {
	int last_eol = false;
	while (*str) {
		last_eol = *str=='\n';
		++str;
	}
	return last_eol;
}

int main(int argc, char **argv) {
	char buf[BUF_SIZE];
	const char *colors[4];

	colors[0] = "1;34";
	colors[1] = "1;32";
	colors[2] = "1;31";
	colors[3] = "0;37";
	register int i = 0;
	for (; i<argc-1 && i<4; ++i) {
		colors[i] = argv[i+1];
	}

	while (fgets(buf, BUF_SIZE, stdin)) {
		switch (buf[0]) {
		case '@': printf("\e[%sm", colors[0]); break;
		case '+': printf("\e[%sm", colors[1]); break;
		case '-': printf("\e[%sm", colors[2]); break;
		default : printf("\e[%sm", colors[3]); break;
		}

		printf("%s", buf);
		while (!end_with_eol(buf)) {
			fgets(buf, BUF_SIZE, stdin);
			printf("%s", buf);
		}
	}
	puts("\e[0n");

	return 0;
}
