/*
 * Address Conflict Detection implementation
 * Copyright (c) 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
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
 * Address Conflict Detection is described in
 * draft-cheshire-ipv4-acd-06.txt Internet-Draft which can be found at
 * ftp://ftp.rfc-editor.org/in-notes/internet-drafts/draft-cheshire-ipv4-acd-06.txt
 *
 * It is not fully compatibl with the draft since it ignores any other
 * probes of given address that someone may send but that's because
 * this tool is ment for something slightly different (it's a good
 * starting point for full ACD implementation though).
 */

#define _BSD_SOURCE 1

#include <arpa/inet.h>
#include <errno.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


/* http://tools.ietf.org/html/rfc826 */
struct arp {
	/* Ethernet transmission layer */
	uint8_t  dest[6];    /* Ethernet address of destination */
	uint8_t  source[6];  /* Ethernet address of sender */
	uint16_t proto;      /* Protocol type = htons(ETH_P_ARP) */

	/* Ethernet packet data */
	uint16_t htype;      /* Hardware address space = htons(ARPHRD_ETHER); */
	uint16_t ptype;      /* Protocol address space = htons(ETH_P_IP); */
	uint8_t  hlen;       /* byte length of each hardware address = 6 */
	uint8_t  plen;       /* byte length of each protocol address = 8 */
	uint16_t op;         /* opcode (ares_op$REQUEST | ares_op$REPLY) */
	uint8_t  sha[6];     /* Hardware address of sender of this packet */
	uint8_t  spa[4];     /* Protocol address of sender of this packet */
	uint8_t  tha[6];     /* Hardware address of target of this packet */
	uint8_t  tpa[4];     /* Protocol address of target */

	/* Don't change spa and tpa to uint32_t as then compiler will try
	   to align those elements and __attribute__((packed)) will be
	   required and we might be compiled using compiler which does not
	   support it.  Moreover, after packing structure spa and tpa
	   would not be aligned thus on some platforms writing to or
	   reading from those varaibles will cause an exception. */

	/* Padding */
	uint8_t  pad[18];    /* Pad for min. ethernet payload (60 bytes) */
} /* __attribute__((packed)) */;

#define ARP_MSG_SIZE 0x2a

struct static_asserts {
	/* If compiler shows error here it means arp structure has invalid
	   size.  Try uncomenting __attribute__((packed)) few lines
	   above. */
	char struct_arp_has_wrong_size[sizeof(struct arp) - sizeof ((struct arp*)0)->pad == ARP_MSG_SIZE ? 1 : -1];
};



#define ERR(msg1, msg2) fprintf(stderr, "%s: %s: %s\n", *argv, (msg1), (msg2))
#define PERR(msg) ERR(msg, strerror(errno))
#define DIE(n, msg1, msg2) (ERR(msg1, msg2), exit(n))
#define PDIE(n, msg) DIE(n, msg, strerror(errno))



int main(int argc, char **argv) {
	static const int one = 1;

	struct timeval tv = { 0, 0 };
	struct sockaddr addr;
	int s, count = 0;
	struct arp arp;


	/* Parse arguments */
	if (argc != 3) {
		fprintf(stderr, "usage: %s <interface> <ip>\n", *argv);
		return 3;
	}

	{
		uint32_t ip = inet_addr(argv[2]);
		if (ip == INADDR_NONE) {
			DIE(3, argv[2], "invalid IP address");
		}
		memcpy(arp.tpa, &ip, sizeof arp.tpa);
	}


	/* Prepare socket */
	s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));
	if (s < 0) {
		PDIE(2, "socket");
	}

	if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE,
	               argv[1], strlen(argv[1]))<0) {
		PDIE(2, argv[1]);
	}

	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof one) < 0) {
		PDIE(2, "setting broadcast flag");
	}


	/* Get our hardware address */
	memset(&addr, 0, sizeof addr);
	strncpy(addr.sa_data, argv[1], sizeof addr.sa_data);
	{
		struct ifreq ifreq;
		strncpy(ifreq.ifr_name, addr.sa_data, sizeof ifreq.ifr_name);
		if (ioctl(s, SIOCGIFHWADDR, &ifreq) < 0) {
			PDIE(2, "ioctl: get hardware address");
		}
		memcpy(arp.source, ifreq.ifr_hwaddr.sa_data, sizeof arp.source);
	}


	/* Prepare ARP Probe */
	memset(arp.dest, 0xff, sizeof arp.dest);
	/* memcpy(arp.source, ..., 6); -- already set*/
	arp.proto = htons(ETH_P_ARP);
	arp.htype = htons(ARPHRD_ETHER);
	arp.ptype = htons(ETH_P_IP);
	arp.hlen = sizeof arp.sha;
	arp.plen = sizeof arp.spa;
	arp.op = htons(ARPOP_REQUEST);
	memcpy(arp.sha, arp.source, sizeof arp.sha);
	memset(arp.spa, 0, sizeof arp.spa);
	memset(arp.tha, 0, sizeof arp.tha);
	/* addr.tpa = ...; -- already set */


	/* Initial random delay */
	srand(time(0));
	tv.tv_sec = 0;
	tv.tv_usec = rand() % 1000;
	select(0, 0, 0, 0, &tv);


	/* Query */
	do {
		/* Send ARP Probe */
		if (sendto(s, &arp, sizeof arp, 0, &addr, sizeof addr) < 0) {
			PDIE(2, "send");
		}

		/* Random delay until repeated probes */
		if (++count == 3) { /* that's the last one */
			tv.tv_sec = 2;
			tv.tv_usec = 0;
		} else {            /* there are more to go */
			tv.tv_sec = 1;
			tv.tv_usec = rand() % 1000;
		}

		/* Wait for ARP reply */
		for(;;){
			struct arp arp_rec;
			fd_set fds;
			int ret;

			FD_ZERO(&fds);
			FD_SET(s, &fds);
			ret = select(s + 1, &fds, 0, 0, &tv);

			if (ret == 0) {
				break;
			} else if (ret < 0) {
				PDIE(2, "select");
			} else if ((ret = read(s, &arp_rec, sizeof arp_rec)) < 0) {
				PDIE(2, "read");
			} else if (ret >= ARP_MSG_SIZE &&
			           !memcmp(arp_rec.spa, arp.tpa, sizeof arp.tpa)) {
				printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
				       arp_rec.sha[0], arp_rec.sha[1], arp_rec.sha[2],
				       arp_rec.sha[3], arp_rec.sha[4], arp_rec.sha[5]);
				close(s);
				return 0;
			}
		}
	} while (count < 3);

	close(s);
	return 1;
}
