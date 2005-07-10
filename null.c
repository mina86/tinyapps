/*
 * Discards standard input
 * $Id: null.c,v 1.1 2005/07/10 18:06:59 mina86 Exp $
 * By Michal Nazarewicz (mn86/AT/o2.pl)
 * Released to Public Domain
 */

#define HAVE_UNISTD_H /* Comment this line if you have problems compiling */
#define BUFFER_SIZE 1024

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
# include <stdio.h>
#endif

int main(void) {
	int buf[BUFFER_SIZE];
#ifdef HAVE_UNISTD_H
	while (read(0, buf, sizeof(char) * 1024));
#else
	while (fread(buf, sizeof(char), 1024, stdin));
#endif
	return 0;
}
