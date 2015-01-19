/*
 * Timer.
 * Copyright (c) 2005-2007 by Michal Nazareicz <mina86@mina86.com>
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

#define _POSIX_C_SOURCE 199309
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>

volatile sig_atomic_t signum = 0;

static void sighandler(int sig) {
	signum = sig;
}


int main(void) {
	struct timespec wait = { 0, 10000000 };
	unsigned int m = 0, ss = 0;
	char buffer[11] = "\r   0:00.00";

#ifdef SIGHUP
	signal(SIGHUP, sighandler);
#endif
#ifdef SIGINT
	signal(SIGINT, sighandler);
#endif
#ifdef SIGQUIT
	signal(SIGQUIT, sighandler);
#endif
#ifdef SIGPIPE
	signal(SIGPIPE, sighandler);
#endif
#ifdef SIGTERM
	signal(SIGTERM, sighandler);
#endif
#ifdef SIGSTOP
	signal(SIGSTOP, sighandler);
#endif
#ifdef SIGTSTP
	signal(SIGTSTP, sighandler);
#endif

	do {
		if (write(1, buffer, 11)!=11) return 1;

		while (!signum && nanosleep(&wait, &wait));
		if (signum) break;
		wait.tv_nsec = 10000000;

		if ((ss = (ss+1)%6000)==0) {
			++m;
			buffer[ 1] = m<1000 ? ' ' : (m/1000%10 + '0');
			buffer[ 1] = m<100  ? ' ' : (m/100 %10 + '0');
			buffer[ 3] = m<10   ? ' ' : (m/10  %10 + '0');
			buffer[ 4] =                 m     %10 + '0';
		}

		buffer[ 6] = ss/1000    + '0';
		buffer[ 7] = ss/100 %10 + '0';
		buffer[ 9] = ss/10  %10 + '0';
		buffer[10] = ss     %10 + '0';
	} while (!signum);

	printf("%07ld\n", 10000000 - wait.tv_nsec);
	return 0;
}
