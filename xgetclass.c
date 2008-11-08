/*
 * Prints window's ClassHint
 * $Id: xgetclass.c,v 1.6 2008/11/08 23:45:57 mina86 Exp $
 * Copyright 2005,2006 by Michal Nazarewicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
 *
 * This is part of Tiny Applications Collection
 *   -> http://tinyapps.sourceforge.net/
 */

/*
 * Compilation:
 *     <CC> -O2 $CFLAGS -I<X11_INC_DIR> -L<X11_LIB_DIR> -lX11 \
 *         -o xgetclass xgetclass.c
 * Where:
 *     <CC>          - C Compiler (eg. gcc)
 *     <X11_INC_DIR> - Path to X11's header files (eg. /usr/X11R6/include)
 *     <X11_LIB_DIR> - Path to X11's libraries (eg. /usr/X11R6/lib)
 */

/*
 * Invocation: xgetclass <wnd-id>
 * Result:     Prints aplication name and class (in separate lines) of
 *             window with ID given as an argument.
 */


#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


int main(int argc, char **argv) {
	char *c;
	unsigned long wnd;
	Display *display;
	XClassHint class;

	/* Get executable name */
	for (c = *argv; *c; ++c) {
		if (*c=='/' && c[1]) {
			*argv = ++c;
		}
	}

	/* Invalid number of arguments */
	if (argc!=2) {
		fprintf(stderr, "usage: %s <wnd-id>\n", *argv);
		return 1;
	}

	/* Parse win id */
	wnd = strtoul(argv[1], &c, 0);
	if (*c) {
		fprintf(stderr, "usage: %s <wnd-id>\n", *argv);
		return 1;
	}

	/* Open display */
	c = XDisplayName(0);
	if (!(display = XOpenDisplay(c))) {
		fprintf(stderr, "%s: can't open display %s\n", *argv, c);
		return 1;
	}

	/* Get class */
	XGetClassHint(display, wnd, &class);

	/* Print */
	printf("%s\n%s\n", class.res_name, class.res_class);
	XFree(class.res_name);
	XFree(class.res_class);
	return 0;
}
