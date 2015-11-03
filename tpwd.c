/*
 * Truncates pwd to specified number of characters
 * Copyright (c) 2015 Michal Nazarewicz <mina86@mina86.com>
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

#define _POSIX_C_SOURCE 2
#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


/* Cap PATH_MAX at this value when allocating initial buffer.  This only limits
 * what the buffer can be at the beginning.  If path to the current working
 * directory is longer, the buffer will be resized. */
#define MAX_PATH_MAX      32768

/* Cap buffer for path at this value when resizing it up.  If path to the
 * current working directory ends up longer than this, failure message will be
 * printed and dot used instead. */
#define MAX_PATH_LENGTH 1048576


struct opts {
	char *pwd;
	const char *trunc_str;
	unsigned trunc_str_len, max_len;
	bool logical, no_nl;
	bool pwd_mode;
};

static void parse_opts(struct opts *opts, int argc, char **argv);
static char *getpwd(bool logical);
static char *shorten_pwd(char *pwd, bool home_only);
static void output(struct opts *opts);


int main(int argc, char **argv) {
	struct opts opts = {
		.trunc_str = "...",
		.trunc_str_len = 3,
	};

	parse_opts(&opts, argc, argv);
	if (!opts.pwd) {
		opts.pwd = getpwd(opts.logical);
	}
	opts.pwd = shorten_pwd(opts.pwd, opts.pwd_mode);
	output(&opts);
	return 0;
}


static unsigned parse_uint(const char *str);
static void usage(bool error);


static void parse_opts(struct opts *opts, int argc, char **argv) {
	char *argv0 = *argv, *ch;
	int i;

	/* Strip directories from argv0 and check if we are run as ‘pwd’. */
	ch = strrchr(argv0, '/');
	if (ch) {
		argv0 = ch + 1;
	}
	opts->pwd_mode = !strcmp(argv0, "pwd");
	opts->logical = !opts->pwd_mode;

	/* Parse options */
	for (;;) {
		switch (getopt(argc, argv, "LPnh")) {
		case 'L': opts->logical = true; continue;
		case 'P': opts->logical = false; continue;
		case 'n': opts->no_nl = true; continue;
		case 'h': usage(false);
		case -1: break;
		default: usage(true);
		}
		break;
	}

	/* Parse positional arguments */
	i = argc - optind;
	switch (i) {
	case 4:
		opts->pwd = argv[optind + 3];
	case 3:
		opts->trunc_str_len = parse_uint(argv[optind + 2]);
	case 2:
		opts->trunc_str = argv[optind + 1];
		if (i < 3)
			opts->trunc_str_len = strlen(opts->trunc_str);
	case 1:
		opts->max_len = parse_uint(argv[optind]);
		break;
	case 0:
		opts->max_len = opts->pwd_mode ? 0 : 30;
		break;
	default:
		usage(true);
	}
}


static unsigned parse_uint(const char *str) {
	unsigned long ret;
	char *end;

	errno = 0;
	ret = strtoul(str, &end, 10);
	if (errno || ret > UINT_MAX || *end) {
		fprintf(stderr, "tpwd: %s: not a valid number", str);
		usage(false);
	}
	return ret;
}


static void usage(bool error)
{
	fputs("usage: tpwd [ -L | -P ] [ -n ] "
	      "[<len> [<trunc> [<tlen> [<path>]]]]\n",
	      error ? stderr : stdout);
	if (error) {
		exit(EXIT_FAILURE);
	}

	puts("\
 -L       use $PWD, even if it contains symlinks        [the default]\n\
 -P       avoid all symlinks\n\
 -n       doesn't print new line character at the end\n\
 <len>    maximum length of printed string,         [0 if run as pwd]\n\
          0 to disable truncating                      [30 otherwise]\n\
 <trunc>  string to print at the beginning if pwd was truncated [...]\n\
 <tlen>   length of <trunc> (useful if using ANSI codes or UTF-6 chars)\n\
 <path>   path to use, no validation on the path will be performed\n\
\n\
Command might be used in PS1 variable to truncate very long PWD's which\n\
normally would took a whole line (or more), for example:\n\
    PS1='[\\u@\\h $(tpwd -n 30 {)]\\$ '    # works in bash");
	exit(EXIT_SUCCESS);
}


static char *getpwd(bool logical) {
	static char dot[] = ".";

	size_t capacity;
	char *pwd, *buf;

	if (logical) {
		struct stat l, p;

		pwd = getenv("PWD");
		if (pwd && *pwd == '/' &&
		    stat(pwd, &l) != -1 && stat(dot, &p) != -1 &&
		    l.st_dev == p.st_dev && l.st_ino == p.st_ino) {
			return pwd;
		}
	}

	capacity = PATH_MAX > MAX_PATH_MAX ? MAX_PATH_MAX : PATH_MAX;
	buf = malloc(capacity);
	if (!buf) {
		goto nomem;
	}

	while (!(pwd = getcwd(buf, capacity)) &&
	       errno == ERANGE && capacity < MAX_PATH_LENGTH) {
		capacity *= 2;
		pwd = realloc(buf, capacity);
		if (!pwd) {
			free(buf);
			goto nomem;
		}
		buf = pwd;
	}

	if (!pwd) {
nomem:
		fputs("tpwd: unable to figure out PWD\n", stderr);
		pwd = dot;
	}

	return pwd;
}


static char *shorten_pwd(char *pwd, bool home_only) {
	static char tilde[] = "~";

	size_t pwd_len = strlen(pwd), var_len, len;
	char *vars, *var, *dir;

	if (home_only || !(vars = getenv("TPWD_DIRS"))) {
		vars = tilde;
	}

	for (;;) {
		/* Skip leading ':' from vars. */
		for (var = vars; *var && *var == ':'; ++var) {
			/* nop */
		}
		if (!*var) {
			break;
		}

		/* Find next ':'.  Use strchrnul if possible. */
#if (__GLIBC__ << 16) + (__GLIBC_MINOR__) >= (2 << 16 + 1)
		vars = strchrnul(var, ':');
		var_len = vars - var;
		if (*vars) {
			*vars = 0;
			++vars;
		}
#else
		vars = strchr(var, ':');
		if (vars) {
			var_len = vars - var;
			*vars = 0;
			++vars;
		} else {
			var_len = strlen(var);
			vars = var + var_len;
		}
#endif

		/* var_len == 0 indicates var is "~" which is handled specially:
		 * we’re reading $HOME but simplifying it to just ~. */
		if (var_len == 1 && *var == '~') {
			var_len = 0;
		}

		/* Read the value.  If it’s not an absolute path, ignore. */
		dir = getenv(var_len ? var : "HOME");
		if (!dir || !*dir || *dir != '/') {
			continue;
		}

		// Strip trailing slashes
		for (len = strlen(dir); len && dir[len - 1] == '/'; --len) {
			/* nop */
		}
		if (!len ||  /* empty string, ignore */
		    len > pwd_len ||  /* dir cannot be prefix of pwd */
		    len < 1 + var_len ||  /* replacement longer then dir */
		    memcmp(pwd, dir, len) ||  /* not a prefix */
		    (pwd[len] != '/' && pwd[len])) {  /* not a dir name */
			continue;
		}

		pwd += len - 1 - var_len;
		if (var_len) {
			*pwd = '$';
			memcpy(pwd + 1, var, var_len);
		} else {
			*pwd = '~';
		}
		break;
	}

	return pwd;
}


static void output(struct opts *opts) {
	size_t len = strlen(opts->pwd);

	if (!opts->max_len || len <= opts->max_len) {
		fputs(opts->pwd, stdout);
	} else {
		fputs(opts->trunc_str, stdout);
		if (opts->trunc_str_len < opts->max_len) {
			fputs(opts->pwd +
			      (len - opts->max_len + opts->trunc_str_len),
			      stdout);
		}
	}

	if (!opts->no_nl) {
		putc('\n', stdout);
	}
}
