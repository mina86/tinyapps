/*
 * Discards standard input.
 * $Id: null.c,v 1.3 2005/09/03 13:27:05 mina86 Exp $
 * By Michal Nazarewicz (mina86/AT/tlen.pl)
 * Released to Public Domain
 */

/*
 * You can run it as: `something | null` in which case stdou of
 * `something` will de disgarded or as `null somtehing` in which case
 * stdout and stderr will be redirected to /dev/null.  The leter
 * option is called 'exec feature'.
 */


/******** Config ********/
#define HAVE_UNISTD_H /* Comment this line if you have problems compiling */
                      /* Note, however, that you won't be able to use exec
                         feature of null */
/*#define DISABLE_EXEC  /* Uncomment this if you do not want exec feature */
#define BUFFER_SIZE 1024


/******** Include ********/
#ifdef HAVE_UNISTD_H
# include <unistd.h>
# ifndef DISABLE_EXEC
#  include <fcntl.h>
# endif
#else
# include <stdio.h>
# ifndef DISABLE_EXEC
#  define DISABLE_EXEC
# endif
#endif


/******** Main ********/
#ifdef DISABLE_EXEC
int main(void) {
#else
int main(int argc, char **argv) {
	if (argc==1) {
#endif
		char buf[BUFFER_SIZE];
#ifdef HAVE_UNISTD_H
		while (read(0, buf, sizeof(char) * 1024));
#else
		while (fread(buf, sizeof(char), 1024, stdin));
#endif
		return 0;
#ifndef DISABLE_EXEC
	}

	int fd = open("/dev/null", O_RDONLY);
	if (fd==-1 || dup2(fd, 1)==-1 || dup2(fd, 2)==-1) return 1;
	return execvp(argv[1], argv + 1);
#endif
}
