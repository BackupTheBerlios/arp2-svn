/*
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - VNC layer
   Copyright (C) Matthew Chapman 1999-2001
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <unistd.h>		/* select read write close */
#include <sys/socket.h>		/* socket connect setsockopt */
#include <sys/time.h>		/* timeval */
#include <netdb.h>		/* gethostbyname */
#include <netinet/in.h>		/* sockaddr_in */
#include <netinet/tcp.h>	/* TCP_NODELAY */
#include <arpa/inet.h>		/* inet_addr */
#include <errno.h>		/* errno */
#include "rdesktop.h"

static int sock;
static struct stream in;
static struct stream out;
extern int vnc_port_rdp;

int
vnc_socket()
{
	return (sock);
}

/* Initialise VNC transport data packet */
STREAM
vnc_init(int maxlen)
{
	if (maxlen > out.size) {
		out.data = xrealloc(out.data, maxlen);
		out.size = maxlen;
	}

	out.p = out.data;
	out.end = out.data + out.size;
	return &out;
}

/* Send VNC transport data packet */
void
vnc_send(STREAM s)
{
	int length = s->end - s->data;
	int sent, total = 0;

	while (total < length) {
		sent = write(sock, s->data + total, length - total);

#ifdef WITH_DEBUG_RDP
		DEBUG("vnc send\n");
		hexdump(s->data + total, length - total);
#endif

		if (sent <= 0) {
			STATUS("write: %s\n", strerror(errno));
			return;
		}

		total += sent;
	}
}

/* Receive a message on the VNC layer */
STREAM
vnc_recv(int length)
{
	int rcvd = 0;

	if (length > in.size) {
		in.data = xrealloc(in.data, length);
		in.size = length;
	}

	in.end = in.p = in.data;

	while (length > 0) {

		DEBUG("length=%d\n", length);
		rcvd = read(sock, in.end, length);

		if (rcvd <= 0) {
			STATUS("vnc_recv read: %s\n", strerror(errno));
			return NULL;
		}
#ifdef WITH_DEBUG_RDP
		DEBUG("vnc recv\n");
		hexdump(in.end, rcvd);
#endif
		in.end += rcvd;
		length -= rcvd;
	}
	return &in;
}

/* Establish a connection on the VNC layer */
BOOL
vnc_connect(char *server)
{
	struct hostent *nslookup;
	struct sockaddr_in servaddr;
	int true = 1;
	char *index;

	if ( (index = rindex(server, ':')) ) {
		vnc_port_rdp = 5900 + atoi(index + 1);
		*index = 0;
		fprintf(stderr, "host: %s, port: %d\n", server, vnc_port_rdp);
		fflush(stderr);
	}
	if ((nslookup = gethostbyname(server)) != NULL) {
		memcpy(&servaddr.sin_addr, nslookup->h_addr,
		       sizeof (servaddr.sin_addr));
	} else if (!(servaddr.sin_addr.s_addr = inet_addr(server))) {
		STATUS("%s: unable to resolve host\n", server);
		return False;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		STATUS("socket: %s\n", strerror(errno));
		return False;
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(vnc_port_rdp);

	if (connect
	    (sock, (struct sockaddr *) &servaddr, sizeof (struct sockaddr)) < 0) {
		STATUS("connect: %s\n", strerror(errno));
		close(sock);
		return False;
	}

	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *) &true,
		   sizeof (true));

	in.size = 8192;
	in.data = xmalloc(in.size);

	out.size = 4096;
	out.data = xmalloc(out.size);

	return True;
}

/* Disconnect on the VNC layer */
void
vnc_disconnect()
{
	close(sock);
}
