/*
 * Logs to syslog all messages logged to syslog :]
 * $Id: infinite-logger.c,v 1.2 2005/07/11 00:20:58 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/tlen.pl)
 * Licensed under the Academic Free License version 2.1.
 */

/*
 * Installation (as root):
 *   mkfifo /var/log/EVERYTHING
 *   chown nobody:nogroup /var/log/EVERYTHING
 *   chmod 600 /var/log/EVERYTHING
 *   echo '*.* |/var/log/EVERYTHING' >>/etc/syslog.conf
 *   kill -SIGHUP `cat /var/run/syslogd.pid
 *   su nobody -c '/path/to/infinite-logger'
 * Also add the fallowing line to system's initialisation script:
 *   su nobody -c '/path/to/infinite-logger'
 *
 */

/*
 * WARNING: If you didn't notice yet: This is only a joke.
 */

#include <stdio.h>
#include <syslog.h>

int main(void) {
	FILE *fp = fopen("/var/log/EVERYTHING", "r");
	if (fp==NULL) {
		fputs("Could not open /var/log/EVERYTHING", stderr);
		return 1;
	}
	openlog("infinite-logger", 0, LOG_USER);

	char buffer[4096];
	for(;;) syslog(LOG_DEBUG, "%s", fgets(buffer, sizeof(buffer), fp));

	closelog();
	fclose(fp);
}
