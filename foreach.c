/*
 * Runs a command for all specified arguments.
 * Copyright (c) 2013 by Michal Nazareicz (mina86@mina86.com)
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

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


static const char *const MARKER = "{+}";


static const char *ARGV0;

static void error(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s: ", ARGV0);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}


static void usage(bool full) {
	FILE *out = full ? stdout : stderr;
	fprintf(out, "usage: %s [<options>] [--] <command> <arg>... -- <value> ...\n",
	        ARGV0);
	if (full) {
		fputs("Possible <options>:\n"
		      "  -j<jobs> run <jobs> jobs at the same time\n"
		      "  -J       run one job per processor\n"
		      "  -K       stop once a job returns non-zero\n"
		      "<value>    values to pass to the <command>\n"
		      "<command>  command to run for each <value>\n"
		      "<arg>      arguments to pass to the <command>,\n"
		      "           {+} will be substituted by <value>,\n"
		      "           if no {+} given, <value> will be passed at the end\n",
		      out);
	} else {
		fprintf(out, "       %s --help\n", ARGV0);
	}
}


struct options {
	unsigned jobs;
	bool keep_going;
	const char **values, **command;
};


static unsigned parseJobs(char *arg) {
	unsigned long val;
	char *end;

	errno = 0;
	val = strtoul(arg, &end, 10);
	if (!val || errno || val > UINT_MAX || *end) {
		error("-j: %s: invalid argument\n", arg);
		return 0;
	}

	return val;
}

static unsigned countProcessors(void) {
	bool ignore = false;
	unsigned count = 0;
	char buffer[128];
	FILE *fd;

	fd = fopen("/proc/cpuinfo", "r");
	if (!fd) {
		error("/cpu/info: %s\n", strerror(errno));
		error("assuming -j1\n");
		return 1;
	}

	while (fgets(buffer, sizeof buffer, fd)) {
		count += !ignore && strncmp(buffer, "processor\t", 10);
		ignore = !strchr(buffer, '\n');
	}

	if (!count) {
		error("no processors detected, assuming -j1\n");
		count = 1;
	}
	return count;
}


static void takeArguments(const char ***argvp, const char ***outp,
                          const char *marker, bool wait_for_dd) {
	const char **argv = *argvp, **out = *outp, *arg;
	bool found = false;

	if (*argv && !strcmp(**argvp, "--")) {
		++argv;
	}

	arg = "--";  /* Whatever non-NULL will suffice */
	for (; arg; ++argv) {
		arg = *argv;
		if (arg) {
			if (wait_for_dd && !strcmp(*argv, "--")) {
				arg = NULL;
			} else if (marker && !strcmp(arg, marker)) {
				arg = marker;
				found = true;
			}
		}
		*out++ = arg;
	}

	if (marker && !found) {
		out[-1] = marker;
		*out++ = NULL;
	}

	*argvp = argv;
	*outp = out;
}


static int parseArgs(int argc, char **_argv, struct options *opts) {
	const char **argv, **out = (const char **)_argv;
	int opt;

	ARGV0 = _argv[0];
	{
		const char *arg = strrchr(ARGV0, '/');
		if (arg) {
			ARGV0 = arg + 1;
		}
	}

	if (argc > 1 && !strcmp(_argv[1], "--help")) {
		usage(true);
		return -1;
	}

	opts->jobs = 1;
	opts->keep_going = true;
	opterr = 0;
	while ((opt = getopt(argc, _argv, "+:j:JKh")) != -1) {
		switch (opt) {
		case 'h':
			usage(true);
			return -1;
		case 'K':
			opts->keep_going = false;
			break;
		case 'j':
			opts->jobs = parseJobs(optarg);
			if (!opts->jobs) {
				goto usage;
			}
			break;
		case 'J':
			opts->jobs = countProcessors();
			break;
		case ':':
			error("-%c: requires an argument\n", optopt);
			goto usage;
		default:
			error("-%c: unknown option\n", optopt);
			goto usage;
		}
	}

	argv = (const char **)(_argv + optind);

	opts->command = out;
	takeArguments(&argv, &out, MARKER, true);
	if (!*opts->command) {
		error("no command given\n");
		goto usage;
	}

	opts->values = out;
	takeArguments(&argv, &out, NULL, false);
	if (!*opts->values) {
		error("no values given\n");
		goto usage;
	}

	return 0;

usage:
	usage(false);
	return 1;
}


static bool run(struct options *opts, const char *value) {
	const char **cmd;
	pid_t pid;

	pid = fork();
	switch (pid) {
	case -1:
		error("fork: %s\n", strerror(errno));
		return false;

	case 0:
		cmd = opts->command;
		while (*cmd) {
			if (*cmd == MARKER) {
				*cmd = value;
			}
			++cmd;
		}

		execvp(opts->command[0], (char **)opts->command);
		error("%s: %s\n", opts->command[0], strerror(errno));
		_exit(1);
	}

	return true;
}


static bool reap(int *rc, bool block) {
	int status;
	pid_t pid = waitpid(-1, &status, block ? 0 : WNOHANG);
	if (pid <= 0) {
		return false;
	} else if (!status) {
		/* nop */
	} else if (WIFSIGNALED(status)) {
		error("[%d]: killed by a signal: %d\n",
		      pid, WTERMSIG(status));
		*rc |= 2;
	} else {
		error("[%d]: exited with exit code: %d\n",
		      pid, WEXITSTATUS(status));
		*rc |= 4;
	}
	return true;
}


static int runCommands(struct options *opts) {
	const char **value;
	unsigned jobs = 0;
	int rc = 0;

	for (value = opts->values;
	     *value && (opts->keep_going || !rc);
	     ++value) {
		if (!run(opts, *value)) {
			continue;
		}

		++jobs;
		while (reap(&rc, jobs >= opts->jobs)) {
			--jobs;
		}
	}

	if (*value) {
		error("terminating before all jobs were started\n");
	}

	while (jobs > 0) {
		reap(&rc, true);
		--jobs;
	}

	return rc;
}


int main(int argc, char **argv) {
	struct options opts;
	int rc;

	rc = parseArgs(argc, argv, &opts);
	if (rc) {
		return rc < 0 ? 0 : rc;
	}

	return runCommands(&opts);
}
