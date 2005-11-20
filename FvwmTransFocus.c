/*
 * FVWM module changing opacity depending on focus.
 * $Id: FvwmTransFocus.c,v 1.3 2005/11/20 20:31:45 mina86 Exp $
 * Copyright 2005 by Michal Nazarewicz (mina86/AT/tlen.pl)
 * Some code from transset by Matthew Hawn
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
 * Compilation without debug messages:
 *     <CC> -O2 $CFLAGS -I<X11_INC_DIR> -L<X11_LIB_DIR> -lX11 \
 *         -o FvwmTransFocus FvwmTransFocus.c
 * Compilation withh debug messages:
 *     <CC> -O2 $CFLAGS -I<X11_INC_DIR> -L<X11_LIB_DIR> -lX11 -DDEBUG \
 *         -o FvwmTransFocus FvwmTransFocus.c
 * Where:
 *     <CC>          - C Compiler (eg. gcc)
 *     <X11_INC_DIR> - Path to X11's header files (eg. /usr/X11R6/include)
 *     <X11_LIB_DIR> - Path to X11's libraries (eg. /usr/X11R6/lib)
 */

/*
 * Invocation:
 *     Module FvwmTransFocus [ <active> [ <inactive> [ <new> ]]]
 * Where:
 *     <active>   - opacity of active windows (default: 0.8)
 *     <inactive> - opacity of inactive windows (default: 0.6)
 *     <new>      - opacity of newly created windows (default:  <inactive>)
 */

/*
 * If you want some windows to be excluded from setting translucency
 * hack the ignoreWindow() function.  It has a simple test making it
 * sking tvtime and xawtv windows - add your own as you please.  In
 * the future, it will be configurable through a FVWM config file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>



/********** Debug **********/
#ifdef DEBUG
#define debug(format, ...) fprintf(stderr, "%s: " format "\n", MyName, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

#define MIN(x, y)  ((x),(y)?(x):(y))



/********** Defines **********/
#define DONT_USE_MX_PROPERTY_CHANGE  // PROPERTY_CHANGE doesn't seem to work :(

#define OPACITY "_NET_WM_WINDOW_OPACITY"
#define OPAQUE  0xFFFFFFFF

#define M_FOCUS_CHANGE            (1 << 6)
#define M_ADD_WINDOW              (1 << 29)
#define M_EXTENDED_MSG            (1<<31)
#define MX_PROPERTY_CHANGE        ((1<<3) | M_EXTENDED_MSG)

#define MX_PROPERTY_CHANGE_BACKGROUND  1

#define START_FLAG 0xffffffff



/********** Typedefs **********/
typedef struct {
	unsigned long start_pattern;
	unsigned long body[1];
} FvwmPacket;



/********** Global variables **********/
static Display *display;
static int fvin, fvout;
static char *MyName;



/********** Functions **********/
static void send(const char *message);
static unsigned long readLong();
static void readLongs(char *buffer, int count);
static void transSet(unsigned long int window, double value);
static int ignoreWindow(unsigned long int window);
static int ErrorHandler(Display *display, XErrorEvent *error);



/********** Main **********/
int main (int argc, char **argv) {
	/* Get executable name */
	MyName = strrchr(argv[0], '/');
	MyName = MyName==NULL ? argv[0] : (MyName + 1);
	debug("Starting");

	/* Check args */
	if (argc<6) {
		fprintf(stderr, "%s: should only be executed by fvwm!\n", MyName);
		return 1;
	}

	/* Open display */
	char *display_name = XDisplayName(NULL);
	if (!(display = XOpenDisplay(display_name))) {
		fprintf(stderr, "%s: can't open display %s", MyName, display_name);
		return 1;
	}

	/* Parse args */
	fvout = atoi(argv[1]);
	fvin = atoi(argv[2]);

	double values[3];
	values[0] = argc>6 ? atof(argv[6]) : 0.80;
	values[1] = argc>7 ? atof(argv[7]) : 0.60;
	values[2] = argc>8 ? atof(argv[8]) : values[1];


	/* Init module */
	debug("Initializing");
	char set_mask_mesg[50];
	sprintf(set_mask_mesg, "SET_MASK %lu", M_FOCUS_CHANGE | M_ADD_WINDOW);
	send(set_mask_mesg);
#ifndef DONT_USE_MX_PROPERTY_CHANGE
	sprintf(set_mask_mesg, "SET_MASK %lu",
			M_EXTENDED_MSG | MX_PROPERTY_CHANGE_BACKGROUND);
	send(set_mask_mesg);
#endif
	send("NOP FINISHED STARTUP");

	/* Set error handler */
	XSetErrorHandler(&ErrorHandler);


	/* Init loop */
	unsigned long buffer[252];
	unsigned long type, size;
	unsigned long prev_id = 0;

	/* Loop */
	debug("Loop begins");
	for(;;){
		while (readLong()!=START_FLAG);               /* Wait for START_FLAG */
		type = readLong();                            /* Type */
		size = readLong() - 4;                        /* Size */
		readLong();                                   /* Ignore Timestamp */
		readLongs((char *)(&buffer), MIN(size, 252)); /* Body */

		switch (type) {
			/* M_FOCUS_CHANGE */
		case M_FOCUS_CHANGE:
			debug("M_FOCUS_CHANGE recieved");
			transSet(prev_id, values[1]);
			prev_id = buffer[1];
			transSet(prev_id, values[0]);
			XSync(display, False);
			break;

			/* M_ADD_WINDOW */
		case M_ADD_WINDOW:
			debug("M_ADD_WINDOW recieved");
			transSet(buffer[1], values[2]);
			XSync(display, False);
			break;

#ifndef DONT_USE_MX_PROPERTY_CHANGE
			/* MX_PROPERTY_CHANGE */
		case MX_PROPERTY_CHANGE:
			if (buffer[0]==MX_PROPERTY_CHANGE_BACKGROUND) {
				debug("MX_PROPERTY_CHANGE_BACKGROUNDR recieved");
				send("All (!Iconic) RefreshWindow\n");
			}
			break;
#endif
		}
	}
}



/******** Sends message to FVWM *********/
void send(const char *message) {
	unsigned long l = 0;
	write(fvout, &l, sizeof(l));
	l = strlen(message);
	write(fvout, &l, sizeof(l));
	write(fvout, message, l);
	l = 1;
	write(fvout, &l, sizeof(l));
}


/********** Reads a single unsigned long from FVWM **********/
unsigned long readLong() {
	static unsigned long buf;
	readLongs((char*)&buf, 1);
	return buf;
}


/********** Reads specified number of unsigned longs from FVWM **********/
void readLongs(char *buf, int count) {
	int n;
	for (count *= sizeof(unsigned long); count>0; count -= n) {
		if ((n = read(fvin, buf, count))<=0) {
			debug("Stopping");
			XCloseDisplay(display);
			exit(0);
		}
		buf += n;
	}
}



/********** Sets translucency **********/
void transSet(unsigned long int window, double value) {
	if (ignoreWindow(window)) {
		return;
	}
	debug("Changing 0x%08lx's opacity to %lf", window, value);

	if (value>=1) {
		XDeleteProperty(display, window, XInternAtom(display, OPACITY, False));
	} else {
		unsigned long opacity = value<=0 ? 0 : OPAQUE * value;
		XChangeProperty(display, window, XInternAtom(display, OPACITY, False),
						XA_CARDINAL, 32, PropModeReplace,
						(unsigned char *) &opacity, 1L);
	}
}



/* Checks whether the window should be ignored */
static int ignoreWindow(unsigned long int window) {
	/* Root window or no window */
	if (!window) {
		return 1;
	}

	/* Get class */
	/* FVWM gives us a ID of a parent window which does not have a
	 * ClassHint so we need to find the child with ClassHint.  At least
	 * that's what it looks like :) */
	XClassHint class = {0, 0};
	unsigned long int ignore, *children = NULL;
	unsigned int num;
	while (!XGetClassHint(display, window, &class)) {
		if (!XQueryTree(display, window, &ignore, &ignore, &children, &num)) {
			return 0;
		} else if (num!=1) {
			XFree((char*)children);
			return 0;
		} else {
			window = children[0];
			XFree((char*)children);
		}
	}

	/* Ignore apps for watching TV */
	num = !strncasecmp(class.res_class, "xawtv", 5)
		|| !strncasecmp(class.res_class, "tvtime", 5);

	/* Return */
	debug("ClassHint: '%s'; '%s'"%s, class.res_name, class.res_class,
		  num ? " (skipping)" : "");
	XFree(class.res_name);
	XFree(class.res_class);
	return num;
}



/********** Error handler ********/
int ErrorHandler(Display *display, XErrorEvent *error) {
	fprintf(stderr, "%s: X Error: #%d; Request: %d, %d; Id: 0x%lx\n", MyName,
			error->error_code, error->request_code, error->minor_code,
			error->resourceid);  /* error->resourceid may be uninitialised */
	return 0;
}
