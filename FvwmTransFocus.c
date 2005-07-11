/*
 * FVWM module changing opacity depending on focus.
 * $Id: FvwmTransFocus.c,v 1.2 2005/07/11 00:20:57 mina86 Exp $
 * Copyright 2005 by Michal Nazarewicz (mina86/AT/tlen.pl)
 * Licensed under the Academic Free License version 2.1.
 * Based on transset by Matthew Hawn
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>


/*** Debug ***/
#ifdef DEBUG
#define debug(format, ...) fprintf(stderr, "%s: " format "\n", MyName, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

#define MIN(x, y)  ((x),(y)?(x):(y))


/*** Defines ***/
#define DONT_USE_MX_PROPERTY_CHANGE  // PROPERTY_CHANGE doesn't seem to work :(

#define OPACITY "_NET_WM_WINDOW_OPACITY"
#define OPAQUE  0xFFFFFFFF

#define M_FOCUS_CHANGE            (1 << 6)
#define M_ADD_WINDOW              (1 << 29)
#define M_EXTENDED_MSG            (1<<31)
#define MX_PROPERTY_CHANGE        ((1<<3) | M_EXTENDED_MSG)

#define MX_PROPERTY_CHANGE_BACKGROUND  1

#define START_FLAG 0xffffffff


/*** Typedefs ***/
typedef struct {
	unsigned long start_pattern;
	unsigned long body[1];
} FvwmPacket;


/*** Global variables ***/
static Display *display;
static int fvin, fvout;
static char *MyName;


/*** Functions ***/
static void send(const char *message);
static unsigned long readLong();
static void readLongs(char *buffer, int count);
static void transSet(unsigned long int id, double value);
static int ErrorHandler(Display *display, XErrorEvent *event);


/*** Main ***/
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


void send(const char *message) {
	unsigned long l = 0;
	write(fvout, &l, sizeof(l));
	l = strlen(message);
	write(fvout, &l, sizeof(l));
	write(fvout, message, l);
	l = 1;
	write(fvout, &l, sizeof(l));
}


unsigned long readLong() {
	static unsigned long buf;
	readLongs((char*)&buf, 1);
	return buf;
}


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



void transSet(unsigned long window, double value) {
	if (!window) {
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


int ErrorHandler(Display *display, XErrorEvent *event) {
	/* Ignore all errors */
}
