/*
 * Prints window's ClassHint
 * $Id: xgetclass.c,v 1.1 2005/11/20 20:18:13 mina86 Exp $
 * Copyright 2005 by Michal Nazarewicz (mina86/AT/tlen.pl)
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
