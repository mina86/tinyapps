/*
 * Shows CPU load and some other information.
 * $Id: load.c,v 1.4 2007/06/30 08:41:02 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
 * Licensed under the Academic Free License version 2.1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/**** Defines ****/
#define BUFFER_SIZE 1024
#define BUFFER_SIZE_STR "1024"
char buffer[BUFFER_SIZE];


/**** Structire with all information ****/
typedef struct {
	struct {
		unsigned long long usr, nice, sys, idle, load, total;
	} cpu;
	struct {
		unsigned long long rec, send;
	} traf;
	struct {
		long total, free;
	} mem;
} State;


/**** Compare 2 strings ****/
int streq(const char *s1, const char *s2) {
	while (*s1 && *s1++==*s2++);
	return *s1 == *s2;
}


/**** Updates CPU load info ****/
void update_cpu(State *state) {
	FILE *file = fopen("/proc/stat", "r");
	if (file==NULL) return;

	fscanf(file, "%*s %Ld %Ld %Ld %Ld", &state->cpu.usr, &state->cpu.nice,
	       &state->cpu.sys, &state->cpu.idle);
	fclose(file);

	state->cpu.load  = state->cpu.usr + state->cpu.sys + state->cpu.nice;
	state->cpu.total = state->cpu.load + state->cpu.idle;
}


/**** Updates traffic info ****/
void update_traf(State *state) {
	FILE *file = fopen("/proc/net/dev", "r");
	if (file==NULL) return;

	while (!feof(file)) {
		fgets(buffer, BUFFER_SIZE, file);
		if (sscanf(buffer, " eth0:%Ld %*d %*d %*d %*d %*d %*d %*d %Ld",
		           &state->traf.rec, &state->traf.send)) {
			break;
		}
	}
	fclose(file);
}


/**** Updates memory info ****/
void update_mem(State *state) {
	FILE *file = fopen("/proc/meminfo", "r");
	long flags = 0, val;
	if (file==NULL) return;

	while (!feof(file) && flags!=3) {
		fscanf(file, "%" BUFFER_SIZE_STR "s %ld %*s", buffer, &val);
		if (streq(buffer, "MemTotal:")) {
			flags |= 1;
			state->mem.total = val;
		} else if (streq(buffer, "MemFree:")) {
			flags |= 2;
			state->mem.free = val;
		}
	}
	fclose(file);
}


/**** Calculates delta between two states ****/
void make_delta(State *delta, const State *prev, const State *now) {
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
void print_state(const State *s) {
	printf("%4Ld %4Ld %4Ld %4Ld   %4Ld %4Ld   %3d%%   %7ld %7ld   %7Ld %7Ld\n",
	       s->cpu.usr, s->cpu.nice, s->cpu.sys, s->cpu.idle,
	       s->cpu.load, s->cpu.total,
	       (int)(s->cpu.total==0?0:100 * s->cpu.load/s->cpu.total),
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
