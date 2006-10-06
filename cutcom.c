/*
 * Removes C++ comments from a file.
 * $Id: cutcom.c,v 1.4 2006/10/06 17:28:04 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
 */

#include <stdio.h>


enum state {
	S_TEXT,
	S_STRING,
	S_STRING_BS,
	S_CHAR,
	S_CHAR_BS,
	S_SLASH,
	S_LINE_COMM,
	S_BLCK_COMM,
	S_STAR
};


struct state_function {
	unsigned char ch, new_state, print, pad;
	const char *pre_print;
};


static const struct state_function s_text_functions[] = {
		{ '/' , S_SLASH    , 0, 0, 0   },
		{ '"' , S_STRING   , 1, 0, 0   },
		{ '\'', S_CHAR     , 1, 0, 0   },
		{ 0   , S_TEXT     , 1, 0, 0   },
};

static const struct state_function s_string_functions[] = {
		{ '"' , S_TEXT     , 1, 0, 0   },
		{ '\\', S_STRING_BS, 1, 0, 0   },
		{ 0   , S_STRING   , 1, 0, 0   },
};

static const struct state_function s_string_bs_functions[] = {
		{ 0   , S_STRING   , 1, 0, 0   },
	};

static const struct state_function s_char_functions[] = {
		{ '"' , S_TEXT     , 1, 0, 0   },
		{ '\\', S_CHAR_BS  , 1, 0, 0   },
		{ 0   , S_STRING   , 1, 0, 0   },
};

static const struct state_function s_char_bs_functions[] = {
		{ 0   , S_CHAR     , 1, 0, 0   },
};

static const struct state_function s_slash_functions[] = {
		{ '/' , S_LINE_COMM, 0, 0, 0   },
		{ '*' , S_BLCK_COMM, 0, 0, 0   },
		{ 0   , S_TEXT     , 1, 0, "/" },
};

static const struct state_function s_line_comm_functions[] = {
		{ '\n', S_TEXT     , 1, 0, 0   },
		{ 0   , S_LINE_COMM, 0, 0, 0   },
};

static const struct state_function s_block_comm_functions[] = {
		{ '*' , S_STAR     , 0, 0, 0   },
		{ 0   , S_BLCK_COMM, 0, 0, 0   },
};

static const struct state_function s_star_functions[] = {
		{ '/' , S_TEXT     , 0, 0, 0   },
		{ 0   , S_BLCK_COMM, 0, 0, 0   },
};


static const struct state_function *const states[] = {
	s_text_functions,
	s_string_functions,
	s_string_bs_functions,
	s_char_functions,
	s_char_bs_functions,
	s_slash_functions,
	s_line_comm_functions,
	s_block_comm_functions,
	s_star_functions,
};


int main(void) {
	int ch, state = 0;

	while ((ch = getchar())!=EOF) {
		const struct state_function *func = states[state];
		while (func->ch && func->ch!=ch) ++func;
		if (func->pre_print) {
			fputs(func->pre_print, stdout);
		}
		if (func->print) {
			putchar(ch);
		}
		state = func->new_state;
	}

	return 0;
}
