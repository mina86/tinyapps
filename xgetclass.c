/*
 * Prints window's ClassHint
 * $Id: xgetclass.c,v 1.3 2006/09/28 15:06:18 mina86 Exp $
 * Copyright 2005 by Michal Nazarewicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
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
	/* Get executable name */
	char *MyName = strrchr(argv[0], '/');
	MyName = MyName==NULL ? argv[0] : (MyName + 1);

	/* Invalid number of arguments */
	if (argc!=2) {
		fprintf(stderr, "usage: %s <wnd-id>\n", MyName);
		return 1;
	}

	/* Parse win id */
	char *c;
	unsigned long int wnd = strtoul(argv[1], &c, 0);
	if (*c) {
		fprintf(stderr, "usage: %s <wnd-id>\n", MyName);
		return 1;
	}

	/* Open display */
	c = XDisplayName(NULL);
	Display *display = XOpenDisplay(c);
	if (!display) {
		fprintf(stderr, "%s: can't open display %s\n", MyName, c);
		return 1;
	}

	/* Get class */
	XClassHint class = {0, 0};
	XGetClassHint(display, wnd, &class);

	/* Print */
	printf("%s\n%s\n", class.res_name, class.res_class);
	XFree(class.res_name);
	XFree(class.res_class);
	return 0;
}
