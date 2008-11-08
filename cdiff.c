/*
 * Colorize (adds ANSI codes) output of diff.
 * $Id: cdiff.c,v 1.6 2008/11/08 23:45:57 mina86 Exp $
 * Copyright (c) 2005,2006 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
 *
 * This is part of Tiny Applications Collection
 *   -> http://tinyapps.sourceforge.net/
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>


#if __STDC_VERSION__ < 199901L
#  if defined __GNUC__
#    define restrict   __restrict__
#  else
#    define restrict
#  endif
#endif


static int match(const char *restrict pattern, const char *restrict str);


int main(int argc, char **argv) {
	enum color_number {
		COLOR_INS,
		COLOR_DEL,
		COLOR_CHANGE,
		COLOR_EQUAL,
		COLOR_FILE1,
		COLOR_FILE2,
		COLOR_POS1,
		COLOR_POS2,
		COLOR_MISC,
		COLOR_DEFAULT
	};


	static const struct color {
		const char *const name;
		const char *value;
	} colors[] = {
		{ "ins" ,    "1;32" },
		{ "del" ,    "1;31" },
		{ "change",  "1;33" },
		{ "equal",   "0;37" },
		{ "file1",   "0;32" },
		{ "file2",   "0;31" },
		{ "pos1",    "0;32" },
		{ "pos2",    "0;31" },
		{ "misc",    "0;35" },
		{ "default", "0;37" },
	};


	struct rule {
		const char *const pattern;
		enum color_number idx;
	};


	static const struct rule unified_rules[] = {
		{ "--- ", COLOR_FILE1   },
		{ "+++ ", COLOR_FILE2   },
		{ "@@ " , COLOR_MISC    },
		{ "+"   , COLOR_INS     },
		{ "-"   , COLOR_DEL     },
		{ " "   , COLOR_EQUAL   },
		{ ""    , COLOR_DEFAULT },
	};

	static const struct rule contex_rules[] = {
		{ "*** [^0-9]", COLOR_FILE1   },
		{ "--- [^0-9]", COLOR_FILE2   },
		{ "*** "      , COLOR_POS1    },
		{ "--- "      , COLOR_POS2    },
		{ "*"         , COLOR_MISC    },
		{ "?[^ ]"     , COLOR_MISC    },
		{ "+ "        , COLOR_INS     },
		{ "- "        , COLOR_DEL     },
		{ "! "        , COLOR_CHANGE  },
		{ "  "        , COLOR_EQUAL   },
		{ ""          , COLOR_DEFAULT },
	};

	static const struct rule normal_rules[] = {
		{ "[0-9]", COLOR_MISC   },
		{ "---"  , COLOR_MISC   },
		{ "> "   , COLOR_INS    },
		{ "< "   , COLOR_DEL    },
		{ ""     , COLOR_DEFAULT },
	};


	static const struct {
		const char *const pattern;
		const struct rule *const rules;
	} modes[] = {
		{ "-", unified_rules },
		{ "*", contex_rules  },
		{ "" , normal_rules  },
	};

	const struct rule *rules;
	char buf[1024];
	int i;


	/* Run diff if we were run as a wrapper */
	if (argc>1) {
		int fds[2];

		if (pipe(fds)==-1) {
			perror("pipe");
			return 1;
		}

		switch (fork()) {
		case -1:
			perror("fork");
			return 1;

		case 0:
			close(fds[0]);
			if (dup2(fds[1], 1)==-1) {
				perror("dup2");
				return 1;
			}
			argv[0] = (char*)"diff";
			execvp("diff", argv);
			perror("exec: diff");
			return 1;

		default:
			close(fds[1]);
			if (dup2(fds[0], 0)==-1) {
				perror("dup2");
				return 1;
			}
		}
	}


	/* Decide the type of diff */
	if (!fgets(buf, sizeof buf, stdin)) {
		return 0;
	}

	for (i = 0; !match(modes[i].pattern, buf); ++i);
	rules = modes[i].rules;

	/* Main loop */
	do {
		for (i = 0; !match(rules[i].pattern, buf); ++i);
		printf("\x1b[%sm%s", colors[rules[i].idx].value, buf);
	} while (fgets(buf, sizeof buf, stdin));


	puts("\x1b[0m");
	return 0;
}




static int match(const char *restrict pat, const char *restrict str) {
	if (!pat || !str) {
		return 0;
	}

	for (; *pat && *str; ++str, ++pat) {
		const char ch = *str;
		int found = 0, expect = 1;

		if (*pat=='?') {
			continue;
		} else if (*pat!='[') {
			if (*(*pat=='\\' ? ++pat : pat) != ch) return 0;
			continue;
		}

		if (*++pat=='^') {
			expect = 0;
			++pat;
		}

		for (; *pat && *pat!=']'; ++pat) {
			if (*pat=='\\' && !*++pat) return 0;
			if (pat[1]!='-') {
				found |= ch == *pat;
			} else {
				const char from = *pat, to = *(pat += 2);
				found |= from>to ? (ch<=to||from<=ch) : (from<=ch&&ch<=to);
			}
		}

		if (found!=expect || !*pat) return 0;
	}
	return 1;
}
