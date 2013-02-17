/*
 * Logs to syslog all messages logged to syslog :]
 * Copyright (c) 2005-2007 by Michal Nazareicz (mina86/AT/mina86.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This software is OSI Certified Open Source Software.
 * OSI Certified is a certification mark of the Open Source Initiative.
 *
 * This is part of Tiny Applications Collection
 *   -> http://tinyapps.sourceforge.net/
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
	char buffer[4096];
	FILE *fp;

	if (!(fp = fopen("/var/log/EVERYTHING", "r"))) {
		fputs("Could not open /var/log/EVERYTHING", stderr);
		return 1;
	}
	openlog("infinite-logger", 0, LOG_USER);

	while (fgets(buffer, sizeof buffer, fp)) {
		syslog(LOG_DEBUG, "%s", buffer);
	}

	closelog();
	fclose(fp);
	return 0;
}
