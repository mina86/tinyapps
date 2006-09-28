/*
 * mountiso - Mounts/unmounts ISO images
 * $Id: mountiso.c,v 1.3 2006/09/28 15:06:19 mina86 Exp $
 * Copyright (c) 2005 by Michal Nazareicz (mina86/AT/mina86.com)
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

/*
 * To install run (as an ordinary user):
 *
 * $ make mountios
 * < should compile w/o errors >
 * $ su
 * < should ask for root password >
 * # chown root:bin mountiso
 * # chmod 4755 mountiso
 * # ln -s mountiso umountiso
 * # cp mountiso umountiso /bin
 * # exit
 */

#include <unistd.h>
#include <stdio.h>


#define DEFAULT_DIR "/mnt/cdrom"
#define MOUNT       "/bin/mount"
#define UMOUNT      "/bin/umount"
#define MNT_OPTS    "loop,ro,nosuid,noexec,nodev,owner"


int main(int argc, char **argv) {
	char *arg0 = *argv, *c = arg0, mount;

	/* Get program name */
	while (*c) if (*c++=='/') arg0 = c;
	mount = arg0[0]!='u' ? 1 : 0;

	/* Usage */
	if ((argc!=(1+mount) && argc!=(2+mount)) || (argc>1 && argv[1][0]=='-')) {
		fputs("usage: mountiso image.iso [ directory ]\n"
			  "       umountiso [ image.iso | directory ]\n", stderr);
		return 1;
	}

	/* Must be root */
	if (getuid() && geteuid()) {
		fprintf(stderr, "%s: must be SUIDed to work\n", arg0);
		return 1;
	}

	/* mount/umount */
	if (setuid(0)) perror("setuid");
	argv[argc] = DEFAULT_DIR;
	if (mount) {
		execl(MOUNT, MOUNT, "-o", MNT_OPTS, "--", argv[1], argv[2], NULL);
	} else {
		execl(UMOUNT, UMOUNT, "-d", "--", argv[1], NULL);
	}
	perror(mount ? "exec: " MOUNT : "exec: " UMOUNT);
}
