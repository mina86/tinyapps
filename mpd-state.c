/*
 * Prints/restores MPD's state.
 * $Id: mpd-state.c,v 1.4 2006/01/03 14:05:07 mina86 Exp $
 * Copyright (c) 2005 by Avuton Olrich (avuton/AT/gmail.com)
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


#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "6600"
#define LINELENGTH   FILENAME_MAX+50


/********** Some global variables and stuff **********/
static char opt_skip_state, opt_skip_playlist, opt_skip_outputs = 0,
	opt_restore = 0, opt_add = 0;
static mpd_Connection *conn = NULL;
static char *hostarg = NULL, *portarg = NULL;


/********** Functions declaration **********/
void usage();
void parse_args(int argc, char **argv);
void connect_to_mpd();
void print_error_and_exit(int exit_code);

void print_status();
void print_playlist();
void print_outputs();
void restore();



/********** Main **********/
int main(int argc, char **argv) {
	parse_args(argc, argv);
	connect_to_mpd();

	if (opt_restore) {
		restore();
		mpd_closeConnection(conn);
		return 0;
	}

	if (!opt_skip_state) print_status();
	if (!opt_skip_playlist) print_playlist();
	if (!opt_skip_outputs) print_outputs();
	mpd_closeConnection(conn);
	return 0;
}



/********** Usage information **********/
void usage() {
	printf("mpd-state  0.12.1  (c) 2005 by Avuton Olrich & Michal Nazarewicz\n"
		   "usage: %s [ -rasSpPoO ] [ <host> [ <port> ]]]\n"
		   " -r      restere the state read from stdin (default is to\n"
		   "         print status to stdout)\n"
		   " -a      add to the playlist (instead of replacing the playlist)\n"
		   " -s      ommit state\n"
		   " -S      ommit everything but state\n"
		   " -p      ommit playlsit\n"
		   " -P      ommit everything but playlsit\n"
		   " -o      ommit outputs (useful if you are using old MPD)\n"
		   " -O      ommit everything but outputs\n"
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

	if (argc>1 && !strcmp(argv[1], "--help")) {
		usage();
		exit(0);
	}

	int opt;
	opterr = 0;
	while ((opt = getopt(argc, argv, "-hraspo"))!=-1) {
		switch (opt) {
		case 'h': usage(); exit(0);
		case 'r': opt_restore = 1; break;
		case 'a': opt_add = 1; break;
		case 's': opt_skip_state = 1; break;
		case 'S': opt_skip_state = 0;
		          opt_skip_playlist = opt_skip_outputs = 1; break;
		case 'p': opt_skip_playlist = 1; break;
		case 'P': opt_skip_playlist = 0;
		          opt_skip_state = opt_skip_outputs = 1; break;
		case 'o': opt_skip_outputs = 1; break;
		case 'O': opt_skip_outputs = 0;
		          opt_skip_state = opt_skip_playlist = 1; break;

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
void connect_to_mpd() {
	long port;
	char *host, *password = NULL, *test;

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
		print_error_and_exit(2);
	}
	free(hostarg);


	/* Send password */
	if (password) {
		mpd_sendPasswordCommand(conn, password);
		if (conn->error) {
			ERR("%s", "could not connect to MPD, invalid password");
			print_error_and_exit(2);
		}
		mpd_finishCommand(conn);
		print_error_and_exit(2);
	}
}



/********** Restore state from stdin **********/
void restore() {
	int state = 0, seconds = MPD_PLAY_AT_BEGINNING, songnum = 0;
	char buffer[LINELENGTH], *word, *str;

	mpd_sendCommandListBegin(conn);
	if (!opt_skip_playlist && !opt_add) mpd_sendClearCommand(conn);

	while (fgets(buffer, LINELENGTH, stdin)) {
		word = strtok(buffer, ": \f\r\t\v\n");
		str  = strtok(NULL, ": \f\r\t\v\n");

		if (!strcmp(word, "state")) {
			if (!strcmp(word, "play")) state = MPD_STATUS_STATE_PLAY;
			else if (!strcmp(word, "pause")) state = MPD_STATUS_STATE_PAUSE;
			else if (!strcmp(word, "stop")) state = MPD_STATUS_STATE_STOP;
			else {
				ERR("invalid state: %s", word);
				exit(3);
			}

		} else if (!strcmp(word, "current")) {
			songnum = atoi(str);
		} else if (!strcmp(word,  "time")) {
			seconds = atoi(str);

		} else if (!strcmp(word, "volume")) {
			if (!opt_skip_state) mpd_sendSetvolCommand(conn, atoi(str));
		} else if (!strcmp(word, "random")) {
			if (!opt_skip_state) mpd_sendRandomCommand(conn, atoi(str));
		} else if (!strcmp(word, "repeat")) {
			if (!opt_skip_state) mpd_sendRepeatCommand(conn, atoi(str));
		} else if (!strcmp(word, "crossfade")) {
			if (!opt_skip_state) mpd_sendCrossfadeCommand(conn, atoi(str));

		} else if (!strcmp(word, "playlist_begin")) {
			while (fgets(buffer, LINELENGTH, stdin) &&
				   strcmp(word, "playlist_end\n") &&
				   (str = strchr(buffer, ':'))) {
				if (!opt_skip_playlist) continue;

				++str;
				str[strlen(str)-1] = '\0';
				mpd_sendAddCommand(conn, str);
			}

		} else if (!strcmp(word,"outputs_begin")) {
			while (fgets(buffer, LINELENGTH, stdin) &&
				   strcmp(word, "outputs_end\n") &&
				   (str = strchr(buffer, ':'))) {
				if (opt_skip_outputs) continue;

				if (atoi(str+1)) {
					mpd_sendEnableOutputCommand(conn, atoi(buffer));
				} else {
					mpd_sendDisableOutputCommand(conn, atoi(buffer));
				}
			}
		}
	}

	if (!opt_skip_state && state!=MPD_STATUS_STATE_STOP) {
		mpd_sendSeekCommand(conn, songnum, seconds);
	}

	mpd_sendCommandListEnd(conn);
	print_error_and_exit(2);
	mpd_finishCommand(conn);
}



/********** Prints MPD's status to stdout **********/
void print_status() {
	mpd_Status *status;

	mpd_sendStatusCommand(conn);
	print_error_and_exit(3);
	status = mpd_getStatus(conn);
	print_error_and_exit(3);
	mpd_nextListOkCommand(conn);
	print_error_and_exit(3);

	printf("state: %s\n", status->state==MPD_STATUS_STATE_PLAY ? "play"
		   : (status->state==MPD_STATUS_STATE_PAUSE ? "pause" : "stop"));
	printf("current: %i\n", status->song);
	printf("time: %i\n", status->elapsedTime);
	printf("random: %i\n", status->random ? 1 : 0 );
	printf("repeat: %i\n", status->repeat ? 1 : 0 );
	printf("crossfade: %i\n",status->crossfade ? 1 : 0 );
	printf("volume: %i\n", status->volume);

	mpd_finishCommand(conn);
	mpd_freeStatus(status);
	print_error_and_exit(3);
}



/********** Prints playlsit to stdout **********/
void print_playlist() {
	puts("playlist_begin");
	mpd_InfoEntity *entity;

	mpd_sendPlaylistInfoCommand(conn,-1);
	while ((entity = mpd_getNextInfoEntity(conn))) {
		if (entity->type==MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_Song *song = entity->info.song;
			printf("%i:%s\n", song->pos, song->file);
		}
		mpd_freeInfoEntity(entity);
	}
	mpd_finishCommand(conn);

	puts("playlist_end");
	print_error_and_exit(3);
}



/********** Prints outputs to stdout **********/
void print_outputs() {
	puts("outputs_begin");
	mpd_OutputEntity * output;

	mpd_sendOutputsCommand(conn);
	while ((output = mpd_getNextOutput(conn))) {
		if (output->id >= 0) {
			printf("%i:%i:%s\n", output->id, output->enabled, output->name);
		}
		mpd_freeOutputElement(output);
	}
	mpd_finishCommand(conn);

	puts("outputs_end");
	print_error_and_exit(3);
}



/********** Prints error end exits if there is any **********/
void print_error_and_exit(int error_code) {
	if (conn->error) {
		ERR("error: %s", conn->errorStr);
		exit(error_code);
	}
}
