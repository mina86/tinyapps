/*
 * Prints song MPD's curently playing.
 * $Id: mpd-show.c,v 1.1 2005/08/30 22:27:20 mina86 Exp $
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

#include "libmpdclient.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static const char *program_name = NULL;
#define ERR(msg, ...) fprintf(stderr, "%s: " msg "\n", program_name, ##__VA_ARGS__)


/********** Config **********/
#define DEFAULT_HOST    "localhost"
#define DEFAULT_PORT    "6600"
#define DEFAULT_COLUMNS 70


/********** Macros **********/
#define STRDUP(str) ((str) ? strdup(str) : NULL)
#define STREQ(s1, s2) ((s1)==(s2) || ((s1!=NULL) && (s2!=NULL) \
									  && !strcmp(s1, s2)))


/********** Some global variables and stuff **********/
static char *hostarg = NULL, *portarg = NULL;


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



/********** Main **********/
int main(int argc, char **argv) {
	song_t songs[2];
	int num;
	mpd_Connection *conn;

	songs[0].artist = songs[0].album = songs[0].title = NULL;
	songs[0].len = songs[0].pos = songs[0].songid = songs[0].song =
		songs[0].state = -1;
	songs[1].artist = songs[1].album = songs[1].title = NULL;
	songs[1].len = songs[1].pos = songs[1].songid = songs[1].song =
		songs[1].state = -1;

	parse_args(argc, argv);
	conn = connect_to_mpd();
	for(num = 0; ; num ^= 1) {
		free_song(songs + num);
		get_song(conn, songs + num);
		if (diff_songs(songs, songs + 1)) {
			print_song(songs + num);
		}
		sleep(1);
	}

	mpd_closeConnection(conn);
	return 0;
}



/********** Usage information **********/
void usage() {
	printf("mpd-state  0.12.1  (c) 2005 by Avuton Olrich & Michal Nazarewicz\n"
		   "usage: %s [ [<pass>@]<host> [ <port> ]]\n"
		   " <pass>  password used to connect; if no set no password is used\n"
		   " <host>  hostname MPD is running; if not set MPD_HOST is used;\n"
		   "         if that is also missing '" DEFAULT_HOST "' is assumed\n"
		   " <port>  port MPD is listining; if not set MPD_PORT is used;\n"
		   "         if that is also missing " DEFAULT_PORT " is assumed\n",
		   program_name);
}



/********** Parse arguments **********/
void parse_args(int argc, char **argv) {
	program_name = strrchr(argv[0], '/');
	program_name = program_name==NULL ? *argv : (program_name + 1);

	if ((argc>1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
		|| argc>3) {
		usage();
		exit(argc>3 ? 1 : 0);
	}

	hostarg = argc>=2 ? argv[1] : NULL;
	portarg = argc>=3 ? argv[2] : NULL;
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
	return song1->len    != song2->len
		|| song1->pos    != song2->pos
		|| song1->songid != song2->songid
		|| song1->song   != song2->song
		|| song1->state  != song2->state
		|| !STREQ(song1->artist, song2->artist)
		|| !STREQ(song1->album,  song2->album)
		|| !STREQ(song1->title,  song2->title);
}


/********** Formats and prints song to stdout **********/
void print_song(song_t *song) {
	char buf[DEFAULT_COLUMNS];
	int artist_len, album_len, title_len, col, sum;

	artist_len = song->artist ? strlen(song->artist) : 0;
	album_len  = song->album  ? strlen(song->album ) : 0;
	title_len  = song->title  ? strlen(song->title ) : 13;

	col = DEFAULT_COLUMNS - 4;
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

	memset(buf, ' ', 80);

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
	}

	memcpy(buf + col, song->title ? song->title : "Unknown title", title_len);

	col = (0.0 + DEFAULT_COLUMNS) * song->pos / song->len;
	write(0, "\r\33[0m\33[K\33[37;1;44m", 18);
	write(0, buf, col);
	write(0, "\33[0;37m", 7);
	write(0, buf + col, DEFAULT_COLUMNS - col);
}
