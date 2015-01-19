/*
 * tuptime - Shows total and biggest uptime.
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

/*
 * Total Uptime Mini-HOWTO
 *
 * First compile this file and move the executable to /usr/bin.
 *
 * Then add the fallowing line to start scripts (eg. at the end of
 * /etc/rc.d/rc.S in Slackware Linux).  It must be run after /var/log
 * is moundet read-write and before crond is lunched.
 *
 *    [ -s /var/log/cuptime ] && mv /var/log/cuptime /var/log/buptime
 *
 * Then add the fallowing lines to both halt and reboot scripts (on
 * some systems, eg. Slackware Linux, it is one script) (eg. before
 * 'umount -a -r -t nfs,smbfs' in /etc/rc.d/rc.6 in Slackware Linux).
 * It must be run before /var/log is unmounted and should be run on
 * seconds before shutdown.  It also must be run only once per session
 * or otherwise tuptime will give invalid results.
 *
 *    /usr/bin/tuptime -r >/var/log/cuptime
 *    [ -s /var/log/cuptime ] && mv /var/log/cuptime /var/log/buptime
 *
 * And finally, add an entry to crontab:
 *
 *    0,15,30,45 * * * * /usr/bin/tuptime -r >/var/log/cuptime
 *
 * To avoid error message when tuptime is run before first reboot run:
 *
 *    echo 0 0 0 0 >/var/log/buptime
 *
 * Note: Never use tuptime >/var/log/buptime since tuptime reads
 *       /var/log/buptime file and if you try to write to the file it's
 *       content will be eresed and therefore tuptime will return current
 *       uptime
 */


#include <stdio.h>


/* Filenames */
#define  UPTIME "/proc/uptime"
#define BUPTIME "/var/log/buptime"


/* Error messages stuff */
static char *program_name;   /* Name program was run with */
#define ERR(msg, arg) fprintf(stderr, "%s: " msg "\n", program_name, arg)
#define ERRS(msg) ERR("%s", msg);


/* Usage information */
static void usage(void) {
	puts("tuptime  0.1  (c) 2005,2006 Michal Nazarewicz\n"
	     "usage: tuptime [ -h | [ -b<file> ] [ -u<file> ] -r ]\n"
	     " -h        prints this screen\n"
	     " -b<file>  path to 'buptime' file (default '" BUPTIME "')\n"
	     " -u<file>  path to 'uptime' file (default '" UPTIME "')\n"
	     " -r        prints total uptime in raw format");

}


/* Prints uptime */
static void print_uptime(const char *title, double uptime, double idle) {
	int d = uptime / 24 / 3600;
	int h = (int)(uptime/3600) % 24;
	int m = (int)(uptime/ 60 );
	double sec = uptime - m * 60;
	m %= 60;

	printf("%s %3d d %2d:%02d:%05.2f (%5.2f%% idle)",
		   title, d, h, m, sec, idle*100/uptime);
}


/* Main */
int main (int argc, char **argv) {
	const char *buptime_file = BUPTIME, *uptime_file = UPTIME;
	double uptime, idle, tuptime, tidle, muptime, midle;
	char *c, mode = 'p';
	FILE *fp;
	int i;


	/* Parse program name */
	for (program_name = c = *argv; *c; ++c) {
		if (*c=='/') {
			program_name = c+1;
		}
	}


	/* Parse arguments */
	i = 0;
	c = 0;
	while ((c && *++c) || (c = 0) || ++i < argc) {
		char *arg;
		if (!c) {
			c = argv[i];
			if (c[0]!='-' || c[1]==0 || c[1]=='-') {
				ERR("invalid argument: %s", argv[i]);
				return 1;
			}
			++c;
		}

		switch (*c) {
		case 'h':
			usage();
			return 0;

		case 'r':
			mode = 'r';
			break;

		case 'b':
		case 'u':
			if (c[1]) {
				arg = c + 1;
			} else if (++i<argc) {
				arg = argv[i];
			} else {
				ERR("parameter expected after -%c", *c);
				return 1;
			}
			*c == 'b' ? (buptime_file = arg) : (uptime_file = arg);
			c = 0;
			break;

		default:
			ERR("invalid option: -%c", *c);
			return 1;
		}
	}


	/* Read /proc/uptime */
	fp = fopen(uptime_file, "r");
	if (!fp) {
		ERR("could not open %s", uptime_file);
		return 2;
	}
	if (fscanf(fp, "%lf %lf", &uptime, &idle)!=2) {
		fclose(fp);
		ERR("error reading %s", uptime_file);
		return 2;
	}
	fclose(fp);


	/* Read buptime */
	fp = fopen(buptime_file, "r");
	if (!fp) {
		ERR("could not open %s (skipping)", buptime_file);
		tuptime = tidle = 0;
	} else {
		i = fscanf(fp, "%lf %lf %lf %lf", &tuptime, &tidle, &muptime, &midle);
		if (i!=2 && i!=4) {
			ERR("error reading %s (skipping)", buptime_file);
			tuptime = tidle = 0;
		}
		fclose(fp);
	}


	/* Calculate total uptime */
	if (uptime>muptime) {
		muptime = uptime;
		midle = idle;
	}
	tuptime += uptime;
	tidle += idle;


	/* Print raw */
	if (mode=='r') {
		printf("%.2f %.2f\n%.2f %.2f\n", tuptime, tidle, muptime, midle);
		return 0;
	}


	/* Print if print mode */
	print_uptime("total:   ", tuptime, tidle);
	putchar('\n');
	print_uptime("biggest: ", muptime, midle);
	printf(" (%.2f%% of total)\n", muptime*100/tuptime);
	print_uptime("current: ", uptime, idle);
	printf(" (%.2f%% of total, %.2f%% of biggest)\n",
		   uptime*100/tuptime, uptime*100/muptime);
	return 0;
}
