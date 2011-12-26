/*
 * Prints song MPD's curently playing.
 * Copyright 2005-2011 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This software is OSI Certified Open Source Software.
 * OSI Certified is a certification mark of the Open Source Initiative.
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


#define APP_VERSION "0.19"

#define HAVE_ICONV 1

#define _POSIX_C_SOURCE 2
#define _BSD_SOURCE

#include <errno.h>
#include <stdint.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <locale.h>

#if HAVE_ICONV
#  include <iconv.h>
#  include <langinfo.h>
#endif

#include "libmpdclient.h"

#ifdef SIGWINCH
#  include <termios.h>
#  ifndef TIOCGWINSZ
#    include <sys/ioctl.h>
#  endif
#endif

#if defined SIGWINCH && defined TIOCGWINSZ
#  define HAVE_RESIZE 1
#else
#  define HAVE_RESIZE 0
#  warn No SIGWINCH support.
#endif


/******************** Misc **************************************************/
#if __GNUC__ <= 2
#  define __attribute__(x)
#endif

const char *argv0 = "mpd-show";

static void _die(int perr, const char *fmt, ...)
	__attribute__((noreturn, format(printf, 2, 3), cold));
#define die_on(cond, ...)  do { if (cond) _die(0, __VA_ARGS__); } while (0)
#define pdie_on(cond, ...) do { if (cond) _die(1, __VA_ARGS__); } while (0)


/********** Defaults **********/
#define DEFAULT_HOST    "localhost"
#define DEFAULT_PORT    6600
#define DEFAULT_COLUMNS 80
#define DEFAULT_FORMAT \
	"[[[%artist% <&%album%> ]|%artist% - |<%album%> ]" \
	"&[[%track%. &%title%]|%title%]]"  \
	"|[%track%. &%title%]|%title%|%filenoext%"


/******************** Terminal **********************************************/
static void termInit(void);
static void termBeginLine(void);
static void termNormal(void);
static void termEndLine(void);


/******************** Main functions ****************************************/
static void parseArguments(int argc, char **argv)
	__attribute__((nonnull));
static void daemonize(void);
static void registerSignalHandlers(void);

static int  connectToMPD(unsigned timeout);
static void disconnectFromMPD(void);
static int  getSong(void);
static void display(unsigned secs);

static void initCodesets(void);

static int  done(void);
static int  error(void) __attribute__((pure));
static unsigned columns(void) __attribute__((pure));


int main(int argc, char **argv) {
	unsigned timeout = 1, connected = 0;

	parseArguments(argc, argv);
	daemonize();
	registerSignalHandlers();
	termInit();
	atexit(disconnectFromMPD);
	initCodesets();

	do {
		if (!connected) {
			timeout = timeout <= 30 ? timeout * 2 : 60;
			connected = connectToMPD(timeout > 10 ? 10 : timeout);
		}

		if (connected) {
			timeout = 1;
			connected = getSong();
		}

		display(timeout);
	} while (!done());

	return 0;
}


/******************** Global data *******************************************/
struct {
	mpd_Connection *conn;
	mpd_InfoEntity *info;

	struct state {
		int error;
		int state;
		int songid;
		int pos;
		int len;
		unsigned hilightPos;
		unsigned scroll;
#if HAVE_RESIZE
		unsigned columns;
#endif
	} cur, old;
#if !HAVE_RESIZE
	unsigned columns;
#endif

	const wchar_t *format;

	wchar_t *line;
	size_t line_len, line_capacity;

	const char *host;
	const char *password;
	unsigned short port;

	unsigned short background;

	volatile sig_atomic_t gotSignal;
#if HAVE_RESIZE
	volatile sig_atomic_t width;
#endif
} D;

static int  done(void) {
	return D.gotSignal;
}

static int  error(void) {
	return !!(D.conn->error);
}

static unsigned columns(void) {
#if HAVE_RESIZE
	return D.cur.columns;
#else
	return D.columns;
#endif
}

static void setColumns(unsigned c) {
#if HAVE_RESIZE
	D.cur.columns = c;
#else
	D.columns = c;
#endif
}


/******************** Initialisation ****************************************/
static void handleSignal(int sig) __attribute__((cold));
static void handleResize(int sig) __attribute__((cold));

static const wchar_t *wideFromMulti(const char *str) __attribute__((nonnull));

static void usage(void) __attribute__((noreturn));
static void usage(void)
{
	printf("mpd-show  " APP_VERSION " (c) 2005-2011 by Michal Nazarewicz (mina86/AT/mina86.com)\n"
	       "usage: %s [ <options> ] [ <host>[:<port> | <port> ]]\n"
	       " -b -B   runs in background mode; -B also forks into background\n"
	       " -c<col> assumes <col>-char wide term [$COLUMNS or %d]"
#if HAVE_RESIZE
	       " (obsolete)"
#endif
	       "\n"
	       " -f<fmt> uses <fmt> for displaying song (see mpc(1)); supports following tags:\n",
	       argv0, DEFAULT_COLUMNS);
	printf("         album, artist, comment, composer, date, dir, disc, file, filenoext,\n"
	       "         genre, name, path, pathnoext, time, title, track and Y pseudo tag.\n"
	       " <host>  host MPD is listening on optionally prefixed with '<password>@'\n"
	       "                                  [$MPD_HOST or " DEFAULT_HOST "]\n"
	       " <port>  port MPD is listening on [$MPD_PORT or %u]\n",
	       DEFAULT_PORT);
	exit(0);
}


static void parseArguments(int argc, char **argv) {
	const char *hostarg = 0, *portarg = 0, *format;
	char *end;
	int opt;

	/* Program name */
	argv0 = strrchr(argv[0], '/');
	argv0 = argv0 && argv0[1] ? argv0 + 1 : *argv;


	/* Help */
	if (argc > 1 && !strcmp(argv[1], "--help")) {
		usage();
	}

	/* Some defaults */
	format = DEFAULT_FORMAT;

	/* Get opts */
	while ((opt = getopt(argc, argv, "-hbBc:f:"))!=-1) {
		switch (opt) {
		case 'h': usage();
		case 'b': D.background = 1; break;
		case 'B': D.background = 2; break;

			/* Columns */
		case 'c': {
			unsigned long c = strtoul(optarg, &end, 0);
			die_on(c < 3 || c > UINT_MAX || *end,
				   "invalid terminal width: %s", optarg);
			setColumns(c);
			break;
		}

			/* Format */
		case 'f':
			format = optarg;
			break;

			/* An argument */
		case 1:
			if (!hostarg) { hostarg = optarg; break; }
			if (!portarg) { portarg = optarg; break; }
			die_on(1, "invalid argument: %s", optarg);

			/* An error */
		default:
			die_on(1, "invalid option: %c", optopt);
		}
	}


	/* Host, password and port */
	if (!hostarg) {
		hostarg = getenv("MPD_HOST");
	}
	if (!hostarg) {
		hostarg = DEFAULT_HOST;
	}
	end = strchr(hostarg, '@');
	if (end) {
		/* If '@' has been found we got host from either command line
		 * or environment and in both cases the string is
		 * modifiable. */
		*end = 0;
		D.password = hostarg;
		hostarg = end + 1;
	}
	D.host = hostarg;

	if (!portarg) {
		portarg = getenv("MPD_PORT");
	}
	if (!portarg) {
		end = strchr(hostarg, ':');
		if (end) {
			*end = 0;
			portarg = end + 1;
		}
	}
	if (!portarg) {
		D.port = DEFAULT_PORT;
	} else {
		unsigned long p = strtoul(portarg, &end, 0);
		die_on(p <= 0 || p > 0xffff || *end, "invalid port: %s", portarg);
		D.port = p;
	}


	/* Format */
	D.format = wideFromMulti(format);


	/* Columns */
#if HAVE_RESIZE
	if (!columns()) {
		handleResize(SIGWINCH);
		setColumns(D.width);
	}
#endif
	if (!columns()) {
		end = getenv("COLUMNS");
		if (end) {
			unsigned long c = strtoul(end, &end, 0);
			if (c >= 3 && c <= UINT_MAX && !*end) {
				setColumns(c);
			}
		}
	}
	if (!columns()) {
		setColumns(DEFAULT_COLUMNS);
	}
#if HAVE_RESIZE
	D.width = D.cur.columns;
#endif
}


static void daemonize(void) {
	if (D.background > 1) {
		pid_t pid = fork();
		pdie_on(pid < 0, "fork");
		if (pid) {
			printf("[%ld]\n", (long)pid);
			_exit(0);
		}
	}
}


static void registerSignalHandlers(void) {
#if HAVE_RESIZE
	signal(SIGWINCH, handleResize);
#endif
#ifdef SIGHUP
	signal(SIGHUP  , handleSignal);
#endif
#ifdef SIGINT
	signal(SIGINT  , handleSignal);
#endif
#ifdef SIGQUIT
	signal(SIGQUIT , handleSignal);
#endif
#ifdef SIGTERM
	signal(SIGTERM , handleSignal);
#endif
}


static void handleSignal(int sig) {
	if (D.gotSignal) {
		exit(1);
	}
	D.gotSignal = 1;
	signal(sig, handleSignal);
}

#if HAVE_RESIZE
static void handleResize(int sig) {
	struct winsize size;
	if (ioctl(1, TIOCGWINSZ, &size) >= 0 && size.ws_col) {
		D.width = size.ws_col;
	}
	signal(sig, handleResize);
}
#endif


/******************** Connection handling ***********************************/
static int  connectToMPD(unsigned timeout) {
	disconnectFromMPD();

	D.conn = mpd_newConnection(D.host, D.port, timeout);
	pdie_on(!D.conn, "connect");

	if (D.password && !error()) {
		mpd_sendPasswordCommand(D.conn, D.password);
		if (!error()) {
			mpd_finishCommand(D.conn);
		}
	}

	return !error();
}

static void disconnectFromMPD(void) {
	if (D.conn) {
		mpd_closeConnection(D.conn);
		D.conn = 0;
	}
	if (D.info) {
		mpd_freeInfoEntity(D.info);
		D.info = 0;
	}
}


/******************** Song ingo retrival ************************************/
static int  getSong(void) {
	mpd_Status *status;
	mpd_InfoEntity *info;

	/* Get status */
	mpd_sendStatusCommand(D.conn);
	if (error()) return 0;
	status = mpd_getStatus(D.conn);
	if (error()) return 0;
	pdie_on(!status, "get status");

	mpd_nextListOkCommand(D.conn);
	if (error()) {
		mpd_freeStatus(status);
		return 0;
	}

	/* Copy status */
	D.cur.state  = status->state;
	D.cur.songid = status->songid;
	D.cur.pos    = status->elapsedTime;
	D.cur.len    = status->totalTime;
	mpd_freeStatus(status);

	/* Same song, return */
	if (D.cur.songid == D.old.songid) {
		return 1;
	}

	if (D.info) {
		mpd_freeInfoEntity(D.info);
		D.info = 0;
	}

	/* Get song */
	mpd_sendCurrentSongCommand(D.conn);
	if (error()) {
		return 0;
	}
	info = mpd_getNextInfoEntity(D.conn);
	if (error()) {
		return 0;
	}

	if (!info) {
		return
			D.cur.state == MPD_STATUS_STATE_PLAY ||
			D.cur.state == MPD_STATUS_STATE_PAUSE;
	}

	if (info->type != MPD_INFO_ENTITY_TYPE_SONG) {
		mpd_freeInfoEntity(info);
		return 0;
	}

	D.info = info;
	return 1;
}


/******************** Displaying data ***************************************/
static void formatLine(void);
static void calculateHilightPos(void);
static void output(void);


static void display(unsigned secs) {
	static int first_time = 1;

	do {
		int doDisplay = 0;

		D.cur.error   = D.conn->error;
#if HAVE_RESIZE
		D.cur.columns = D.width;
#endif

#define CHANGED(field) (D.cur.field != D.old.field)

		if (first_time || CHANGED(error) || CHANGED(songid)) {
			first_time = 0;
			formatLine();
			doDisplay = 1;
		}

#if HAVE_RESIZE
		doDisplay = doDisplay || CHANGED(columns);
#endif
		if (doDisplay || CHANGED(pos) || CHANGED(len)) {
			calculateHilightPos();
			doDisplay = doDisplay || CHANGED(hilightPos);
		}

		doDisplay = doDisplay || CHANGED(state) || CHANGED(scroll);

#undef CHANGED

		D.old = D.cur;
		if (doDisplay || D.background) {
			output();
		}
		usleep(1000000);
	} while (!done() && --secs);
}



static void calculateHilightPos(void) {
	if (!D.cur.error) {
		D.cur.hilightPos = D.cur.len
			? (wchar_t)D.cur.pos * columns() / D.cur.len
			: (unsigned)0;
	}
}


/******************** Outputting data ***************************************/
static unsigned hilightLeft;

static void outs(const wchar_t *str, size_t len) __attribute__((nonnull));

static void outputScrolled(unsigned cols);


static void output(void) {
	unsigned cols = columns();
	wchar_t tmp;

	hilightLeft = D.cur.error ? 0 : D.cur.hilightPos;

	termBeginLine();

	/* State */
	if (D.cur.error) {
		tmp = '!';
	} else {
		switch (D.cur.state) {
		case MPD_STATUS_STATE_STOP:  tmp = L'#'; break;
		case MPD_STATUS_STATE_PLAY:  tmp = L'>'; break;
		case MPD_STATUS_STATE_PAUSE: tmp = L' '; break;
		default:                     tmp = L'?'; break;
		}
	}
	outs(&tmp, 1);
	if (!--cols) {
		goto end;
	}

	outs((const wchar_t[]){L' '}, 1);
	if (!--cols || cols == 1) {
		goto end;
	}

	if (D.line_len < cols) {
		outs(D.line, D.line_len);
	} else {
		outputScrolled(cols);
	}

	if (hilightLeft && hilightLeft < cols) {
		printf("%*s", (int)hilightLeft, "");
		termNormal();
	}

end:
	termEndLine();
}


static void outputScrolled(unsigned cols) {
	static const wchar_t separator[7] = L" * * * ";
	static const size_t separator_len =
		sizeof separator / sizeof *separator;

	const size_t scroll = D.cur.scroll;

	if (scroll < D.line_len) {
		unsigned len = D.line_len - D.cur.scroll;
		if (len >= cols) {
			len = cols - 1;
		}
		outs(D.line + scroll, len);
		cols -= len;
	}

	if (cols != 1) {
		unsigned skip = 0, len = separator_len;
		if (scroll > D.line_len) {
			skip = D.cur.scroll - D.line_len;
			len -= skip;
		}

		if (len >= cols) {
			len = cols - 1;
		}

		outs(separator + skip, len);
		cols -= len;
	}

	if (cols != 1) {
		outs(D.line, cols - 1);
		cols = 1;
	}

	D.cur.scroll = (scroll + 1) % (D.line_len + separator_len);
}


static void _outs(const wchar_t *str, size_t len) __attribute__((nonnull));

static void outs(const wchar_t *str, size_t len) {
	if (hilightLeft && len >= hilightLeft) {
		_outs(str, hilightLeft);
		len -= hilightLeft;
		str += hilightLeft;
		hilightLeft = 0;
		termNormal();
	}

	if (len) {
		_outs(str, len);
		hilightLeft -= len;
	}
}


/******************** Formatting line ***************************************/
static void _ensureCapacity(size_t capacity);
#define ensureCapacity(capacity) do { \
		if (D.line_capacity < (capacity)) _ensureCapacity(capacity); \
	} while (0)

static size_t appendUTF(size_t offset, const char *str, size_t len)
	__attribute__((nonnull));
static size_t appendUTFStr(size_t offset, const char *str)
	__attribute__((nonnull));
static size_t appendW(size_t offset, const wchar_t *str, size_t len)
	__attribute__((nonnull));

static size_t doFormat(const wchar_t *p, size_t offset, const wchar_t **last)
	__attribute__((nonnull(1)));


static void formatLine(void) {
	if (D.cur.error) {
		appendW(0, L"[", 1);
		D.line_len = appendW(appendUTFStr(1, D.conn->errorStr), L"]", 1);
	} else if (!D.info) {
		D.line_len = appendW(0, L"[no song]", 10);
	} else {
		D.line_len = doFormat(D.format, 0, 0);
	}
}


static const wchar_t *skipBraces(const wchar_t *p) {
	unsigned stack = 1;
	for(;; ++p) {
		switch (*p) {
		case L'[':
			++stack;
			break;

		case L']':
			--stack;
			if (!stack == 0) {
				return p;
			}
			break;

		case 0:
			return p;
		}
	}
}

static const wchar_t *skipFormatting(const wchar_t *p) {
	for (;; ++p) {
		switch (*p) {
		case L'[':
			p = skipBraces(p);
			if (!*p) {
				return p;
			}
			break;

		case L'#':
			if (p[1]) {
				++p;
			}
			break;

		case L']':
			++p;
			/* FALL THROUGH */

		case L'&':
		case L'|':
		case 0:
			return p;
		}
	}
}


static size_t doFormatTag(const wchar_t *p, size_t off, const wchar_t **last) {
	const mpd_Song *const song = D.info->info.song;
	const wchar_t *const name = p;
	const char *value;
	size_t len;

	/* Tag */
	while (*p && *p != L'%') {
		++p;
	}
	len = p - name;
	if (last) {
		*last = *p ? p + 1 : p;
	}

#define EQ(str) \
	((len == sizeof #str - 1) && !memcmp(L"" #str, name, sizeof #str - 1))

	value = 0;
	if (EQ(Y)) return off - 1; /* special case */
	else if (EQ(artist  )) value = song->artist;
	else if (EQ(title   )) value = song->title;
	else if (EQ(album   )) value = song->album;
	else if (EQ(track   )) value = song->track;
	else if (EQ(path    )) value = song->file;
	else if (EQ(name    )) value = song->name;
	else if (EQ(date    )) value = song->date;
	else if (EQ(genre   )) value = song->genre;
	else if (EQ(composer)) value = song->composer;
	else if (EQ(disc    )) value = song->disc;
	else if (EQ(comment )) value = song->comment;
	else if (EQ(time    )) {
		if (song->time != MPD_SONG_NO_TIME) {
			static char buffer[16];
			snprintf(buffer, sizeof buffer, "%2d:%02d",
					 song->time / 60, song->time % 60);
			value = buffer;
		}
	} else if (!song->file) {
		/* nop */
	} else if (EQ(file     )) {
		value = strrchr(song->file, '/');
		value = value ? value + 1 : song->file;
	} else if (EQ(filenoext) || EQ(pathnoext)) {
		char *dot;
		value = strchr(song->file, '/');
		value = value ? value + 1 : song->file;
		dot   = strchr(value, '.');
		value = *name == L'p' ? song->file : value;

		if (dot) {
			return appendUTF(off, value, dot - value);
		}
	} else if (EQ(dir)) {
		value = strrchr(song->file, '/');
		return value ? appendUTF(off, song->file, value - song->file) : off;
	}

#undef EQ

	return value ? appendUTFStr(off, value) : off;
}


static size_t doFormat(const wchar_t *p, size_t offset, const wchar_t **last) {
	size_t off = offset;
	int found = 0;

	for(;;) {
		switch (*p) {
		case 0   :   /* NUL */
			if (last) {
				*last = p;
			}
			return off;

		case L'#':   /* Escape */
			if (*++p) {
				off = appendW(off, p, 1);
			}
			++p;
			break;

		case L'|':   /* OR */
			if (!found) {
				++p;
				off = offset;
			} else {
				p = skipFormatting(p + 1);
			}
			break;

		case L'&':   /* AND */
			if (!found) {
				p = skipFormatting(p + 1);
			} else {
				++p;
				found = 0;
			}
			break;

		case L'[': { /* Open group */
			size_t o = doFormat(p + 1, off, &p);
			found = found || o != off;
			off = o;
		}
			break;

		case L']':   /* Close group */
			if (last) {
				*last = p + 1;
			}
			return found ? off : offset;

		case L'%': { /* Tag */
			size_t o = doFormatTag(p + 1, off, &p);
			found = found || o != off;
			off = o + 1 == off /* special case */ ? off : o;
		}
			break;

		default  : { /* Copy variable chars */
			const wchar_t *ch = p;
			while (*++p && !wcschr(L"#%|&[]", *p)) { /* nop */ }
			off = appendW(off, ch, p - ch);
		}
			break;
		}
	}
}


static void _ensureCapacity(size_t capacity)
{
	size_t c = D.line_capacity;

	/* Check for overflow */
	die_on(capacity * (size_t)2 < capacity
	    || capacity * sizeof *D.line < capacity
	    || capacity * (size_t)2 * sizeof *D.line < capacity,
	       "requested too much memory: %zu", capacity);

	if (!c) {
		c = 32;
	}
	while (c < capacity) {
		c *= 2;
	}

	/* Don't care about loosing track of memory if realloc(3) fails,
	 * we're going to die anyways. */
	D.line = realloc(D.line, c * sizeof *D.line);
	pdie_on(!D.line, "malloc");
	D.line_capacity = c;
}


static size_t appendUTFStr(size_t offset, const char *str) {
	return appendUTF(offset, str, strlen(str));
}


static size_t appendW(size_t offset, const wchar_t *str, size_t len)
{
	ensureCapacity(offset + len);
	memcpy(D.line + offset, str, len * sizeof *str);
	return offset + len;
}


/******************** Terminal handling **************************************/
static void termDone(void) {
	/* Show cursor */
	if (!D.background) {
		puts("\33[?25h");
	}
	/* Reset colors */
	puts("\33[0m\n");
}

static void termInit(void) {
	atexit(termDone);
	if (!D.background) {
		/* Hide cursor */
		fputs("\33[?25l", stdout);
	}
}

static void termBeginLine(void) {
	/* Begining of line */
	if (D.background) {
		fputs("\0337\33[1;1f\r", stdout);
	} else {
		putchar('\r');
	}

	/* Set color */
	if (D.cur.error) {
		fputs("\33[30;1m", stdout);     /* dark grey */
	} else if (hilightLeft) {
		fputs("\33[37;1;44m", stdout);  /* hilighted */
	} else {
		termNormal();                   /* normal */
	}
}

static void termNormal(void) {
	fputs("\33[0m", stdout);            /* normal */
}

static void termEndLine(void) {
	/* Clear line */
	fputs("\33[0m\33[K\r", stdout);

	/* Get back to saved position */
	if (D.background) {
		fputs("\0338", stdout);
	}

	/* and flush */
	fflush(stdout);
}



/******************** Misc **************************************************/

static void _die(int perr, const char *fmt, ...)
{
	fputs(argv0, stderr);
	if (fmt) {
		va_list ap;
		va_start(ap, fmt);
		fputs(": ", stderr);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	if (perr) {
		fprintf(stderr, ": %s", strerror(errno));
	}
	putc('\n', stderr);
	exit(1);
}



/******************** Character encoding stuff ******************************/

#if HAVE_ICONV
static iconv_t iconv_utf2wchar = (iconv_t)-1;
static iconv_t iconv_str2wchar = (iconv_t)-1;
static iconv_t iconv_wchar2str = (iconv_t)-1;

static void initIconv(void);
static int  iconvDo(iconv_t cd, char *in, size_t inleft, size_t chsize,
                    void (*outFunc)(const char *buf, size_t len, void *data),
                    void *data);
#else
#  ifndef __STDC_ISO_10646__
#    error Unsupported wchar_t encoding, requires ISO 10646 (Unicode)
#  endif
#define initIconv() do ; while (0)
#define iconvDo(cd, in, inleft, chsize, outFunc, data) 0
#endif


static void initCodesets(void) {
	setlocale(LC_CTYPE, "");
	initIconv();
}




#if HAVE_ICONV
static void initIconv(void) {
	static const char *const wchar_codes[] = {
		"WCHAR_T//TRANSLIT", "WCHAR_T//IGNORE", "WCHAR_T", 0
	};
	static const char *const code_suffix[] = {
		"//TRANSLIT", "//IGNORE", "", 0
	};

	const char *const *it;
	const char *codeset;
	char *buffer;
	size_t len;

	/*  UTF-8 -> wchar_t */
	it = wchar_codes;
	do {
		iconv_utf2wchar = iconv_open(*it, "UTF-8");
	} while (iconv_utf2wchar == (iconv_t)-1 && *++it);
#ifndef __STDC_ISO_10646__
	die_on(iconv_utf2wchar == (iconv_t)-1,
	       "cannot handle UTF-8->wchar_t conversion; no iconv() support nor wchar_t uses unicode");
#endif

	/* Get current codeset */
	codeset = nl_langinfo(CODESET);
	if (!codeset || !*codeset) {
		return;
	}

	/* multibyte -> wchar_t */
	it = wchar_codes;
	do {
		iconv_str2wchar = iconv_open(*it, codeset);
	} while (iconv_str2wchar == (iconv_t)-1 && *++it);

	/* wchar_t -> multibyte */
	len = strlen(codeset);
	buffer = malloc(len + 11);
	if (!buffer) {
		iconv_wchar2str = iconv_open(codeset, "WCHAR_T");
		return;
	}

	memcpy(buffer, codeset, len);

	it = code_suffix;
	do {
		strcat(buffer + len, *it);
		iconv_wchar2str = iconv_open(buffer, "WCHAR_T");
	} while (iconv_wchar2str == (iconv_t)-1 && *++it);

	free(buffer);
}


static int  iconvDo(iconv_t cd, char *in, size_t inleft, size_t chsize,
                    void (*outFunc)(const char *buf, size_t len, void *data),
                    void *data) {
	char buffer[256];

	if (cd == (iconv_t)-1) {
		return 0;
	}

	inleft *= chsize;
	iconv(cd, 0, 0, 0, 0);
	for(;;) {
		size_t outleft = sizeof buffer;
		char *out = buffer;
		size_t ret = iconv(cd, &in, &inleft, &out, &outleft);

		if (out != buffer) {
			outFunc(buffer, out - buffer, data);
		}

		if (!in) {
			/* We are done */
			break;
		} else if (ret != (size_t)-1) {
			/* The whole string has been converted now reset the
			 * state */
			in = 0;
		} else if (errno == EILSEQ) {
			/* Invalid sequence, skip character (should never
			 * happen) */
			in += chsize;
			inleft -= chsize;
		} else if (errno == EINVAL) {
			/* Partial sequence at the end, break (should never
			 * happen) */
			break;
		}
	}

	return 1;
}
#endif




static void _outsIconvFunc(const char *buffer, size_t len, void *ignore) {
	(void)ignore;
	printf("%.*s", (int)len, buffer);
}

static void _outs(const wchar_t *str, size_t len) {
	if (!iconvDo(iconv_wchar2str, (void *)str, len, sizeof *str,
	             _outsIconvFunc, 0)) {
		char buf[MB_CUR_MAX];
		int ret = wctomb(NULL, 0);
		for (; len; --len, ++str) {
			ret = wctomb(buf, *str);
			if (ret > 0) {
				printf("%.*s", ret, buf);
			} else {
				putchar('?');
			}
		}
	}
}


static void appendIconvFunc(const char *buffer, size_t len, void *_off) {
	size_t *offsetp = _off;

	len /= sizeof *D.line;
	ensureCapacity(*offsetp + len);
	memcpy(D.line + *offsetp, buffer, len * sizeof *D.line);
	*offsetp += len;
}

static size_t appendUTFFallback(size_t offset, const char *str, size_t len);

static size_t appendUTF(size_t offset, const char *str, size_t len) {
	size_t off = offset;

	if (iconvDo(iconv_utf2wchar, (char *)str, len, 1, appendIconvFunc, &off)) {
		return off;
	}
	return appendUTFFallback(offset, str, len);
}

static size_t appendUTFFallback(size_t offset, const char *str, size_t len) {
#ifdef __STDC_ISO_10646__
	uint_least32_t val = 0;
	unsigned seq = 0;
	wchar_t *out;

	ensureCapacity(offset + len);
	out = D.line + offset;

	/* http://en.wikipedia.org/wiki/UTF-8#Description */
	/* Invalid sequences are simply ignored. */
	/* This code does not check for 3- and 4-byte long sequences which
	 * could be encoded using fewer bytes.  Those are treated as valid
	 * characters even though they shouldn't be. */
	while (len--) {
		unsigned char ch = *str++;

		if (!ch) {
			seq = 0;
		} else if (ch < 0x80) {
			seq = 0;
			*out++ = ch;
		} else if ((ch & 0xC0) == 0x80) {
			if (!seq) continue;
			val = (val << 6) | (ch & 0x3f);
			if (!--seq && (val < 0xD800 || val > 0xDFFF)
			           && val <= (uint_least32_t)WCHAR_MAX - WCHAR_MIN) {
				*out = val;
			}
		} else if (ch == 0xC0 || ch == 0xC1 || ch >= 0xF5) {
			seq = 0;
		} else if ((ch & 0xE0) == 0xC0) {
			seq = 1;
			val = ch & ~0x1F;
		} else if ((ch & 0xF0) == 0xE0) {
			seq = 2;
			val = ch & 0xF;
		} else if ((ch & 0xF0) == 0xF0) {
			seq = 3;
			val = ch & 0xF;
		}
	}

	return out - D.line;
#else
	/* We should never be here */
	die_on(1, "internall error (%d)", __LINE__);
#endif
}


struct wbuffer {
	size_t len;
	size_t capacity;
	wchar_t *buf;
};

static void
wideFromMultiIconvFunc(const char *buffer, size_t len, void *_data);

static const wchar_t *wideFromMulti(const char *str) {
	size_t len = strlen(str), pos;
	struct wbuffer wb = { 0, 0, 0 };
	mbstate_t ps;

	if (iconvDo(iconv_str2wchar, (char *)str, len, 1,
	            wideFromMultiIconvFunc, &wb)) {
		goto done;
	}

	memset(&ps, 0, sizeof ps);
	wb.capacity = 32;
	pos = 0;
	goto realloc;

	for(;;) {
		size_t ret = mbrtowc(wb.buf + wb.len, str, len, &ps);

		if (ret == (size_t)-1) {
			/* EILSEQ, try skipping single byte */
			++str;
			--len;
		} else if (ret) {
			/* Consumed ret bytes */
			str += ret;
			len -= ret;
			if (++wb.len < wb.capacity) continue;

			wb.capacity *= 2;
realloc:
			wb.buf = realloc(wb.buf, wb.capacity * sizeof *wb.buf);
			pdie_on(!wb.buf, "malloc");
		} else {
			/* Got NUL */
			break;
		}
	}

done:
	if (wb.capacity == wb.len + 1) {
		return wb.buf;
	} else {
		wchar_t *ret = realloc(wb.buf, (wb.len + 1) * sizeof *wb.buf);
		return ret ? ret : wb.buf;
	}
}


static void
wideFromMultiIconvFunc(const char *buffer, size_t len, void *_data) {
	struct wbuffer *wb = _data;

	len /= sizeof *wb->buf;

	if (wb->len + len + 1 < wb->capacity) {
		size_t cap = wb->capacity ? wb->capacity : 16;
		size_t total = wb->len + len + 1;
		do {
			cap *= 2;
		} while (cap < total);

		wb->capacity = cap;
		wb->buf = realloc(wb->buf, cap * sizeof *wb->buf);
		pdie_on(!wb->buf, "malloc");
	}

	memcpy(wb->buf, buffer, len * sizeof *wb->buf);
	wb->len += len;
	wb->buf[wb->len] = 0;
}
