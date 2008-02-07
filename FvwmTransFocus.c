/*
 * FVWM module changing opacity depending on focus.
 * $Id: FvwmTransFocus.c,v 1.10 2008/02/07 19:28:38 mina86 Exp $
 * Copyright 2005-2008 by Michal Nazarewicz (mina86/AT/mina86.com)
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
 * Compilation with debug messages:
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
#include <strings.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>



/********** Debug **********/
#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "%s: " fmt "\n", MyName, __VA_ARGS__)
#define debugs(string) fprintf(stderr, "%s: %s", MyName, (string))
#else
#define debug(fmt, ...)
#define debugs(string)
#endif



/********** Defines **********/
#define DONT_USE_MX_PROPERTY_CHANGE /* PROPERTY_CHANGE doesn't seem to work */

#define OPACITY "_NET_WM_WINDOW_OPACITY"
#define OPAQUE  0xFFFFFFFF

#define M_FOCUS_CHANGE            (1 << 6)
#define M_ADD_WINDOW              (1 << 29)
#define M_EXTENDED_MSG            (1<<31)
#define MX_PROPERTY_CHANGE        ((1<<3) | M_EXTENDED_MSG)

#define MX_PROPERTY_CHANGE_BACKGROUND  1

#define START_FLAG 0xffffffff


#if __STDC_VERSION__ < 199901L
#  if defined __GNUC__
#    define inline   __inline__
#  else
#    define inline
#  endif
#endif




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
static inline unsigned long readLong(void);
static void readLongs(char *buffer, int count);
static void transSet(unsigned long int window, double value);
static int ignoreWindow(unsigned long int window);
static int ErrorHandler(Display *dsp, XErrorEvent *error);



/********** Main **********/
int main (int argc, char **argv) {
	char *display_name, set_mask_mesg[50];
	unsigned long prev_id = 0;
	double values[3];

	/* Get executable name */
	MyName = strrchr(argv[0], '/');
	MyName = MyName && MyName[1] ? MyName + 1 : *argv;
	debugs("Starting");

	/* Check args */
	if (argc<6) {
		fprintf(stderr, "%s: should only be executed by fvwm!\n", MyName);
		return 1;
	}

	/* Open display */
	display_name = XDisplayName(0);
	if (!(display = XOpenDisplay(display_name))) {
		fprintf(stderr, "%s: can't open display %s", MyName, display_name);
		return 1;
	}

	/* Parse args */
	fvout = atoi(argv[1]);
	fvin = atoi(argv[2]);

	values[0] = argc>6 ? atof(argv[6]) : 0.80;
	values[1] = argc>7 ? atof(argv[7]) : 0.60;
	values[2] = argc>8 ? atof(argv[8]) : values[1];


	/* Init module */
	debugs("Initializing");
	sprintf(set_mask_mesg, "SET_MASK %lu",
	        (unsigned long)M_FOCUS_CHANGE | M_ADD_WINDOW);
	send(set_mask_mesg);
#ifndef DONT_USE_MX_PROPERTY_CHANGE
	sprintf(set_mask_mesg, "SET_MASK %lu",
			(unsigned long)M_EXTENDED_MSG | MX_PROPERTY_CHANGE_BACKGROUND);
	send(set_mask_mesg);
#endif
	send("NOP FINISHED STARTUP");

	/* Set error handler */
	XSetErrorHandler(&ErrorHandler);


	/* Loop */
	debugs("Loop begins");
	for(;;){
		unsigned long buffer[252], type, size;

		while (readLong()!=START_FLAG);              /* Wait for START_FLAG */
		type = readLong();                           /* Type */
		size = readLong() - 4;                       /* Size */
		if (size > sizeof buffer / sizeof *buffer) {
			size = sizeof buffer / sizeof *buffer;
		}
		readLong();                                  /* Ignore Timestamp */
		readLongs((char *)(&buffer), size);          /* Body */

		switch (type) {
			/* M_FOCUS_CHANGE */
		case M_FOCUS_CHANGE:
			debugs("M_FOCUS_CHANGE recieved");
			if (prev_id!=buffer[1]) {
				transSet(prev_id, values[1]);
				prev_id = buffer[1];
				transSet(prev_id, values[0]);
				XSync(display, False);
			}
			break;

			/* M_ADD_WINDOW */
		case M_ADD_WINDOW:
			debugs("M_ADD_WINDOW recieved");
			transSet(buffer[1], values[2]);
			XSync(display, False);
			break;

#ifndef DONT_USE_MX_PROPERTY_CHANGE
			/* MX_PROPERTY_CHANGE */
		case MX_PROPERTY_CHANGE:
			if (buffer[0]==MX_PROPERTY_CHANGE_BACKGROUND) {
				debugs("MX_PROPERTY_CHANGE_BACKGROUNDR recieved");
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
static inline unsigned long readLong(void) {
	static unsigned long buf;
	readLongs((char*)&buf, 1);
	return buf;
}


/********** Reads specified number of unsigned longs from FVWM **********/
void readLongs(char *buf, int count) {
	int n;
	for (count *= sizeof(unsigned long); count>0; count -= n) {
		if ((n = read(fvin, buf, count))<=0) {
			debugs("Stopping");
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
	XClassHint class = {0, 0};
	unsigned long int ignore, *children = 0;
	unsigned int num;

	/* Root window or no window */
	if (!window) {
		return 1;
	}

	/* Get class */
	/* FVWM gives us a ID of a parent window which does not have a
	 * ClassHint so we need to find the child with ClassHint.  At least
	 * that's what it looks like :) */
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
	debug("ClassHint: '%s'; '%s'%s", class.res_name, class.res_class,
	      num ? " (skipping)" : "");
	XFree(class.res_name);
	XFree(class.res_class);
	return num;
}



/********** Error handler ********/
int ErrorHandler(Display *dsp, XErrorEvent *error) {
	(void)dsp; (void)error;
	debug("X Error: #%d; Request: %d, %d; Id: 0x%lx\n",
	      error->error_code, error->request_code, error->minor_code,
	      error->resourceid);  /* error->resourceid may be uninitialised */
	return 0;
}
