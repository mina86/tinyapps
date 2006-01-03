/*
 * Prints song MPD's curently playing.
 * $Id: mpd-show.c,v 1.3 2006/01/03 14:08:59 mina86 Exp $
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
#define ERR(msg, ...) fprintf(stderr, "%s: " msg "\n", program_name, ##__VA_ARGS__)


/********** Config **********/
#define DEFAULT_HOST    "localhost"
#define DEFAULT_PORT    "6600"
#define DEFAULT_COLUMNS 80
#define CHARSET_FROM    "UTF8"
#define DEFAULT_CHARSET "ISO-8859-1"


/********** Macros **********/
#ifdef HAVE_ICONV_H
#define STRDUP(str) ((str) ? iconvdup(str) : NULL)
#else
#define STRDUP(str) ((str) ? strdup(str) : NULL)
#endif
#define STREQ(s1, s2) ((s1)==(s2) || ((s1!=NULL) && (s2!=NULL) \
									  && !strcmp(s1, s2)))


/********** Some global variables and stuff **********/
static char *hostarg = NULL, *portarg = NULL;
static int columns = DEFAULT_COLUMNS, background = 0;
#ifdef HAVE_ICONV_H
static iconv_t conv = (iconv_t)-1;
#endif


/********** Structure with song information **********/
typedef struct {
	char *artist, *album, *title;
	unsigned int len, pos, songid, song, state;
} song_t;


/********** Functions declaration **********/
void usage();
void parse_args(int argc, char **argv);

mpd_Connection *connect_to_mpd();
void print_error_and_exit(mpd_Connection *conn, int exit_code);

void free_song(song_t *song);
void get_song(mpd_Connection *conn, song_t *song);
int  diff_songs(song_t *song1, song_t *song2);
void print_song(song_t *song);

#ifdef HAVE_SIGNAL_H
static int signum = 0;
void signal_handler(int sig);
void signal_resize (int sig);
#else
#define signum 0
#endif

#ifdef HAVE_ICONV_H
char *iconvdup(char *s);
#endif



/********** Main **********/
int main(int argc, char **argv) {
	song_t songs[2];
	int num;
	mpd_Connection *conn;

	/* Catch signals */
#ifdef HAVE_SIGNAL_H
	for (num = 32; --num; signal(num, &signal_handler)==SIG_IGN);
	signal(SIGWINCH, &signal_resize);
	signal(SIGCHLD , SIG_IGN); signal(SIGURG  , SIG_IGN);
	signal(SIGCONT , SIG_DFL);
	signal(SIGSTOP , SIG_DFL); signal(SIGTSTP , SIG_DFL);
	signal(SIGTTIN , SIG_DFL); signal(SIGTTOU , SIG_DFL);
#endif

	/* Zero songs */
	songs[0].artist = songs[0].album = songs[0].title = NULL;
	songs[0].len = songs[0].pos = songs[0].songid = songs[0].song =
		songs[0].state = -1;
	songs[1].artist = songs[1].album = songs[1].title = NULL;
	songs[1].len = songs[1].pos = songs[1].songid = songs[1].song =
		songs[1].state = -1;

	/* Parse args and connect */
	parse_args(argc, argv);
	conn = connect_to_mpd();

	/* Open ICONV */
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
	for(num = 0; !signum; num ^= 1) {
		free_song(songs + num);
		get_song(conn, songs + num);
		if (background || diff_songs(songs, songs + 1)) {
			print_song(songs + num);
		}
		sleep(1);
	}


	/* Close ICONV */
#ifdef HAVE_ICONV_H
	if (conv!=(iconv_t)-1) {
		iconv_close(conv);
	}
#endif

	/* Clear line, disconnect and exit */
	if (!background) {
		fputs("\33[2K\r", stdout);
	}
	mpd_closeConnection(conn);
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
		   "        if that is also missing " DEFAULT_PORT " is assumed\n",
		   program_name);
}



/********** Parse arguments **********/
void parse_args(int argc, char **argv) {
	program_name = strrchr(argv[0], '/');
	program_name = program_name==NULL ? *argv : (program_name + 1);

	if (argc>1 && !strcmp(argv[1], "--help")) {
		usage();
		exit(0);
	}

	int opt;
	char *end;
	opterr = 0;
	while ((opt = getopt(argc, argv, "-hbBc:"))!=-1) {
		switch (opt) {
		case 'h': usage(); exit(0);
		case 'b': background = 1; break;
		case 'B': background = 2; break;

		case 'c':
			columns = strtol(optarg, &end, 0);
			if (columns<10 || *end) {
				ERR("invalid terminal width: %s", optarg);
				exit(1);
			}
			break;

		case 1:
			if (hostarg==NULL) { hostarg = optarg; break; }
			if (portarg==NULL) { portarg = optarg; break; }
			ERR("invalid argument: %s", optarg);
			exit(1);

		default:
			ERR("invalid option: %c", optopt);
			exit(1);
		}
	}
}



/********** Connects to MPD **********/
mpd_Connection *connect_to_mpd() {
	long port;
	char *host, *password = NULL, *test;
	mpd_Connection *conn;

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
		++host;
		password = hostarg;
	}

	/* Port */
	if (portarg==NULL && (portarg = getenv("MPD_PORT"))==NULL) {
		portarg = DEFAULT_PORT;
	}

	port = strtol(portarg, &test,10);
	if (port<0 || *test!='\0') {
		ERR("invalid port: %s", portarg);
		exit(1);
	}

	/* Connect */
	conn = mpd_newConnection(host, port, 10);
	if (conn->error) {
		ERR("could not connect to MPD (%s:%ld)", host, port);
		free(hostarg);
		print_error_and_exit(conn, 2);
	}
	free(hostarg);


	/* Send password */
	if (password) {
		mpd_sendPasswordCommand(conn, password);
		if (conn->error) {
			ERR("%s", "could not connect to MPD, invalid password");
			print_error_and_exit(conn, 2);
		}
		mpd_finishCommand(conn);
		print_error_and_exit(conn, 2);
	}

	return conn;
}



/********** Prints error end exits if there is any **********/
void print_error_and_exit(mpd_Connection *conn, int error_code) {
	if (conn->error) {
		ERR("error: %s", conn->errorStr);
		exit(error_code);
	}
}



/********** Frees strings used by song **********/
void free_song(song_t *song) {
	if (song->artist) { free(song->artist); song->artist = NULL; }
	if (song->album ) { free(song->album ); song->album  = NULL; }
	if (song->title ) { free(song->title ); song->title  = NULL; }
	song->len = song->pos = song->songid = song->song = song->state = -1;
}


/********** Queries the song information **********/
void get_song(mpd_Connection *conn, song_t *song) {
	mpd_Status *status;
	mpd_InfoEntity *entity;

	/* get status */
	mpd_sendStatusCommand(conn);
	print_error_and_exit(conn, 3);
	status = mpd_getStatus(conn);
	print_error_and_exit(conn, 3);
	mpd_nextListOkCommand(conn);
	print_error_and_exit(conn, 3);

	/* Copy status */
	song->state = status->state;
	song->song = status->song;
	song->songid = status->songid;
	song->pos = status->elapsedTime;
	song->len = status->totalTime;
	mpd_freeStatus(status);

	/* get song info */
	mpd_sendCurrentSongCommand(conn);
	print_error_and_exit(conn, 3);
	while ((entity = mpd_getNextInfoEntity(conn))) {
		if (entity->type==MPD_INFO_ENTITY_TYPE_SONG) {
			song->artist = STRDUP(entity->info.song->artist);
			song->album  = STRDUP(entity->info.song->album);
			song->title  = STRDUP(entity->info.song->title);
		}
		mpd_freeInfoEntity(entity);
	}
	mpd_nextListOkCommand(conn);
	print_error_and_exit(conn, 3);
}



/********** Returns true if two songs differ **********/
int  diff_songs(song_t *song1, song_t *song2) {
	return song1->songid != song2->songid
		|| song1->song   != song2->song
		|| song1->state  != song2->state
		|| (int)((0.0 + columns) * song1->pos / song1->len)
		!= (int)((0.0 + columns) * song2->pos / song2->len)
		|| !STREQ(song1->artist, song2->artist)
		|| !STREQ(song1->album,  song2->album)
		|| !STREQ(song1->title,  song2->title);
}


/********** Formats and prints song to stdout **********/
void print_song(song_t *song) {
	char buf[columns];
	int artist_len, album_len, title_len, col, sum;

	artist_len = song->artist ? strlen(song->artist) : 0;
	album_len  = song->album  ? strlen(song->album ) : 0;
	title_len  = song->title  ? strlen(song->title ) : 13;

	col = columns - 4;
	sum = artist_len + album_len + title_len;

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

	memset(buf, ' ', columns);

	switch (song->state) {
	case MPD_STATUS_STATE_STOP : buf[0] = '['; buf[1] = ']'; break;
	case MPD_STATUS_STATE_PLAY : buf[0] = ' '; buf[1] = '>'; break;
	case MPD_STATUS_STATE_PAUSE: buf[0] = '|'; buf[1] = '|'; break;
	default                    : buf[0] = '?'; buf[1] = '?'; break;
	}

	col = 3;
	if (artist_len) {
		memcpy(buf + col, song->artist, artist_len);
		col += artist_len;
	}

	if (album_len) {
		if (artist_len) buf[col++] = ' ';
		buf[col++] = '<';
		memcpy(buf + col, song->album, album_len);
		col += album_len;
		buf[col++] = '>';
		buf[col++] = ' ';
	} else if (artist_len) {
		buf[col++] = ' ';
		buf[col++] = '-';
		buf[col++] = ' ';
	}

	memcpy(buf + col, song->title ? song->title : "Unknown title", title_len);

	col = (0.0 + columns) * song->pos / song->len;
	if (background) {
		write(0, "\0337\33[1;1f", 8);
	}
	write(0, "\33[0m\33[K\33[37;1;44m", 17);
	write(0, buf, col);
	write(0, "\33[0;37m", 7);
	write(0, buf + col, columns - col);
	if (background) {
		write(0, "\0338", 2);
	}
}


#ifdef HAVE_SIGNAL_H
/********** Signal handler **********/
void signal_handler(int sig) {
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
	size_t sleft = columns<<1, oleft = columns>>1;
	iconv(conv, &s, &sleft, &o, &oleft);
	return out;
}
#endif
