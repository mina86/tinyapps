/*
 * Prints song MPD's curently playing.
 * $Id: mpd-show.c,v 1.15 2006/10/06 10:23:46 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazarewicz (mina86/AT/mina86.com)
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


#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "libmpdclient.h"



/********** Defaults **********/
#define DEFAULT_HOST    "localhost"
#define DEFAULT_PORT    6600
#define DEFAULT_COLUMNS 80
/*#define CHARSET_FROM    "UTF8" */
/*#define DEFAULT_CHARSET "ISO-8859-2" */
#define DEFAULT_FORMAT \
	"[[[%artist% <&%album%> ]|[%artist% - ]|[<%album%> ]]" \
	"&[[%track%. &%title%]|%title%]]"  \
	"|[[%track%. &%title%]|%title%]"   \
	"|%filenoext%"



/******************** Error messages ********************/
static const char *program_name = "mpd-show";
#define ERR(msg, ...) fprintf(stderr, "%s: " msg "\n", program_name, __VA_ARGS__)



/******************** Types ********************/
struct config {
	char *host, *password;
	char *format, *buffer;
	long port;
	unsigned columns, background;
};

struct name {
	size_t capacity, len, scroll;
	char *buffer;
	int state;
};

struct state {
	long songid;
	unsigned pos, opos, len;
	short state, redisplay;
};



/******************** Functions ********************/
void parse_arguments(int argc, char **argv, struct config *config);

mpd_Connection *connect_to_mpd(struct config *config);
void fill_name_with_error(mpd_Connection *conn, struct name *name);

void zero_state(struct state *state);
int get_song(mpd_Connection *conn, struct config *config, struct state *state,
             struct name *name);

void redisplay(struct config *config, struct name *name, struct state *state);
void loop_redisplay(struct config *config, struct name *name,
                    struct state *state, unsigned time);


#ifdef HAVE_SIGNAL_H
static int signum = 0;
void signal_handler(int sig);
void signal_resize (int sig);
#else
#define signum 0
#endif



/******************** Main ********************/
int main(int argc, char **argv) {
	mpd_Connection *conn;
	struct config config;
	struct name name = { 0, 0, 0, 0, ' ' };
	struct state state;


	/* Parse arguments */
	parse_arguments(argc, argv, &config);


	/* Daemonize */
	if (config.background==2) {
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


	/* Catch signals */
#ifdef HAVE_SIGNAL_H
	signal(SIGWINCH, signal_resize );
	signal(SIGHUP  , signal_handler);
	signal(SIGINT  , signal_handler);
	signal(SIGQUIT , signal_handler);
#endif


	/* Main loop */
	conn = connect_to_mpd(&config);
	do {
		int error = 0;
		zero_state(&state);

		/* Connect */
		while (!signum && conn->error) {
			if (error!=conn->error) {
				fill_name_with_error(conn, &name);
				state.pos = 0;
				state.len = 1;
				state.redisplay = 1;
				error = conn->error;
			}
			mpd_closeConnection(conn);
			loop_redisplay(&config, &name, &state, 5);
			conn = connect_to_mpd(&config);
		}


		/* Get and display song */
		while (!signum && get_song(conn, &config, &state, &name)) {
			loop_redisplay(&config, &name, &state, 1);
		}
	} while (!signum);


	/* Return */
	mpd_closeConnection(conn);
	write(0, "\n", 1);
	return signum;
}



/******************** Usage ********************/
void usage() {
	printf("mpd-show   (c) 2006 by Michal Nazarewicz (mina86/AT/mina86.com)\n" \
		   "$Id: mpd-show.c,v 1.15 2006/10/06 10:23:46 mina86 Exp $\n"
		   "\n"															\
		   "usage: %s [ <options> ] [ <host> [ <port> ]]\n"				\
		   "<options> are:\n"											\
		   " -b      run in background mode (does not fork into background)\n" \
		   " -B      run in background mode and fork into background\n" \
		   " -c<col> assume terminal is <col>-character wide; defaults to COLUMNS or %d\n" \
		   " -f<fmt> use <fmt> for displaying song (for syntax see mpc(1)); supports\n" \
		   "         following tags: album, artist, comment, composer, date, dir, disc,\n" \
		   "         file, filenoext, genre, name, path, pathnoext, time, title and track.\n" \
		   "<host>   host name MPD is running with optional password and '@' character\n" \
		   "         at the beginning; defaults to MPD_HOST or '" DEFAULT_HOST "'\n" \
		   "<port>   port MPD is listening; defaults to MPD_PORT or %d\n",
		   program_name, DEFAULT_COLUMNS, DEFAULT_PORT);
}



/******************** Parse arguments ********************/
void parse_arguments(int argc, char **argv, struct config *config) {
	char *hostarg = 0, *portarg = 0, *end;
	int opt;


	/* Program name */
	program_name = strrchr(argv[0], '/');
	program_name = program_name ? program_name + 1 : *argv;


	/* Help */
	if (argc>1 && !strcmp(argv[1], "--help")) {
		usage();
		exit(0);
	}


	/* Defults */
	config->format  = DEFAULT_FORMAT;
	config->columns = 0;
	config->background = 0;


	/* Get opts */
	opterr = 0;
	while ((opt = getopt(argc, argv, "-hbBc:f:"))!=-1) {
		switch (opt) {
		case 'h': usage(); exit(0);
		case 'b': config->background = 1; break;
		case 'B': config->background = 2; break;

			/* Columns */
		case 'c':
			{
				long c = strtol(optarg, &end, 0);
				if (c<3 || *end) {
					ERR("invalid terminal width: %s", optarg);
					exit(1);
				}
				config->columns = c;
			}
			break;

			/* Format */
		case 'f':
			config->format = optarg;
			break;

			/* An argument */
		case 1:
			if (!hostarg) { hostarg = optarg; break; }
			if (!portarg) { portarg = optarg; break; }
			ERR("invalid argument: %s", optarg);
			exit(1);

			/* An error */
		default:
			ERR("invalid option: %c", optopt);
			exit(1);
		}
	}


	/* Host and password */
	if (!hostarg && !(hostarg = getenv("MPD_HOST"))) {
		hostarg = DEFAULT_HOST;
	}

	config->host = strchr(hostarg, '@');
	if (!config->host) {
		config->host = hostarg;
		config->password = 0;
	} else {
		*config->host = 0;
		++config->host;
		config->password = hostarg;
	}


	/* Port */
	if (!portarg && !(portarg = getenv("MPD_PORT"))) {
		config->port = DEFAULT_PORT;
	} else {
		config->port = strtol(portarg, &end, 10);
		if (config->port<0 || *end) {
			ERR("invalid port: %s", portarg);
			exit(1);
		}
	}


	/* Columns */
	if (!config->columns) {
		end = getenv("COLUMNS");
		if (end) {
			long c = strtol(end, &end, 0);
			if (c<3 || *end) {
				ERR("invalid terminal width: %s", getenv("COLUMNS"));
				exit(1);
			}
			config->columns = c;
		} else {
			config->columns = DEFAULT_COLUMNS;
		}
	}


	/* Buffer */
	config->buffer = malloc(config->columns+64);
	if (!config->buffer) {
		ERR("not enought memory to allocate %d btyes", config->columns+64);
		exit(1);
	}
}



/******************** Connect to MPD ********************/
mpd_Connection *connect_to_mpd(struct config *config) {
	mpd_Connection *conn = mpd_newConnection(config->host, config->port, 10);

	/* Send password */
	if (config->password) {
		if (conn->error) return conn;
		mpd_sendPasswordCommand(conn, config->password);
		if (conn->error) return conn;
		mpd_finishCommand(conn);
	}

	/* Return */
	return conn;
}



/******************** Append to name ********************/
size_t append_to_name(struct name *name, size_t offset,
                      const char *str, size_t len) {
	if (offset+len+1>=name->capacity) {
		size_t cap = (offset + len + 1 + 128) & ~127;
		char *tmp = name->buffer ? realloc(name->buffer, cap) : malloc(cap);
		if (!tmp) {
			return offset;
		}
		name->capacity = cap;
		name->buffer = tmp;
	}
	memcpy(name->buffer + offset, str, len);
	name->buffer[offset + len] = 0;
	return offset + len;
}

inline size_t append_to_name_cstr(struct name *name, size_t offset,
                                  const char *str) {
	return append_to_name(name, offset, str, strlen(str));
}



/******************** Fill name with error message ********************/
void fill_name_with_error(mpd_Connection *conn, struct name *name) {
	if (conn->error) {
		name->state  = '!';
		name->len    = append_to_name_cstr(name, 0, conn->errorStr);
		name->scroll = 0;
	}
}



/******************** Formats song title ********************/
const char *skip_formatting(const char *p) {
	unsigned stack = 0;
	for (; *p; ++p) {
		if (*p == '[') {
			++stack;
		} else if (*p=='#' && p[1]) {
			++p;
		} else if (stack) {
			if (*p==']') --stack;
		} else if (*p=='&' || *p=='|' || *p==']') {
			break;
		}
	}
	return p;
}


size_t format_song_internal(const mpd_Song *song, const char *p,
                            struct name *name, size_t offset,
                            const char **last) {
	static char _buffer[sizeof(int) * 3 + 3];
	char found = 0, *temp;
	size_t off = offset, len;

	while (*p) {
		/* Copy variable chars */
		const char *end = p;
		while (*end && *end!='#' && *end!='%' && *end!='|' && *end!='&'
		       && *end!='[' && *end!=']') ++end;
		if (p!=end) {
			off = append_to_name(name, off, p, end - p);
			p = end;
			continue;
		}

		/* Escape */
		if (*p=='#' && *++p) {
			off = append_to_name(name, off, p, 1);
			++p;
			continue;
		}

		/* OR */
		if (*p=='|') {
			++p;
			if (!found) {
				off = offset;
			} else {
				p = skip_formatting(p);
			}
			continue;
		}

		/* AND */
		if (*p=='&') {
			++p;
			if (!found) {
				p = skip_formatting(p);
			} else {
				found = 0;
			}
			continue;
		}

		/* Open group */
		if (*p=='[') {
			size_t o = format_song_internal(song, p+1, name, off, &p);
			if (o!=off) {
				found = 1;
				off = o;
			}
			continue;
		}

		/* Close group */
		if (*p==']') {
			if (last) *last = p+1;
			return found ? off : offset;
		}

		/* Tag */
		for (end = ++p; *end>='a' && *end<='z'; ++end);
		len = end - p;

		temp = 0;
		if      (len==6 && !strncmp("artist"  , p, 6)) temp = song->artist;
		else if (len==5 && !strncmp("title"   , p, 5)) temp = song->title;
		else if (len==5 && !strncmp("album"   , p, 5)) temp = song->album;
		else if (len==5 && !strncmp("track"   , p, 5)) temp = song->track;
		else if (len==4 && !strncmp("path"    , p, 4)) temp = song->file;
		else if (len==4 && !strncmp("name"    , p, 4)) temp = song->name;
		else if (len==4 && !strncmp("date"    , p, 4)) temp = song->date;
		else if (len==5 && !strncmp("genre"   , p, 5)) temp = song->genre;
		else if (len==8 && !strncmp("composer", p, 8)) temp = song->composer;
		else if (len==4 && !strncmp("disc"    , p, 4)) temp = song->disc;
		else if (len==7 && !strncmp("comment" , p, 7)) temp = song->comment;
		else if (len==4 && !strncmp("time"    , p, 4)) {
			if (song->time!=MPD_SONG_NO_TIME) {
				snprintf(_buffer, sizeof _buffer, "%d:%d",
				         song->time/60, song->time % 60);
				temp = _buffer;
			}
		} else if (!song->file) {
			/* check that just in case */
		} else if (len==4 && !strncmp("file", p, 4)) {
			temp = strrchr(song->file, '/');
			temp = temp ? temp + 1 : song->file;
		} else if (len==9 && (!strncmp("filenoext", p, 9) ||
		                      !strncmp("pathnoext", p, 9))) {
			char *ch, *dot = 0;
			for (temp = ch = song->file; *ch; ++ch) {
				if (*ch=='/') {
					temp = ch+1;
					dot = 0;
				} else if (*ch=='.') {
					dot = ch;
				}
			}
			if (*p=='p') temp = song->file;
			if (temp && dot) {
				found = 1;
				off = append_to_name(name, off, temp, dot - temp);
				temp = 0;
			}
		} else if (len==3 && !strncmp("dir", p, 3)) {
			temp = strrchr(song->file, '/');
			if (temp) {
				found = 1;
				off = append_to_name(name, off, song->file, temp-song->file);
				temp = 0;
			}
		}

		if (temp) {
			found = 1;
			off = append_to_name_cstr(name, off, temp);
		}

		p = end;
		if (*p=='%') ++p;
	}

	if (last) *last = p;
	return off;
}


void format_song(struct config *config, struct name *name, mpd_Song *song) {
	name->len    = format_song_internal(song, config->format, name, 0, 0);
	name->scroll = 0;
}



/******************** Zerores state ********************/
void zero_state(struct state *state) {
	state->songid    = -2;
	state->len       = 1;
	state->pos       = state->opos = 0;
	state->state     = -2;
	state->redisplay = 1;
}



/******************** Queries song from MPD ********************/
int get_song(mpd_Connection *conn, struct config *config, struct state *state,
             struct name *name) {
	mpd_Status *status;
	mpd_InfoEntity *info;

	state->redisplay = 0;

	/* Get status */
	mpd_sendStatusCommand(conn);  if (conn->error) return 0;
	status = mpd_getStatus(conn); if (conn->error) return 0;
	mpd_nextListOkCommand(conn);
	if (conn->error) {
		mpd_freeStatus(status);
		return 0;
	}

	/* Error */
	/*
	if (status->error) {
		if (state->songid!=-1) {
			name->len = append_to_name_cstr(name, 0, status->error);
			state->songid    = -1;
			state->pos       = state->opos = 0;
			state->len       = 1;
			state->redisplay = 1;
			name->state      = ' ';
		}
		return 1;
	}
	*/

	/* Copy status */
	state->opos   = state->pos;
	state->pos    = status->elapsedTime;
	if (status->state!=state->state) {
		switch ((state->state = status->state)) {
		case MPD_STATUS_STATE_STOP : name->state = '#'; break;
		case MPD_STATUS_STATE_PLAY : name->state = '>'; break;
		case MPD_STATUS_STATE_PAUSE: name->state = ' '; break;
		default                    : name->state = '?'; break;
		}
		state->redisplay = 1;
	}

	/* It's the same song */
	if (status->songid==state->songid) {
		mpd_freeStatus(status);
		return 1;
	}

	/* It's different song */
	state->redisplay = 1;
	state->len    = status->totalTime ? status->totalTime : 1;
	state->songid = status->songid;

	/* Get song info */
	mpd_sendCurrentSongCommand(conn);      if (conn->error) return 0;
	info = mpd_getNextInfoEntity(conn);    if (!info) return 0;
	if (info->type!=MPD_INFO_ENTITY_TYPE_SONG) {
		mpd_freeInfoEntity(info);
		return 0;
	}

	/* Format song string */
	format_song(config, name, info->info.song);
	mpd_freeInfoEntity(info);
	return 1;
}



/******************** Displays title ********************/
void redisplay(struct config *config, struct name *name, struct state *state){
	static const char separator[] = "   * * *   ";
	static const size_t sep_len = sizeof(separator) - 1;

	/* Aliases */
	const size_t columns = config->columns;
	char *const buffer = config->buffer + 0;

	/* Fill buffer with song name */
	if (name->len <= columns - 3) {
		memcpy(buffer + 3, name->buffer, name->len);
		memset(buffer + 3 + name->len, ' ', columns - 3 - name->len);
	} else {
		size_t off = 3;
		if (name->scroll >= name->len + sep_len) {
			name->scroll = 0;
		}

		/* Name for the first time */
		if (name->scroll<name->len) {
			size_t l = name->len - name->scroll;
			if (l > columns - off) l = columns - off;
			memcpy(buffer + off, name->buffer + name->scroll, l);
			off += l;
		}

		/* Separator */
		if (off<columns) {
			size_t o = off==3 ? name->scroll - name->len : 0;
			size_t l = sep_len - o;
			if (l > columns - off) l = columns - off;
			memcpy(buffer + off, separator + o, l);
			off += l;
		}

		/* Name for the second time */
		if (off<columns) {
			memcpy(buffer + off, name->buffer, columns - off);
		}

		++name->scroll;
	}

	/* Init buffer */
	memcpy(buffer - 26, "\0337\33[1;1f\r\33[0m\33[K\33[37;1;44m", 26);
	buffer[0] = name->state;
	buffer[1] = ' ';
	buffer[columns  ] = '\33';
	buffer[columns+1] = '8';

	/* Print */
	{
		size_t split = state->len ? state->pos * columns / state->len : 0;
		if (split>columns) split = columns;
		write(0, buffer - (config->background ? 26 : 18),
		      split + (config->background ? 26 : 18));
		write(0, "\33[0;37m", 7);
		write(0, buffer+split, columns-split+(config->background ? 2 :0));
	}
}



/******************** Redisplays and waits time seconds ********************/
void loop_redisplay(struct config *config, struct name *name,
                    struct state *state, unsigned time) {
	if (name->len > config->columns - 3) {
		for (time *= 2; time && !signum; --time) {
			redisplay(config, name, state);
			usleep(500000);
		}
	} else{
		if (state->redisplay || !state->len ||
	        state-> pos*config->columns/state->len !=
	        state->opos*config->columns/state->len) {
			redisplay(config, name, state);
		}
		sleep(time);
	}
}



/******************** Signal handler ********************/
#ifdef HAVE_SIGNAL_H
void signal_handler(int sig) {
	if (signum) {
		exit(1);
	}
	signum = sig;
}


/********** Terminal have been resized **********/
void signal_resize (int sig) {
	sig = sig; /* supress warning */
}
#endif
