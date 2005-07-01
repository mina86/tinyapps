/*
 * Timer
 * $Id: timer.c,v 1.1 2005/07/01 17:36:50 mina86 Exp $
 * Copyright (C) 2005 by Michal Nazareicz (mn86/AT/o2.pl)
 * Licensed under the Academic Free License version 2.1.
 */

#include <unistd.h>
#include <time.h>

int main(void) {
	struct timespec wait = { 0, 10000000 };
	unsigned int m = 0, ss = 0;
	char buffer[11] = "\r   0:00.00";

	for(;;) {
		if (write(1, buffer, 11)!=11) return 1;

		while (nanosleep(&wait, &wait));
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
	}
}
