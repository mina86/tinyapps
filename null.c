/*
 * Discards standard input.
 * $Id: null.c,v 1.7 2007/06/30 08:41:02 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/mina86.com)
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
 * usage: null [ [ -b | -B | -d | -D ] <program> <args> ]
 *
 * If called without arguments program will eat data send to it's
 * stdin so one can type `command | null` instead of `command
 * >/dec/null`. ;)
 *
 * If called with a command executes it with stdout and stderr
 * redirected to /dev/null.
 *
 * If -b is given before the command program will first fork() and
 * then execute the command with stdout and stderr refirected to
 * /dev/null. -B makes it also print the PID of the process.
 *
 * If -d is given before the command program will fork(), setsid() (to
 * become process group and session group leader), fork() once again
 * (so process cannot regain a controlling terminal), then close() all
 * opened file descriptors above 2, and finally execute the command
 * with all stdin, stdout and stderr redirected to/from /dev/null. -D
 * makes it also print the PID of the process.
 *
 * Running it as drun or xrun defaults to the -b parameter.
 */


/******** Config ********/

/* Comment this out if you have problems compiling (may happen on non
   POSIX systems).  Note, that exec and daemonize features require
   unistd.h */
#define HAVE_UNISTD_H

/* Comment this out if you want an exec feature to be compiled it. */
/* #define DISABLE_EXEC */

/* Comment this out if you want forking to work. */
/* #define DISABLE_DAEMONIZE */

/* Comment this out if you want the app to take -d and -D arguments.
   This takes effect only if daemonize feature is turned on. */
/* #define DISABLE_ARGS */

/* Comment this out if you do not want the app to assume it was run as
   drun and daemonize by default.  It will disable a pipe feature.  */
/* #define BUILD_DRUN */

/* Size of buffer used when reading data from stdin. */
#define BUFFER_SIZE 1024



/******** Dependencies ********/
#ifndef HAVE_UNISTD_H
# ifndef DISABLE_EXEC
#  define DISABLE_EXEC
# endif
#endif

#ifdef DISABLE_EXEC
# ifndef DISABLE_DAEMONIZE
#  define DISABLE_DAEMONIZE
# endif
#endif

#ifdef DISABLE_DAEMONIZE
# ifdef BUILD_DRUN
#  undef BUILD_DRUN
# endif
#endif


/******** Includes ********/
#ifdef HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
# ifndef DISABLE_EXEC
#  include <fcntl.h>
# endif
#else
# include <stdio.h>
#endif

#ifndef CHAR_BIT
# include <limits.h>
#endif


/******** Main ********/
#ifdef DISABLE_EXEC
int main(void) {
# define argc 1
#else
int main(int argc, char **argv) {
#endif

#ifndef DISABLE_DAEMONIZE
	int daemonize = 0;
#else
# define daemonize 0
#endif


	/* No args given or exec feature disabled */
	if (argc==1) {
#ifdef BUILD_DRUN
		write(2, "usage: drun [-D | -b | -B ] command\n", 36);
		return 1;
#else
		char buf[BUFFER_SIZE];
# ifdef HAVE_UNISTD_H
		while (read(0, buf, sizeof(char) * 1024));
# else
		while (fread(buf, sizeof(char), 1024, stdin));
# endif
		return 0;
#endif
	}



#ifndef DISABLE_EXEC
# ifndef DISABLE_DAEMONIZE
#  ifdef BUILD_DRUN
	daemonize = 1;
#  else /* BUILD_DRUN */
	/* Check whether was run as drun */
	{
		char *c, *s = argv[0];
		for (c = argv[0]; *c; ++c) {
			if (*c=='/' && c[1] && c[1]!='/') s = ++c;
		}
		if ((s[0]=='d' || s[0]=='x')
			&& s[1]=='r' && s[2]=='u' && s[3]=='n' && !s[4]) {
			daemonize = 1;
		}
	}
#  endif /* BUILD_DRUN */

#  ifndef DISABLE_ARGS
	/* Check whether -d or -D was given */
	if (argc>2 && argv[1][0]=='-' && !argv[1][2]) {
		switch (argv[1][1]) {
		case 'b': if (daemonize!=1) { daemonize = 1; ++argv; } break;
		case 'B': daemonize = 2; ++argv; break;
		case 'd': daemonize = 3; ++argv; break;
		case 'D': daemonize = 4; ++argv; break;
		}
	}
#  endif /* DISABLE_ARGS */


	/* Fork into background */
	if (daemonize) {
		pid_t pid;
		char buf[sizeof(pid_t) * CHAR_BIT / 3 + 2];
		int i;

		switch (pid = fork()) {
		case -1: return 2;
		case  0: break;
		default:
			if (daemonize!=2) return 0;
			buf[i = sizeof(buf) - 1] = '\n';
			do { buf[--i] = '0' + (pid % 10); } while (pid /= 10);
			write(1, buf+i, sizeof(buf) - i);
			_exit(0);
		}
	}

	/* Daemonize */
	if (daemonize>2) {
		pid_t pid;
		char buf[sizeof(pid_t) * CHAR_BIT / 3 + 2];
		int i, max;

		setsid();
		pid = fork();

		switch (pid) {
		case -1: return 2;
		case  0: break;
		default:
			if (daemonize!=4) return 0;
			buf[i = sizeof(buf) - 1] = '\n';
			do { buf[--i] = '0' + (pid % 10); } while (pid /= 10);
			write(1, buf+i, sizeof(buf) - i);
			_exit(0);
		}

		/* Close all fds */
		max = sysconf(_SC_OPEN_MAX);
		for (i = 2; ++i<max; close(i));
	}
# endif /* DISABLE_DAEMONIZE */



	/* Run app */
	{
		if (daemonize>2) {
			int fd = open("/dev/null", O_RDWR);
			if (fd==-1 || dup2(fd, 0)==-1 ||
				dup2(fd, 1)==-1 || dup2(fd, 2)==-1) return 1;
		} else {
			int fd = open("/dev/null", O_WRONLY);
			if (fd==-1 || dup2(fd, 1)==-1 || dup2(fd, 2)==-1) return 1;
		}

		return execvp(argv[1], argv + 1);
	}
#endif /* DISABLE_EXEC */
}
