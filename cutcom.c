/*
 * Removes C++ comments from a file
 * $Id: cutcom.c,v 1.1 2005/07/02 11:51:12 mina86 Exp $
 * Copyright (C) 2005 by Michal Nazareicz (mn86/AT/o2.pl)
 * Licensed under the Academic Free License version 2.1.
 */

#include <stdio.h>

#define S_TEXT 0
#define S_STRING 1
#define S_SLASH 3
#define S_LINE_COMMENT 4
#define S_BLOCK_COMMENT 5
#define S_STAR 6

int main(void) {
	unsigned int state = S_TEXT, ch, skip_next = 0, str_char;
	while ((ch = getchar())!=EOF) {
		switch (state) {
		case S_TEXT:
			switch (ch) {
			case '/':
				state = S_SLASH;
				break;
			case '"':
			case '\'':
				state = S_STRING;
				str_char = ch;
			default:
				putchar(ch);
			}
			break;

		case S_STRING:
			putchar(ch);
			if (skip_next) {
				skip_next = 0;
			} else 	if (ch=='\\') {
				skip_next = 1;
			} else if (ch==str_char) {
				state = S_TEXT;
			}
			break;

		case S_SLASH:
			switch (ch) {
			case '/':
				state = S_LINE_COMMENT;
				break;
			case '*':
				state = S_BLOCK_COMMENT;
				break;
			default:
				putchar('/');
				putchar(ch);
				state = S_TEXT;
			}
			break;

		case S_LINE_COMMENT:
			if (ch=='\n') {
				state = S_TEXT;
			}
			break;

		case S_BLOCK_COMMENT:
			if (ch=='*') {
				state = S_STAR;
			}
			break;

		case S_STAR:
			state = ch=='/'?S_TEXT:S_BLOCK_COMMENT;
		}
	}
	return 0;
}
