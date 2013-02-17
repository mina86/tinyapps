/*
 * Colorize (adds ANSI codes) output of diff.
 * Copyright (c) 2005,2006 by Michal Nazareicz (mina86/AT/mina86.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This is part of Tiny Applications Collection
 *   -> http://tinyapps.sourceforge.net/
 */

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
	const char *pattern;
	unsigned idx;
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


	static const struct color {
		const char name[8];
		const char value[8];
	} colors[] = {
		{ "default", "0;37" },
		{ "ins" ,    "1;32" },
		{ "del" ,    "1;31" },
		{ "change",  "1;33" },
		{ "equal",   "0;37" },
		{ "file1",   "0;32" },
		{ "file2",   "0;31" },
		{ "pos1",    "0;32" },
		{ "pos2",    "0;31" },
		{ "misc",    "0;35" },
	};


	static const struct pattern unified_ruleset[] = {
		{ "+++ ", COLOR_FILE1   },
		{ "--- ", COLOR_FILE2   },
		{ "@@ " , COLOR_MISC    },
		{ "+"   , COLOR_INS     },
		{ "-"   , COLOR_DEL     },
		{ " "   , COLOR_EQUAL   },
		{ 0     , COLOR_DEFAULT },
	};

	static const struct pattern contex_ruleset[] = {
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

	static const struct pattern normal_ruleset[] = {
		{ "[0-9]", COLOR_MISC   },
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
		{ "-",     1 },
		{ "*",     2 },
		{ "[0-9]", 3 },
		{ 0,       0 } /* side note: 0 == COLOR_DEFAULT */
	};


	const struct pattern *ruleset = 0;
	char buf[4096];


	while (fgets(buf, sizeof buf, stdin)) {
		unsigned idx;
		char *ch;

		do {
			idx = find(ruleset ? ruleset : modes, buf);
		} while (!ruleset && (ruleset = rulesets[idx]));

		printf("\x1b[%sm", colors[idx].value);

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
static int match(const char *pattern, const char *str);
static const char *match_group(const char *pat, char ch);


static unsigned find(const struct pattern *patterns, const char *str) {
	while (!match(patterns->pattern, str)) {
		++patterns;
	}
	return patterns->idx;
}


static int match(const char *pat, const char *str) {
	if (!pat) {
		return 1;
	}

	for (; *pat && *str; ++str, ++pat) {
		switch (*pat) {
		case '?':
			break;

		case '[':
			pat = match_group(pat, *str);
			if (!pat) return 0;
			break;

		default:
			if (*pat != *str) return 0;
		}
	}
	return 1;
}


static const char *match_group(const char *pat, char ch) {
	int ok = 0;

	++pat;
	if (*pat == '^') {
		ok = 1;
		++pat;
	}

	do {
		if (pat[1] != '-') {
			if (ch != *pat) continue;
		} else {
			const char from = *pat;
			const char to = *(pat += 2);

			if (from > to ? (to < ch   && ch < from)
			              : (ch < from || to < ch  )) continue;
		}

		/* Found */
		while (*++pat != ']') {
			/* nop */
		}
		ok = ok ^ 1;
		break;

	} while (*++pat != ']');

	return ok ? pat : 0;
}
