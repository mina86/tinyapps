/*
 * Prints window's ClassHint
 * Copyright 2005,2006 by Michal Nazarewicz <mina86@mina86.com>
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
