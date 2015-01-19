/*
 * Shows CPU load and some other information.
 * Copyright (c) 2005,2007 by Michal Nazareicz <mina86@mina86.com>
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


#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/**** Defines ****/
#define BUFFER_SIZE 1024
static char buffer[BUFFER_SIZE];


#ifdef ULLONG_MAX
typedef unsigned long long bigint;
#define FMT "llu"
#else
typedef unsigned long bigint;
#define FMT "lu"
#endif


/**** Structire with all information ****/
typedef struct {
	struct {
		bigint usr, nice, sys, idle, load, total;
	} cpu;
	struct {
		bigint rec, send;
	} traf;
	struct {
		unsigned long total, free;
	} mem;
} State;


/**** Updates CPU load info ****/
static void update_cpu(State *state) {
	FILE *file = fopen("/proc/stat", "r");
	int i;
	if (!file) return;

	i = fscanf(file, "%*s %" FMT " %" FMT " %" FMT " %" FMT ,
	           &state->cpu.usr, &state->cpu.nice,
	           &state->cpu.sys, &state->cpu.idle);
	fclose(file);

	switch (i) {
	case 0:
		state->cpu.usr = 0;
	case 1:
		state->cpu.nice = 0;
	case 2:
		state->cpu.sys = 0;
	case 3:
		state->cpu.idle = 0;
	}

	state->cpu.load  = state->cpu.usr + state->cpu.sys + state->cpu.nice;
	state->cpu.total = state->cpu.load + state->cpu.idle;
}


/**** Updates traffic info ****/
static void update_traf(State *state) {
	FILE *file = fopen("/proc/net/dev", "r");
	if (!file) return;

	while (fgets(buffer, BUFFER_SIZE, file)) {
		if (sscanf(buffer, " eth0:%" FMT " %*d %*d %*d %*d %*d %*d %*d %" FMT,
		           &state->traf.rec, &state->traf.send)) {
			break;
		}
	}
	fclose(file);
}


/**** Updates memory info ****/
static void update_mem(State *state) {
	FILE *file = fopen("/proc/meminfo", "r");
	unsigned long val;
	int flags = 0;
	if (!file) return;

	while (fgets(buffer, BUFFER_SIZE, file) && flags!=3) {
		if (sscanf(buffer, " MemTotal: %lu", &val)==1) {;
			flags |= 1;
			state->mem.total = val;
		} else if (sscanf(buffer, " MemFree: %lu", &val)==1) {;
			flags |= 2;
			state->mem.free = val;
		}
	}
	fclose(file);
}


/**** Calculates delta between two states ****/
static void make_delta(State *delta, const State *prev, const State *now) {
	delta->cpu.usr   = now->cpu.usr   - prev->cpu.usr  ;
	delta->cpu.nice  = now->cpu.nice  - prev->cpu.nice ;
	delta->cpu.sys   = now->cpu.sys   - prev->cpu.sys  ;
	delta->cpu.idle  = now->cpu.idle  - prev->cpu.idle ;
	delta->cpu.load  = now->cpu.load  - prev->cpu.load ;
	delta->cpu.total = now->cpu.total - prev->cpu.total;
	delta->traf.rec  = now->traf.rec  - prev->traf.rec ;
	delta->traf.send = now->traf.send - prev->traf.send;
	delta->mem.total = now->mem.total;
	delta->mem.free  = now->mem.free ;
}


/**** Prints state ****/
static void print_state(const State *s) {
	printf("%4" FMT " %4" FMT " %4" FMT " %4" FMT "   %4" FMT " %4" FMT
	       "   %3d%%   %7ld %7ld   %7" FMT " %7" FMT "\n",
	       s->cpu.usr, s->cpu.nice, s->cpu.sys, s->cpu.idle,
	       s->cpu.load, s->cpu.total,
	       (int)(s->cpu.total ? 100 * s->cpu.load / s->cpu.total : 0),
	       s->mem.free, s->mem.total,
	       s->traf.rec, s->traf.send);
}


/**** Main ****/
int main(void) {
	State states[3];
	int num = 0;
	update_cpu(states + 1);
	update_traf(states + 1);
	update_mem(states + 1);

	puts("user nice  sys idle | load  ttl | cpu  | memfree mem-ttl "
	     "|     rec    send");
	for(;;) {
		sleep(1);

		update_cpu(states + num);
		update_traf(states + num);
		update_mem(states + num);

		make_delta(states + 2, states + (num^1), states + num);
		print_state(states + 2);
		num ^= 1;
	}
}
