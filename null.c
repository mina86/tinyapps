/*
 * Discards standard input.
 * $Id: null.c,v 1.9 2008/04/17 21:37:12 mina86 Exp $
 * Copyright (c) 2005-2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * usage: <progra> <args> | null
 *
 * Discards all data read from standard input.  For people feeling
 * >/dev/null is too much to type.
 *
 * usage: run [ -n | -b | -B | -d | -D ] <program> <args>
 *
 * If -n is given program will only redirect stdout and stderr to
 * /dev/null and execute given program.
 *
 * If -b is given program will fork() and then execute the command
 * with stdout and stderr refirected to /dev/null.  -B makes it also
 * print the PID of the process.
 *
 * If -d is given program will fork(), setsid() (to become process
 * group and session group leader), fork() once again (so process
 * cannot regain a controlling terminal), change cwd to root
 * directory, then close() all opened file descriptors above 2, and
 * finally execute the command with all stdin, stdout and stderr
 * redirected to/from /dev/null.  -D makes it also print the PID of
 * the process.
 *
 * The default is -b but program checks how it was called and if the
 * first letter of the command maches one of the switches given switch
 * is taken as default.
 */


/******** Includes ********/
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>


/******** Usage ********/
void usage(FILE *out, char *cmd, int full) {
	fprintf(out, "usage: %s [-n | -b | -B | -d | -D] <program> <args>\n"
	             "       <program> <args> | null", cmd);
	if (!full) return;
	fputs("  -n  discard output only (runs in foreground)\n"
	      "  -b  discard output and run in background (ie. fork) [default]\n"
	      "  -B  -b + print child's PID\n"
	      "  -d  discard input and output and daemonization"
	      "  -D  -d + print child's PID\n"
	      "-b is the default unless command's name first letter maches one of the\n"
	      "switches in which case taht switch becomes default.\n", out);
}


/******** Main ********/
int main(int argc, char **argv) {
	int daemonize;

	/* Parse command name */
	{
		char *c = *argv;
		for (; *c; ++c) {
			if (*c=='/' && c[1] && c[1]!='/') *argv = ++c;
		}
	}
	switch (*argv[0]) {
	case 'n': daemonize = 0; break;
	case 'B': daemonize = 2; break;
	case 'd': daemonize = 3; break;
	case 'D': daemonize = 4; break;
	default:  daemonize = 1;
	}


	/* No arguments */
	if (argc < 2) {
		if (!daemonize) {
			char buf[4096];
			chdir("/");
			while (read(0, buf, sizeof buf) > 0);
		} else {
			usage(stdout, *argv, 1);
		}
		return 0;
	}


	/* Parse argument */
	if (argv[1][0]=='-' && argv[1][1] && argv[1][2] == 0) {
		switch (argv[1][1]) {
		case 'n': daemonize = 0; break;
		case 'b': daemonize = 1; break;
		case 'B': daemonize = 2; break;
		case 'd': daemonize = 3; break;
		case 'D': daemonize = 4; break;

		case 'h':
			usage(stdout, *argv, 1);
			return 0;

		default:
			usage(stderr, *argv, 0);
			return 1;
		}
		++argv;
		--argc;

		if (argc == 1) {
			usage(stderr, *argv, 0);
			return 1;
		}
	}


#define DIE(cond, msg) if (cond) { perror(msg); return 1; } else (void)0

	/* Fork into background */
	if (daemonize) {
		pid_t pid = fork();

		switch (pid) {
		case -1: DIE(1, "fork");
		case  0: break;
		default:
			if (daemonize==2) printf("%d\n", (int)pid);
			_exit(0);
		}
	}

	/* Daemonize */
	if (daemonize>2) {
		pid_t pid;
		int i;

		DIE(chdir("/"), "chdir: /");
		DIE(setsid() < 0, "setsid");

		switch (pid = fork()) {
		case -1: DIE(1, "fork");
		case  0: break;
		default:
			if (daemonize==4) printf("%d\n", (int)pid);
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
	perror("exec");
	return 2;
}
