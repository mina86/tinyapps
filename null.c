/*
 * Discards standard input.
 * Copyright (c) 2005-2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This software is OSI Certified Open Source Software.
 * OSI Certified is a certification mark of the Open Source Initiative.
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

/*
 * With -n redirects stdout and stderr to /dev/null and execute given
 * program.
 *
 * With -b or -B fork()s and then execute the command with stdout and
 * stderr refirected to /dev/null.
 *
 * With -d or -D fork()s, setsid()s (to become process group, session
 * group leader and loose controling terminal), fork()s once again (so
 * process cannot regain a controlling terminal), changes cwd to root
 * directory, then close()s all opened file descriptors above 2, and
 * finally executes the command with all stdin, stdout and stderr
 * redirected to/from /dev/null.
 *
 * The default is -b but program checks how it was called and if the
 * first letter of the command maches one of the switches given switch
 * is taken as default.
 *
 * With -N adjusts processes nice value.  This feature can be disabled
 * at compile time with DISABLE_NICE macro.
 *
 * With -b, -B, -d or -D parent waits one second in case child exits
 * so that it can report on exit status etc.  This is disabled with -w
 * switch.  It can be also disabled at compile time with DISABLE_WAIT
 * macro.
 */


/******** Config ********/
#define DISABLE_NICE 0 /* Set to 1 to disable -N feature. */
#define DISABLE_WAIT 0 /* Set to 0 to disable waiting. */


/******** Includes ********/
#if !DISABLE_NICE
#  define _BSD_SOURCE 1
#endif
#if !DISABLE_WAIT
#  define _POSIX_SOURCE 1
#endif

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#if !DISABLE_NICE
#  include <stdlib.h>
#endif
#if !DISABLE_WAIT
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <signal.h>
#endif


/******** Usage ********/
static void usage(FILE *out, char *cmd, int full) {
	fprintf(out, "usage: %s [<switches>] [--] <program> <args>\n"
	             "       <program> <args> | null", cmd);
	if (!full) return;
	fputs("<switches> are:\n"
	      "  -n         discard output only (runs in foreground)\n"
	      "  -b         discard output and run in background (ie. fork)\n"
	      "             default unless command name first letter maches one\n"
	      "             of the [xBdD] (x means b)\n"
	      "  -B         -b + print child's PID\n"
	      "  -d         discard input and output and daemonization\n"
	      "  -D         -d + print child's PID\n"
#if !DISABLE_NICE
	      "  -N<[adj>]  adjust nice by <adj> (10 by default)\n"
#endif
#if !DISABLE_WAIT
	      "  -w         do not wait a second in case child exits\n"
#endif
	      "  -c<dir>    change directory to <dir>\n"
	      "  -c         do not change directory when daemonizing\n"
	      , out);
}


#if DISABLE_WAIT
#  define wait_and_exit(argv0) _exit(0)
#else
static void wait_and_exit(char *argv0);
#endif


/******** Main ********/
int main(int argc, char **argv) {
	const char *dir = 0;
	char *argv0, *c;
	int daemonize;
#if !DISABLE_NICE
	int nice_adj = 0;
#endif
#if !DISABLE_WAIT
	int doWait = 1;
#endif

	/* Parse command name */
	for (c = argv0 = *argv; *c; ++c) {
		if (*c=='/' && c[1] && c[1]!='/') argv0 = ++c;
	}
	switch (*argv0) {
	case 'n': daemonize = 0; break;
	case 'B': daemonize = 2; break;
	case 'd': daemonize = 3; dir = "/"; break;
	case 'D': daemonize = 4; dir = "/"; break;
	default:  daemonize = 1;
	}


	/* No arguments */
	if (argc < 2) {
		if (!daemonize) {
			char buf[4096];
			chdir("/");
			while (read(0, buf, sizeof buf) > 0);
		} else {
			usage(stdout, argv0, 1);
		}
		return 0;
	}


	/* Parse argument */
	while (argc > 1 && argv[1][0] == '-') {
		int pos = 1;
		++argv;
		--argc;
		do {
			switch (argv[0][pos]) {
			case 'n': daemonize = 0; break;
			case 'b': daemonize = 1; break;
			case 'B': daemonize = 2; break;
			case 'd': daemonize = 3; dir = "/"; break;
			case 'D': daemonize = 4; dir = "/"; break;

#if !DISABLE_NICE
			case 'N':
				nice_adj = 10;
				if (argv[0][pos + 1] == 0) break;
				errno = 0;
				nice_adj = strtol(argv[0] + pos + 1, &c, 0);
				if (errno || *c || nice_adj < -40 || nice_adj > 40) {
					fprintf(stderr, "%s: invalid adjustment: %s\n",
					        argv0, argv[0] + pos + 1);
					return 1;
				}
				pos = c - argv[0] - 1;
				break;
#endif

#if !DISABLE_WAIT
			case 'w': doWait = 0; break;
#endif

			case 'c':
				if (argv[0][pos + 1]) {
					dir = argv[0] + pos + 1;
					pos += strlen(dir);
				} else {
					dir = 0;
				}
				break;

			case 'h': usage(stdout, argv0, 1); return 0;
			case '-': if (pos == 1 && !argv[0][2]) goto args_end;
			default : usage(stderr, argv0, 0); return 1;
			}
		} while (argv[0][++pos]);
	}

 args_end:
	if (argc == 1) {
		usage(stderr, argv0, 0);
		return 1;
	}


#define DIE(cond, msg) if (cond) { \
		fprintf(stderr, "%s: %s: %s\n", argv0, msg, strerror(errno)); \
		return 1; \
	} else (void)0

#if !DISABLE_NICE
	/* Nice */
	if (nice_adj) {
		errno = 0;
		nice(nice_adj);
		DIE(errno, "nice");
	}
#endif

	/* Fork into background */
	if (daemonize) {
		pid_t pid = fork();

		switch (pid) {
		case -1: DIE(1, "fork");
		case  0: break;
		default:
			if (daemonize==2) printf("%d\n", (int)pid);
#if !DISABLE_WAIT
			if (doWait) wait_and_exit(daemonize < 3 ? argv0 : 0);
#endif
			_exit(0);
		}
	}

	/* Change dir */
	if (dir) {
		DIE(chdir(dir), dir);
	}

	/* Daemonize */
	if (daemonize>2) {
		pid_t pid;
		int i;

		DIE(setsid() < 0, "setsid");

		switch (pid = fork()) {
		case -1: DIE(1, "fork");
		case  0: break;
		default:
			if (daemonize==4) printf("%d\n", (int)pid);
#if !DISABLE_WAIT
			if (doWait) wait_and_exit(argv0);
#endif
			_exit(0);
		}

		/* Close all fds and stdin*/
		DIE(close(0) < 0, "close: stdin");
		DIE(open("/dev/null", O_RDONLY) < 0, "open: /dev/null");
		for (i = sysconf(_SC_OPEN_MAX); i > 3; close(--i)) /* nop */;
	}

	/* Run app */
	DIE(close(1) < 0, "close: stdout");
	DIE(open("/dev/null", O_WRONLY) < 0, "open: /dev/null");
	DIE(dup2(1, 2) < 0, "dup: stdout, stderr");

	execvp(argv[1], argv + 1);
	/*
	 * fprintf(stderr, "%s: exec: %s: %s", argv0, argv[1], strerror(errno));
	 * No use writing anything to stderr anyway now.
	 */
	return 3;
}


/******** Wait for child for a while and exit ********/
#if !DISABLE_WAIT
static void sig_dummy(int sig) { (void)sig; }

static void wait_and_exit(char *argv0) {
	struct sigaction sa;
	int status;

	sa.sa_handler = sig_dummy;
	sa.sa_flags = SA_NOMASK;
	sigaction(SIGALRM, &sa, 0);

	alarm(1);
	if (wait(&status) <= 0) {
		_exit(0);
	}

	if (WIFEXITED(status)) {
		if (argv0 && WEXITSTATUS(status)) {
			fprintf(stderr, "%s: child exited with code: %d\n", argv0,
			        WEXITSTATUS(status));
		}
		_exit(WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		if (argv0) {
#ifdef WCOREDUMP
			fprintf(stderr, "%s: child recieved signal: %d%s\n", argv0,
			        WTERMSIG(status),
			        WCOREDUMP(status) ? " [core dumped]" : "");
#else
			fprintf("%s: child recieved signal: %d\n", argv0,
			        WTERMSIG(status));
#endif
		}
		_exit(2);
	}

	_exit(0);
}
#endif
