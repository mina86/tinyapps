/*
 * Colorize (adds ANSI codes) output of diff.
 * Copyright (c) 2005,2006 by Michal Nazareicz <mina86@mina86.com>
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



/******************** Main ***************************************************/
static void not_tty(char **argv);
static int  run_diff(char **argv);
static void loop(void);

int main(int argc, char **argv) {
	/* If not TTY either run diff or pass in->out through (cat) */
	if (!isatty(1)) {
		not_tty(argv);
		return 1;
	}

	/* If arguments given run diff and prepare pipe */
	if (argc > 1) {
		int ret = run_diff(argv);
		if (ret) {
			return ret;
		}
	}

	/* Colourise output */
	loop();
	return 0;
}



/******************** Handle no TTY case *************************************/
static void not_tty(char **argv) {
	argv[0] = (char *)(argv[1] ? "diff" : "cat");
	execvp(argv[0], argv);
	perror(argv[1] ? "exec: diff" : "exec: cat");
}



/******************** Run diff and set up pipe *******************************/
static int run_diff(char **argv) {
	int fds[2];

	if (pipe(fds) < 0) {
		perror("pipe");
		return 1;
	}

	switch (fork()) {
	case -1:
		perror("fork");
		return 1;

	case 0:
		close(fds[0]);
		if (dup2(fds[1], 1) < 0) {
			perror("dup2");
			return 1;
		}
		argv[0] = (char *)"diff";
		execvp("diff", argv);
		perror("exec: diff");
		return 1;

	default:
		close(fds[1]);
		if (dup2(fds[0], 0) < 0) {
			perror("dup2");
			return 1;
		}
	}

	return 0;
}



/******************** Main loop; main part of the app ************************/
struct pattern {
	const char pattern[7];
	unsigned char idx;
};

static unsigned find(const struct pattern *patterns, const char *str);


static void loop(void) {
	enum color_number {
		COLOR_DEFAULT, /* must be zero */
		COLOR_INS,
		COLOR_DEL,
		COLOR_CHANGE,
		COLOR_EQUAL,
		COLOR_FILE1,
		COLOR_FILE2,
		COLOR_POS1,
		COLOR_POS2,
		COLOR_MISC
	};


	static const char colors[][8] = {
		[COLOR_DEFAULT] = "0;37",
		[COLOR_INS]     = "1;32",
		[COLOR_DEL]     = "1;31",
		[COLOR_CHANGE]  = "1;33",
		[COLOR_EQUAL]   = "0;37",
		[COLOR_FILE1]   = "0;32",
		[COLOR_FILE2]   = "0;31",
		[COLOR_POS1]    = "0;32",
		[COLOR_POS2]    = "0;31",
		[COLOR_MISC]    = "0;35",
	};


	static const struct pattern unified_ruleset[] = {
		{ "+++ " , COLOR_FILE1   },
		{ "--- " , COLOR_FILE2   },
		{ "@@ "  , COLOR_MISC    },
		{ "+"    , COLOR_INS     },
		{ "-"    , COLOR_DEL     },
		{ " "    , COLOR_EQUAL   },
		{ ""     , COLOR_DEFAULT },
	};

	static const struct pattern contex_ruleset[] = {
		{ "*** 0", COLOR_POS1    },
		{ "--- 0", COLOR_POS2    },
		{ "*** " , COLOR_FILE1   },
		{ "--- " , COLOR_FILE2   },
		{ "*"    , COLOR_MISC    },
		{ "+ "   , COLOR_INS     },
		{ "- "   , COLOR_DEL     },
		{ "! "   , COLOR_CHANGE  },
		{ "  "   , COLOR_EQUAL   },
		{ "? "   , COLOR_DEFAULT },
		{ "??"   , COLOR_MISC    },
		{ ""     , COLOR_DEFAULT },
	};

	static const struct pattern normal_ruleset[] = {
		{ "0"    , COLOR_MISC   },
		{ "---"  , COLOR_MISC   },
		{ "> "   , COLOR_INS    },
		{ "< "   , COLOR_DEL    },
		{ ""     , COLOR_DEFAULT },
	};

	static const struct pattern *rulesets[] = {
		0, /* must be at index zero */
		unified_ruleset,
		contex_ruleset,
		normal_ruleset
	};


	static const struct pattern modes[] = {
		{ "-", 1 },
		{ "*", 2 },
		{ "0", 3 },
		{ "",  0 } /* side note: 0 == COLOR_DEFAULT */
	};


	const struct pattern *ruleset = 0;
	char buf[4096];

	while (fgets(buf, sizeof buf, stdin)) {
		unsigned idx;
		char *ch;

		do {
			idx = find(ruleset ? ruleset : modes, buf);
		} while (!ruleset && (ruleset = rulesets[idx]));

		printf("\x1b[%sm", colors[idx]);

		do {
			ch = strchr(buf, '\n');
			if (ch) {
				*ch = '\0';
			}
			fputs(buf, stdout);
		} while (!ch && fgets(buf, sizeof buf, stdin));
		puts("\x1b[0m");
	}
}



/******************** String matching ****************************************/
static bool match(const char *pattern, const char *str);
static const char *match_group(const char *pat, char ch);


static unsigned find(const struct pattern *patterns, const char *str) {
	while (!match(patterns->pattern, str)) {
		++patterns;
	}
	return patterns->idx;
}


static bool match(const char *pat, const char *str) {
	for (;;) {
		switch (*pat) {
		case 0:
			return true;
		case '?':
			if (!*str) return false;
			break;
		case '0':
			if (*str < '0' || '9' < *str) return false;
			break;
		default:
			if (*pat != *str) return false;
		}
		++pat;
		++str;
	}
}
