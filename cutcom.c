/*
 * Removes C++ comments from a file.
 * Copyright (c) 2005-2008 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 3.0.
 *
 * This software is OSI Certified Open Source Software.
 * OSI Certified is a certification mark of the Open Source Initiative.
 *
 * This is part of Tiny Applications Collection
 *   -> http://tinyapps.sourceforge.net/
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>


struct state_function {
	const unsigned char ch, print;
	const char pre_print, pad;
	const struct state_function *const new_state;
};


static const struct state_function s_text[4];
static const struct state_function s_string[3];
static const struct state_function s_string_bs[1];
static const struct state_function s_char[3];
static const struct state_function s_char_bs[1];
static const struct state_function s_slash[3];
static const struct state_function s_line_comm[2];
static const struct state_function s_block_comm[2];
static const struct state_function s_star[2];


static const struct state_function s_text[] = {
	{ '/' , 0, 0  , 0, s_slash     },
	{ '"' , 1, 0  , 0, s_string    },
	{ '\'', 1, 0  , 0, s_char      },
	{ 0   , 1, 0  , 0, s_text      },
};

static const struct state_function s_string[] = {
	{ '"' , 1, 0  , 0, s_text      },
	{ '\\', 1, 0  , 0, s_string_bs },
	{ 0   , 1, 0  , 0, s_string    },
};

static const struct state_function s_string_bs[] = {
	{ 0   , 1, 0  , 0, s_string    },
};

static const struct state_function s_char[] = {
	{ '"' , 1, 0  , 0, s_text      },
	{ '\\', 1, 0  , 0, s_char_bs   },
	{ 0   , 1, 0  , 0, s_string    },
};

static const struct state_function s_char_bs[] = {
	{ 0   , 1, 0  , 0, s_char      },
};

static const struct state_function s_slash[] = {
	{ '/' , 0, 0  , 0, s_line_comm },
	{ '*' , 0, 0  , 0, s_block_comm },
	{ 0   , 1, '/', 0, s_text      },
};

static const struct state_function s_line_comm[] = {
	{ '\n', 1, 0  , 0, s_text      },
	{ 0   , 0, 0  , 0, s_line_comm },
};

static const struct state_function s_block_comm[] = {
	{ '*' , 0, 0  , 0, s_star      },
	{ 0   , 0, 0  , 0, s_block_comm },
};

static const struct state_function s_star[] = {
	{ '/' , 0, 0  , 0, s_text      },
	{ 0   , 0, 0  , 0, s_block_comm },
};


int main(int argc, char **argv) {
	int i;

	if (argc==1) {
		argv[1] = (char*)"-";
		argc = 2;
	}

	for (i = 1; i < argc; ++i) {
		const struct state_function *state = s_text;
		FILE *fp;
		int ch;

		if (argv[i][0]=='-' && argv[i][1]==0) {
			fp = stdin;
		} else if (!(fp = fopen(argv[i], "r"))) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i],
			        strerror(errno));
			continue;
		}

		while ((ch = getc(fp))!=EOF) {
			const struct state_function *func = state;
			while (func->ch && func->ch!=ch) ++func;
			if (func->pre_print) putchar(func->pre_print);
			if (func->print) putchar(ch);
			state = func->new_state;
		}

		if (fp!=stdin) {
			fclose(fp);
		}
	}

	return 0;
}
