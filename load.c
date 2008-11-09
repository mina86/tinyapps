/*
 * Shows CPU load and some other information.
 * $Id: load.c,v 1.7 2008/11/09 00:02:21 mina86 Exp $
 * Copyright (c) 2005,2007 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 3.0.
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
	if (!file) return;

	fscanf(file, "%*s %" FMT " %" FMT " %" FMT " %" FMT ,
	       &state->cpu.usr, &state->cpu.nice,
	       &state->cpu.sys, &state->cpu.idle);
	fclose(file);

	state->cpu.load  = state->cpu.usr + state->cpu.sys + state->cpu.nice;
	state->cpu.total = state->cpu.load + state->cpu.idle;
}


/**** Updates traffic info ****/
static void update_traf(State *state) {
	FILE *file = fopen("/proc/net/dev", "r");
	if (!file) return;

	while (!feof(file)) {
		fgets(buffer, BUFFER_SIZE, file);
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

	while (!feof(file) && flags!=3) {
		fgets(buffer, BUFFER_SIZE, file);
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
