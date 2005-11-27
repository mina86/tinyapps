/*
 * Discards standard input.
 * $Id: null.c,v 1.4 2005/11/27 16:57:38 mina86 Exp $
 * By Michal Nazarewicz (mina86/AT/tlen.pl)
 * Released to Public Domain
 */

/*
 * usage:
 *   command | null         (pipe feature)
 *   null    command        (exec feature)
 *   null -d command        (exec + daemonize feature)
 *   null -D command        (exec + daemonize feature and PID written)
 *   drun    command        (synonym of  null -d command )
 *   drun -D command        (synonym of  null -D command )
 *
 * Pipe feature is used to discard stdout of a command.  Exec feature
 * discards both - stdout and stderr.  If -d or -D is given or it's
 * run as drun (daemon run) the command will be put into background.
 * Each of the features can be disabled by editing the defines in the
 * source file.  Running it as xrun has the same effect as running it
 * as drun.
 */


/******** Config ********/

/* Comment this out if you have problems compiling (may happen on non
   POSIX systems).  Note, that exec and daemonize features require
   unistd.h */
#define HAVE_UNISTD_H

/* Comment this out if you want an exec feature to be compiled it. */
/* #define DISABLE_EXEC /**/

/* Comment this out if you want an daemonize feature to be compiled
  in.  Note, that deamonize feature works only if exec feature is
  compiled in.
/* #define DISABLE_DAEMONIZE /**/

/* Comment this out if you want the app to take -d and -D arguments.
   This takes effect only if daemonize feature is turned on. */
/* #define DISABLE_ARGS /**/

/* Comment this out if you do not want the app to assume it was run as
   drun and daemonize by default.  This requires daemonize to be
   compiled.  Moreover, it will disable a pipe feature.  */
/* #define BUILD_DRUN /**/

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
# include <unistd.h>
# ifndef DISABLE_EXEC
#  include <fcntl.h>
# endif
#else
# include <stdio.h>
#endif



/******** Main ********/
#ifdef DISABLE_EXEC
int main(void) {
# define argc 1
#else
int main(int argc, char **argv) {
#endif


	/* No args given or exec feature disabled */
	if (argc==1) {
#ifdef BUILD_DRUN
		write(2, "usage: drun [-D] command\n", 25);
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
	int daemonize = 0;

#  ifdef BUILD_DRUN
	daemonize = 1;
#  else
	/* Check whether was run as drun */
	char *c, *s = argv[0];
	for (c = argv[0]; *c; ++c) {
		if (*c=='/') {
			s = c+1;
		}
	}
	if ((s[0]=='d' || s[0]=='x')
		&& s[1]=='r' && s[2]=='u' && s[3]=='n' && !s[4]) {
		daemonize = 1;
	}
#  endif

#  ifndef DISABLE_ARGS
	/* Check whether -d or -D was given */
	if (argc>2 && argv[1][0]=='-' && !argv[1][2]) {
		if (argv[1][1]=='D') {
			daemonize = 2;
			++argv;
#   ifndef BUILD_DRUN
		} else if (argv[1][1]=='d') {
			daemonize = 1;
			++argv;
#   endif
		}
	}
#  endif


	/* Daemonize */
	if (daemonize) {
		pid_t pid = fork();
		switch (pid) {
		case -1: return 2;
		case  0: break;
		default:
			if (daemonize!=2) return 0;
			char buf[32]; buf[31] = '\n';
			int i = 31;
			do { buf[--i] = '0' + (pid%10); } while ((pid/=10)!=0);
			write(1, buf+i, 32 - i);
			return 0;
		}
	}
# endif



	/* Run app */
	int fd = open("/dev/null", O_RDONLY);
	if (fd==-1 || dup2(fd, 1)==-1 || dup2(fd, 2)==-1) return 1;
	return execvp(argv[1], argv + 1);
#endif
}
