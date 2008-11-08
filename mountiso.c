/*
 * mountiso - Mounts/unmounts ISO images
 * $Id: mountiso.c,v 1.7 2008/11/08 23:45:57 mina86 Exp $
 * Copyright (c) 2005-2007 by Michal Nazareicz (mina86/AT/mina86.com)
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
 * To install run (as an ordinary user):
 *
 * $ make mountios
 * < should compile w/o errors >
 * $ su
 * < should ask for root password >
 * # chown root:bin mountiso
 * # chmod 4755 mountiso
 * # cp mountiso /usr/bin
 * # exit
 */

#define _FILE_OFFSET_BITS 64 /* Large file support */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define DEFAULT_DIR "/media/cdrom"
#define MOUNT       "/bin/mount"
#define UMOUNT      "/bin/umount"
#define MNT_OPTS    "loop,ro,nosuid,noexec,nodev"


#if __STDC_VERSION__ < 199901L
#  if defined __GNUC__
#    define inline   __inline__
#  else
#    define inline
#  endif
#endif


static int check_paths(int argc, char **argv, uid_t uid, int mount);
static inline const char *mnt_opts(uid_t uid);


int main(int argc, char **argv) {
	int mount;
	uid_t uid;

	/* Get program name */
	{
		char *ch = strrchr(argv[0], '/');
		if (ch) {
			argv[0] = ch + 1;
		}
		mount = argv[0][0]!='u';
	}

	/* Check arguments */
#ifdef DEFAULT_DIR
	if (argc!=1+mount && argc!=2+mount) {
		fputs("usage: mountiso image.iso [ directory ]\n"
		      "       umountiso [ image.iso | directory ]\n", stderr);
		return 1;
	}
#else
	if (argc!=2+mount) {
		fputs("usage: mountiso image.iso directory\n"
		      "       umountiso image.iso | directory\n", stderr);
		return 1;
	}
#endif

	if ((uid = getuid())) {
		int ret;

		/* Became root */
		if (setuid(0)) {
			fprintf(stderr, "%s: setuid: %s\n", argv[0], strerror(errno));
			return 1;
		}

		/* Check paths */
		ret = check_paths(argc, argv, uid, mount);
		if (ret) {
			return ret;
		}
	}

	/* Mount */
#ifdef DEFAULT_DIR
	argv[argc] = (char*)DEFAULT_DIR;
#endif
	if (mount) {
		execl(MOUNT, MOUNT, "-o", mnt_opts(uid), "--", argv[1], argv[2],
		      (const char *)0);
	} else {
		execl(UMOUNT, UMOUNT, "--", argv[1], (const char *)0);
	}
	fprintf(stderr, "%s: %s: %s\n", argv[0], mount ? MOUNT : UMOUNT,
	        strerror(errno));
	return 1;
}



static int check_paths(int argc, char **argv, uid_t uid, int mount) {
	int i;

	for (i = 1; i < argc; ++i) {
		const char *err = 0;
		struct stat st;
		char *ch;

#if CHAR_MAX > 127
		for (ch = argv[i]; *ch && *ch>32 && *ch<=127; ++ch);
#else
		for (ch = argv[i]; *ch && *ch>32; ++ch);
#endif

		if (*ch) {
			err = "path contains insecure characters";
		} else if (stat(argv[i], &st)) {
			err = strerror(errno);
		} else if (!mount  && !S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)) {
			err = "not a directory nor regular file";
		} else if (mount && i==2 && !S_ISDIR(st.st_mode)) {
			err = "not a directory";
		} else if (mount && i==1 && !S_ISREG(st.st_mode)) {
			err = "not a regular file";
		} else if (st.st_uid != uid) {
			err = "you are not an owner";
		}

		if (err) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i], err);
			return 1;
		}
	}

	return 0;
}



static inline const char *mnt_opts(uid_t uid) {
	static char buffer[sizeof(MNT_OPTS)+50];
	sprintf(buffer, "%s,uid=%lu", MNT_OPTS, (unsigned long)uid);
	return buffer;
}
