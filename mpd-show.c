/*
 * Prints song MPD's curently playing.
 * $Id: mpd-show.c,v 1.10 2006/04/27 14:15:55 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/tlen.pl)
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


#define HAVE_SIGNAL_H
#define HAVE_ICONV_H


#include "libmpdclient.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif


static const char *program_name = NULL;
#define ERR(msg, ...) fprintf(stderr, "%s: " msg "\n", program_name, __VA_ARGS__)


/********** Config **********/
#define DEFAULT_HOST    "localhost"
#define DEFAULT_PORT    6600
#define DEFAULT_COLUMNS 80
#define CHARSET_FROM    "UTF8"
#define DEFAULT_CHARSET "ISO-8859-1"


/********** Macros **********/
#define STRDUP(str) ((str) ? iconvdup(str) : NULL)
#define STREQ(s1, s2) ((s1)==(s2) || ((s1!=NULL) && (s2!=NULL) \
									  && !strcmp(s1, s2)))


/********** Some global variables **********/
static char *host = NULL, *password = NULL;
static long int port = 0;

static int columns = DEFAULT_COLUMNS, background = 0;

static long int songid = -2;
static int len = 1, pos = 0, state = 0, opos = 0, ostate = 0;
static char *buf = NULL;

#ifdef HAVE_ICONV_H
static iconv_t conv = (iconv_t)-1;
#endif


/********** Functions declaration **********/
void usage();
void parse_args(int argc, char **argv);

mpd_Connection *connect_to_mpd();
int  get_song(mpd_Connection *conn);
void print_song();

#ifdef HAVE_SIGNAL_H
static int signum = 0;
void signal_handler(int sig);
void signal_resize (int sig);
#else
#define signum 0
#endif

#ifdef HAVE_ICONV_H
char *iconvdup(char *s);
#else
#define iconvdup(str) strdup(str)
#endif



/********** Main **********/
int main(int argc, char **argv) {
	mpd_Connection *conn;

	/* Catch signals */
#ifdef HAVE_SIGNAL_H
	signal(SIGWINCH, &signal_resize );
	signal(SIGHUP  , &signal_handler);
	signal(SIGINT  , &signal_handler);
	signal(SIGQUIT , &signal_handler);
#endif


	/* Init */
	parse_args(argc, argv);
	if ((buf = malloc(columns+1))==NULL) {
		perror("malloc");
	}
#ifdef HAVE_ICONV_H
	conv = iconv_open(DEFAULT_CHARSET, CHARSET_FROM);
#endif


	/* Daemonize */
	if (background==2) {
		pid_t pid;
		switch (pid = fork()) {
		case -1: perror("fork"); return -1;
		case  0: break;
		default: printf("[%d]\n", pid); return 0;
		}
		if (setsid()==-1) {
			perror("setsid");
			return -1;
		}
	}



	/* The Loop */
	do {
		/* Connect */
		for (conn = connect_to_mpd(); !signum && conn->error; ) {
			mpd_closeConnection(conn);
			memset(buf, ' ', columns); memset(buf, '-', 3);
			state = -1; songid = -2; pos = 0; len = 1;
			print_song();
			sleep(5);
			conn = connect_to_mpd();
		}


		/* Print song */
		for (; !signum && get_song(conn); sleep(1)){
			if (background || state!=ostate
				|| (int)((0.0 + columns) *  pos / len)
				!= (int)((0.0 + columns) * opos / len)) {
				print_song();
			}
		}


		/* Clear state */
		mpd_closeConnection(conn);
	} while (!signum);


	/* Close everything and exit */
#ifdef HAVE_ICONV_H
	if (conv!=(iconv_t)-1) {
		iconv_close(conv);
	}
#endif
	if (!background) {
		fputs("\33[2K\r", stdout);
	}
	free(buf);
	free(host);
	if (password!=NULL) free(password);
	return 0;
}



/********** Usage information **********/
void usage() {
	printf("mpd-state  0.12.1  (c) 2005 by Avuton Olrich & Michal Nazarewicz\n"
		   "usage: %s [ <options> ] [ [<pass>@]<host> [ <port> ]]\n"
		   "<options> are:\n"
		   " -b      run in background mode (does not fork into bockground)\n"
		   " -B      run in background mode and fork into background\n"
		   " -c<col> assume terminal is <col>-character wide            [80]\n"
		   "\n"
		   "<pass>  password used to connect; if no set no password is used\n"
		   "<host>  hostname MPD is running; if not set MPD_HOST is used;\n"
		   "        if that is also missing '" DEFAULT_HOST "' is assumed\n"
		   "<port>  port MPD is listining; if not set MPD_PORT is used;\n"
		   "        if that is also missing %d is assumed\n",
		   program_name, DEFAULT_PORT);
}



/********** Parse arguments **********/
void parse_args(int argc, char **argv) {
	char *hostarg = NULL, *portarg = NULL, *test;

	/* Program name */
	program_name = strrchr(argv[0], '/');
	program_name = program_name==NULL ? *argv : (program_name + 1);

	/* Help */
	if (argc>1 && !strcmp(argv[1], "--help")) {
		usage();
		exit(0);
	}

	/* Get opts */
	int opt;
	char *end;
	opterr = 0;
	while ((opt = getopt(argc, argv, "-hbBc:"))!=-1) {
		switch (opt) {
		case 'h': usage(); exit(0);
		case 'b': background = 1; break;
		case 'B': background = 2; break;

			/* Columns */
		case 'c':
			columns = strtol(optarg, &end, 0);
			if (columns<10 || *end) {
				ERR("invalid terminal width: %s", optarg);
				exit(1);
			}
			break;

			/* An argument */
		case 1:
			if (hostarg==NULL) { hostarg = optarg; break; }
			if (portarg==NULL) { portarg = optarg; break; }
			ERR("invalid argument: %s", optarg);
			exit(1);

			/* An error */
		default:
			ERR("invalid option: %c", optopt);
			exit(1);
		}
	}


	/* Host and password */
	if (hostarg==NULL && (hostarg = getenv("MPD_HOST"))==NULL) {
		hostarg = DEFAULT_HOST;
	}

	hostarg = strdup(hostarg);
	host = strchr(hostarg, '@');
	if (host==NULL) {
		host = hostarg;
	} else {
		*host = 0;
		host = strdup(host+1);
		password = strdup(hostarg);
		free(hostarg);
	}

	/* Port */
	if (portarg==NULL && (portarg = getenv("MPD_PORT"))==NULL) {
		port = DEFAULT_PORT;
	} else {
		port = strtol(portarg, &test, 10);
		if (port<0 || *test!='\0') {
			ERR("invalid port: %s", portarg);
			exit(1);
		}
	}
}



/********** Connects to MPD **********/
mpd_Connection *connect_to_mpd() {
	/* Connect */
	mpd_Connection *conn = mpd_newConnection(host, port, 10);
	if (conn->error) return conn;

	/* Send password */
	if (password) {
		mpd_sendPasswordCommand(conn, password);
		if (conn->error) return conn;
		mpd_finishCommand(conn);
	}

	/* Return */
	return conn;
}



/********** Queries the song information **********/
int  get_song(mpd_Connection *conn) {
	/* get status */
	mpd_Status *status = NULL;
	mpd_sendStatusCommand(conn);       if (conn->error) return 0;
	status = mpd_getStatus(conn);      if (conn->error) return 0;
	mpd_nextListOkCommand(conn);
	if (conn->error) {
		mpd_freeStatus(status);
		return 0;
	}

	/* Error */
	if (status->error) {
		if (songid!=-1) {
			len = strlen(status->error);
			memcpy(buf, status->error, len>columns ? columns : len);
			opos = pos = 0;
			len = 1;
			ostate = songid = -2;
			state = -1;
		} else {
			ostate = MPD_STATUS_STATE_PAUSE;
		}
	}

	/* Copy status */
	opos = pos;
	pos = status->elapsedTime;
	ostate = state;
	state = status->state;
	len = status->totalTime;
	if (status->songid==songid) {
		mpd_freeStatus(status);
		return 1;
	}

	ostate = state^1;
	songid = status->songid;
	mpd_freeStatus(status);

	/* get song info */
	mpd_InfoEntity *entity;
	mpd_sendCurrentSongCommand(conn);      if (conn->error) return 0;
	entity = mpd_getNextInfoEntity(conn);  if (!entity) return 0;
	if (entity->type!=MPD_INFO_ENTITY_TYPE_SONG) {
		mpd_freeInfoEntity(entity);
		return 0;
	}

	/* init strings and lengths */
#define artist entity->info.song->artist
#define album  entity->info.song->album
	const char *title = entity->info.song->title;
	if (!title) {
		title = !artist && !album ? entity->info.song->file : "Unknown title";
	}

	int artist_len, album_len, title_len, sum, col;
	artist_len = artist ? strlen(artist) : 0;
	album_len  = album  ? strlen(album ) : 0;
	title_len  = title  ? strlen(title ) : 0;
	sum = artist_len + album_len + title_len;
	col = columns - 3;

	if (sum>col && artist_len>col>>2) {
		artist_len = artist_len-sum+col<col>>2 ? col>>2 : artist_len-sum+col;
		sum = artist_len + album_len + title_len;
	}

	if (sum>col && album_len >col>>2) {
		album_len  = album_len -sum+col<col>>2 ? col>>2 : album_len -sum+col;
		sum = artist_len + album_len + title_len;
	}

	if (sum>col && title_len >col>>1) {
		title_len  = title_len -sum+col<col>>1 ? col>>1 : title_len -sum+col;
		sum = artist_len + album_len + title_len;
	}

	/* fill buffer */
	memset(buf, ' ', columns);
	buf[columns] = 0;

	col = 2;
	if (artist_len) {
		memcpy(buf + col, artist, artist_len);
		col += artist_len;
	}

	if (album_len) {
		if (artist_len) ++col;
		buf[col++] = '<';
		memcpy(buf + col, album, album_len);
		col += album_len;
		buf[col++] = '>';
		++col;
	} else if (artist_len) {
		++col;
		buf[col++] = '-';
		++col;
	}

	memcpy(buf + col, title, title_len);

	/* return */
	mpd_freeInfoEntity(entity);
	return 1;
}



/********** Formats and prints song to stdout **********/
void print_song() {
	switch (state) {
	case MPD_STATUS_STATE_STOP : buf[0] = '#'; break;
	case MPD_STATUS_STATE_PLAY : buf[0] = '>'; break;
	case MPD_STATUS_STATE_PAUSE: buf[0] = ' '; break;
	case -1                    :               break;
	default                    : buf[0] = '?'; break;
	}

	int col = len ? (0.0 + columns) * pos / len : columns, old = buf[col];
	buf[col] = buf[columns] = 0;
	printf("%s\r\33[0m\33[K\33[37;1;44m%s\33[0;37m",
		   background ? "\0337\33[1;1f" : "", buf);
	buf[col] = old;
	printf("%s%s", buf + col, background ? "\338" : "");
	fflush(stdout);
}


#ifdef HAVE_SIGNAL_H
/********** Signal handler **********/
void signal_handler(int sig) {
	if (signum) {
		exit(1);
	}
	signum = sig;
}


/********** Terminal have been resized **********/
void signal_resize (int sig) {
}
#endif


#ifdef HAVE_ICONV_H
/********** Duplicates a string and performs conversion **********/
char *iconvdup(char *s) {
	if (conv==(iconv_t)-1) {
		return strdup(s);
	}

	char *out = malloc((columns>>1)+1);
	if (out==NULL) {
		return NULL;
	}

	char *o = out;
	size_t sleft = columns<<2, oleft = columns>>1;
	iconv(conv, &s, &sleft, &o, &oleft);
	return out;
}
#endif
