/*
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - RFB layer
   Copyright (C) Matthew Chapman 1999-20010
   
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

#include "rdesktop.h"

static uint8 challenge[16];
static uint16 width;
static uint16 height;
static uint8 bpp;
static uint8 depth;
static uint8 bigendian;
static uint8 truecolour;
static uint16 redmax;
static uint16 greenmax;
static uint16 bluemax;
static uint8 redshift;
static uint8 greenshift;
static uint8 blueshift;

char vnccolormap[768];

/* Send a version message */
static void
rfb_send_version()
{
	STREAM s;

	s = vnc_init(12);

	out_uint8(s, 'R');	/* version */
	out_uint8(s, 'F');	/* version */
	out_uint8(s, 'B');	/* version */
	out_uint8(s, ' ');	/* version */
	out_uint8(s, '0');	/* version */
	out_uint8(s, '0');	/* version */
	out_uint8(s, '3');	/* version */
	out_uint8(s, '.');	/* version */
	out_uint8(s, '0');	/* version */
	out_uint8(s, '0');	/* version */
	out_uint8(s, '3');	/* version */
	out_uint8(s, '\n');	/* version */
	s_mark_end(s);
	vnc_send(s);
}

/* Send a response message */
static void
rfb_send_response(char *password)
{
	STREAM s;

	s = vnc_init(16);

	vncEncryptBytes(challenge, password);
	out_uint8a(s, challenge, 16);	/* challenge */
	s_mark_end(s);
	vnc_send(s);
}

/* Send a clientinit message */
static void
rfb_send_clientinit()
{
	STREAM s;

	s = vnc_init(1);

	out_uint8(s, 1);	/* shared flag */
	s_mark_end(s);
	vnc_send(s);
}

/* Send pixelformat */
static BOOL
rfb_send_pixelformat()
{
	STREAM s;

	s = vnc_init(20);

	out_uint8(s, 0);	/* message type */
	out_uint8(s, 0);	/* padding */
	out_uint16(s, 0);	/* padding */
	out_uint8(s, 8);	/* bits per pixel */
	out_uint8(s, 8);	/* depth */
	out_uint8(s, 0);	/* bigendian */
	out_uint8(s, 1);	/* truecolour */
	out_uint16_be(s, 7);	/* redmax */
	out_uint16_be(s, 7);	/* greenmax */
	out_uint16_be(s, 3);	/* bluemax */
	out_uint8(s, 0)		/* redshift */
	    out_uint8(s, 3);	/* greenshift */
	out_uint8(s, 6);	/* blueshift */
	out_uint8s(s, 3);	/* padding */

	s_mark_end(s);
	vnc_send(s);
}

/* Send colormapmapentries type 1 (NOT IMPLEMENTED) */

/* Send encodings */
static void
rfb_send_encodings()
{
	STREAM s;

	s = vnc_init(24);	/* 4 + 5 * 4 */

	out_uint8(s, 2);	/* message type */
	out_uint8(s, 0);	/* padding */
	out_uint16_be(s, 5);	/* number between 1 and 5 */

	out_uint32_be(s, 1);	/*  copy rectangle encoding */
	out_uint32_be(s, 0);	/*  raw encoding (always present !!!) */
	out_uint32_be(s, 5);	/*  hextile encoding */
	out_uint32_be(s, 4);	/*  CoRRE encoding */
	out_uint32_be(s, 2);	/*  RRE encoding */

	s_mark_end(s);
	vnc_send(s);
}

/* Send updaterequest */
void
rfb_send_updaterequest(x_off, y_off, width, height, incremental)
{
	STREAM s;

	s = vnc_init(10);	/* size */

	out_uint8(s, 3);	/* message type */
	out_uint8(s, incremental);	/* incremental */
	out_uint16_be(s, x_off);	/* x position */
	out_uint16_be(s, y_off);	/* y position */
	out_uint16_be(s, width);	/* width */
	out_uint16_be(s, height);	/* height */

	s_mark_end(s);
	vnc_send(s);
}

/* Send keyevent */
void
rfb_send_keyevent(uint32 keycode, uint32 downflag)
{
	STREAM s;

	s = vnc_init(8);	/* size */

	out_uint8(s, 4);	/* message type */
	out_uint8(s, downflag);	/* downflag */
	out_uint16_be(s, 0);	/* padding */
	out_uint32_be(s, keycode);	/* keycode */

	s_mark_end(s);
	vnc_send(s);
}

/* Send pointerevent */
void
rfb_send_pointerevent(uint32 x, uint32 y, uint32 buttonmask)
{
	STREAM s;

	s = vnc_init(6);	/* size */

	out_uint8(s, 5);	/* message type */
	out_uint8(s, buttonmask);	/* buttonmask */
	out_uint16_be(s, x);	/* x position */
	out_uint16_be(s, y);	/* y position */

	s_mark_end(s);
	vnc_send(s);
}

/* Send clientcuttext */
void
rfb_send_clientcuttext(uint8 * text, uint32 length)
{
	STREAM s;

	s = vnc_init(8 + length);	/* size */

	out_uint8(s, 6);	/* message type */
	out_uint8(s, 0);	/* padding */
	out_uint16(s, 0);	/* padding */
	out_uint32_be(s, length);	/* length */
	out_uint8a(s, text, length);	/* text */

	s_mark_end(s);
	vnc_send(s);
}

/* Expect a version message on the RFB layer */
static BOOL
rfb_recv_version()
{
	STREAM s;
	uint8 version[13];

	s = vnc_recv(12);
	if (s == NULL)
		return False;

	in_uint8a(s, version, 12);
	if (strncmp(version, "RFB 003.003\n", 12)) {
		error("rfb version v%s\n", version);
		return False;
	}
	return True;
}

/* Expect a auth message on the RFB layer */
static BOOL
rfb_recv_auth(uint32 * code)
{
	STREAM s;

	s = vnc_recv(4);
	if (s == NULL)
		return False;

	in_uint32_be(s, *code);
	if (*code > 2) {
		error("rfb auth %d\n", *code);
		return False;
	}
	return True;
}

/* Expect a failed message on the RFB layer */
static BOOL
rfb_recv_failed()
{
	STREAM s;
	uint32 length;
	uint8 *reason;

	s = vnc_recv(4);
	if (s == NULL)
		return False;

	in_uint32_be(s, length);
	if (length > 0) {
		s = vnc_recv(length);
		if (s == NULL)
			return False;

		reason = xmalloc(length + 1);
		in_uint8a(s, reason, length);
		reason[length] = 0;
		error("rfb connection failed %s\n", reason);
		xfree(reason);
		return False;
	}
	return True;
}

/* Expect a challenge message on the RFB layer */
static BOOL
rfb_recv_challenge()
{
	STREAM s;

	s = vnc_recv(16);
	if (s == NULL)
		return False;

	in_uint8a(s, challenge, 16);

	return True;
}

/* Expect a status message on the RFB layer */
static BOOL
rfb_recv_status(uint32 * code)
{
	STREAM s;

	s = vnc_recv(4);
	if (s == NULL)
		return False;

	in_uint32_be(s, *code);
	if (*code > 2) {
		error("rfb status %d\n", *code);
		return False;
	}
	return True;
}

/* Expect a serverinit message on the RFB layer */
static BOOL
rfb_recv_serverinit()
{
	STREAM s;
	uint8 *name;
	uint32 length;
	int i;

	s = vnc_recv(24);
	if (s == NULL)
		return False;

	in_uint16_be(s, width);	/* 640 */
	in_uint16_be(s, height);	/* 480 */
	in_uint8(s, bpp);	/* 8 */
	in_uint8(s, depth);	/* 8 */
	in_uint8(s, bigendian);	/* 0 */
	in_uint8(s, truecolour);	/* 1 */
	in_uint16_be(s, redmax);	/* 7 */
	in_uint16_be(s, greenmax);	/* 7 */
	in_uint16_be(s, bluemax);	/* 3 */
	in_uint8(s, redshift);	/* 0 */
	in_uint8(s, greenshift);	/* 3 */
	in_uint8(s, blueshift);	/* 6 */
	in_uint8s(s, 3);	/* padding */
	in_uint32_be(s, length);
	if (length > 0) {
		s = vnc_recv(length);
		if (s == NULL)
			return False;

		name = xmalloc(length + 1);
		in_uint8a(s, name, length);
		name[length] = 0;
		DEBUG("rfb connection ok %s\n", name);
		xfree(name);
	}
	for (i = 0; i < 256; i++) {
		vnccolormap[3 * i] =
		    ((i >> redshift) & redmax) * (256 / (redmax + 1));
		vnccolormap[3 * i + 1] =
		    ((i >> greenshift) & greenmax) * (256 / (greenmax + 1));
		vnccolormap[3 * i + 2] =
		    ((i >> blueshift) & bluemax) * (256 / (bluemax + 1));
	}
	rdp_send_palette();

	return True;
}

/* Receive a message on the RFB layer, return code */
static STREAM
rfb_recv_rawrect(uint16 x, uint16 y, uint16 width, uint16 height)
{
	STREAM s;
	char *buffer;
	int i;
	int lines;
	int lwidth;

	lwidth = (width + 3) & ~3;
	buffer = xmalloc(lwidth * height);
	s = vnc_recv(width * height);
	if (s == NULL) {
		xfree(buffer);
		return False;
	}

	for (i = 0; i < height; i++) {
//    in_uint8a(s, buffer, width * height );
		in_uint8a(s, buffer + lwidth * (height - i - 1), width);
	}
	/* avoid stupid bug */
	if ((lwidth * height) > 60000) {
		i = y;
		lines = 60000 / lwidth;
		while (height > lines) {
			height -= lines;
			DEBUG("update %d, %d, %d, %d, size:%d\n", x, i, width,
			      lines, height * lwidth);
			rdp_send_rawrect(x, i, width, lines,
					 &buffer[height * lwidth],
					 lines * lwidth);
			i += lines;
		}
		if (height > 0) {
			DEBUG("update %d, %d, %d, %d, last:%d\n", x, i, width,
			      height, 0);
			rdp_send_rawrect(x, i, width, height, buffer,
					 height * lwidth);
		}
	} else {
		DEBUG("update %d, %d, %d, %d, only:%d\n", x, y, width, height,
		      0);
		rdp_send_rawrect(x, y, width, height, buffer, height * lwidth);
	}
	xfree(buffer);

	return s;
}

/* Receive a message on the RFB layer, return code */
static STREAM
rfb_recv_cpyrect(uint16 x, uint16 y, uint16 width, uint16 height)
{
	STREAM s;
	uint16 x_src;
	uint16 y_src;

	s = vnc_recv(4);
	if (s == NULL)
		return False;

	in_uint16_be(s, x_src);	/* x position */
	in_uint16_be(s, y_src);	/* y position */
	DEBUG("copy %d, %d, %d, %d, %d, %d\n", x_src, y_src, x, y, width,
	      height);
	rdp_send_cpyrect(x_src, y_src, x, y, width, height);

	return s;
}

/* Receive a message on the RFB layer, return code */
static STREAM
rfb_recv_rre(uint16 x, uint16 y, uint16 width, uint16 height)
{
	STREAM s;
	uint16 x_src;
	uint16 y_src;
	int rectangles;
	int colour;

	s = vnc_recv(5);
	if (s == NULL)
		return False;

	in_uint32_be(s, rectangles);	/* rectangles */
	in_uint8(s, colour);	/* colour */
	DEBUG("RECT %d*%d+%d+%d, %d, %02x\n", width, height, x, y, rectangles,
	      colour);
	rdp_send_rect(x, y, width, height, colour);
	while (rectangles--) {
		s = vnc_recv(9);
		if (s == NULL)
			return False;

		in_uint8(s, colour);	/* colour */
		in_uint16_be(s, x_src);	/* x position */
		in_uint16_be(s, y_src);	/* y position */
		in_uint16_be(s, width);	/* width */
		in_uint16_be(s, height);	/* height */
		DEBUG("RECT %d*%d+%d+%d, %02x\n", width, height, x + x_src,
		      y + y_src, colour);
		rdp_send_rect(x + x_src, y + y_src, width, height, colour);
	}

	return s;
}

/* Receive a message on the RFB layer, return code */
static STREAM
rfb_recv_corre(uint16 x, uint16 y, uint16 width, uint16 height)
{
	STREAM s;
	uint16 x_src;
	uint16 y_src;
	int rectangles;
	int colour;

	s = vnc_recv(5);
	if (s == NULL)
		return False;

	in_uint32_be(s, rectangles);	/* rectangles */
	in_uint8(s, colour);	/* colour */
	DEBUG("rect %d*%d+%d+%d, %d, %02x\n", width, height, x, y, rectangles,
	      colour);
	rdp_send_rect(x, y, width, height, colour);
	while (rectangles--) {
		s = vnc_recv(5);
		if (s == NULL)
			return False;

		in_uint8(s, colour);	/* colour */
		in_uint8(s, x_src);	/* x position */
		in_uint8(s, y_src);	/* y position */
		in_uint8(s, width);	/* width */
		in_uint8(s, height);	/* height */
		DEBUG("rect %d*%d+%d+%d, %02x\n", width, height, x + x_src,
		      y + y_src, colour);
		rdp_send_rect(x + x_src, y + y_src, width, height, colour);
	}

	return s;
}

/* Receive a message on the RFB layer, return code */
static STREAM
rfb_recv_hextile(uint16 x, uint16 y, uint16 width, uint16 height)
{
	STREAM s;
	int i, j, k;
	int colour;
	int flag;
	int tile_x;
	int tile_y;
	int line;
	unsigned char tile_buf[256];
	int foreground;
	int background;
	int subrectangles;
	int sub_x, sub_y, sub_w, sub_h;

	DEBUG("hextile %d*%d+%d+%d\n", width, height, x, y);
	for (i = 0; i < height; i += 16) {
		for (j = 0; j < width; j += 16) {
			tile_x = width - j > 16 ? 16 : width - j;
			tile_y = height - i > 16 ? 16 : height - i;

			s = vnc_recv(1);
			if (s == NULL)
				return False;

			in_uint8(s, flag);	/* flag */
			if (flag & 1) {	/* raw */
				s = vnc_recv(tile_x * tile_y);
				if (s == NULL)
					return False;

				line = (tile_x + 3) & ~3;
				for (k = 0; k < tile_y; k++)
					in_uint8a(s,
						  tile_buf + line * (tile_y -
								     k - 1),
						  tile_x);
				rdp_send_rawrect(x + j, y + i, tile_x, tile_y,
						 tile_buf, line * tile_y);
				DEBUG("  raw %d*%d+%d+%d, %d\n", tile_x, tile_y,
				      x + j, y + i, line * tile_y);
				continue;
			}
			if (flag & 2) {	/* background */
				s = vnc_recv(1);
				if (s == NULL)
					return False;

				in_uint8(s, background);	/* background */
			}
			DEBUG("  back %d*%d+%d+%d, %d\n", tile_x, tile_y, x + j,
			      y + i, background);
			rdp_send_rect(x + j, y + i, tile_x, tile_y, background);
			if (flag & 4) {	/* foreground */
				s = vnc_recv(1);
				if (s == NULL)
					return False;

				in_uint8(s, foreground);	/* foreground */
				DEBUG("  fore %d*%d+%d+%d, %d\n", tile_x,
				      tile_y, x + j, y + i, foreground);
			}
			if (flag & 8) {	/* subrectangles */
				s = vnc_recv(1);
				if (s == NULL)
					return False;

				in_uint8(s, subrectangles);	/* subrectangles */
				DEBUG("  subr %d*%d+%d+%d, %d\n", tile_x,
				      tile_y, x + j, y + i, subrectangles);
			} else
				continue;
			while (subrectangles--) {
				s = vnc_recv(flag & 0x10 ? 3 : 2);
				if (s == NULL)
					return False;

				if (flag & 0x10)
					in_uint8(s, colour);	/* colour */
				in_uint8(s, sub_x);	/* x, y */
				in_uint8(s, sub_w);	/* w, h */
				DEBUG
				    ("    rc %d*%d+%d+%d, %d, %02x, %02x, %02x, %02x\n",
				     tile_x, tile_y, x + j, y + i,
				     tile_x * tile_y, sub_x, sub_w, flag,
				     colour);
				sub_y = sub_x & 0x0f;
				sub_x = (sub_x >> 4) & 0x0f;
				sub_h = (sub_w & 0x0f) + 1;
				sub_w = ((sub_w >> 4) & 0x0f) + 1;
				if (flag & 0x10)
					rdp_send_rect(x + j + sub_x,
						      y + i + sub_y, sub_w,
						      sub_h, colour);
				else
					rdp_send_rect(x + j + sub_x,
						      y + i + sub_y, sub_w,
						      sub_h, foreground);
			}
		}
	}

	return s;
}

/* Receive a message on the RFB layer, return code */
static STREAM
rfb_recv_colourmap()
{
	STREAM s;
	int i;
	uint16 first;
	uint16 number;
	uint16 red;
	uint16 green;
	uint16 blue;

	s = vnc_recv(5);
	if (s == NULL)
		return False;

	in_uint8s(s, 1);	/* padding */
	in_uint16_be(s, first);	/* x position */
	in_uint16_be(s, number);	/* y position */

	for (i = 0; i < number; i++) {
		s = vnc_recv(6);
		if (s == NULL)
			return False;

		in_uint16_be(s, red);	/* red */
		in_uint16_be(s, green);	/* green */
		in_uint16_be(s, blue);	/* blue */

		/* do something with this data ? */

		DEBUG("colourmap %d, %d, %d\n", red, green, blue);
	}
	return s;
}

/* Expect a version message on the RFB layer */
static BOOL
rfb_recv_servercuttext()
{
	STREAM s;
	uint32 length;
	uint8 *servercuttext;

	s = vnc_recv(7);
	if (s == NULL)
		return False;

	in_uint8s(s, 3);	/* padding */
	in_uint32_be(s, length);	/* length */
	servercuttext = xmalloc(length + 1);
	in_uint8a(s, servercuttext, length);
	servercuttext[length] = 0;
	/* do something with this data */
	xfree(servercuttext);

	return True;
}

/* Receive a message on the RFB layer, return code */
static STREAM
rfb_recv_msg(uint8 * code)
{
	STREAM s;
	uint16 number;
	uint16 x;
	uint16 y;
	uint16 width;
	uint16 height;
	uint16 encoding;
	int lwidth;

	s = vnc_recv(1);
	if (s == NULL)
		return False;

	in_uint8(s, *code);
	if (*code == 0) {	/* Framebuffer update */
		s = vnc_recv(3);
		if (s == NULL)
			return False;

		in_uint8s(s, 1);	/* pad */
		in_uint16_be(s, number);
		DEBUG("number = %d\n", number);
		while (number--) {
			s = vnc_recv(12);
			if (s == NULL)
				return False;

			in_uint16_be(s, x);	/* x position */
			in_uint16_be(s, y);	/* y position */
			in_uint16_be(s, width);	/* width */
			in_uint16_be(s, height);	/* height */
			in_uint32_be(s, encoding);	/* encoding */
			DEBUG("update %d, %d, %d, %d, type: %d, code:%d\n", x,
			      y, width, height, encoding, *code);

			lwidth = (width + 3) & ~3;
			DEBUG("lwidth = %d\n", lwidth);
			switch (encoding) {
			case 0:	/* RAW */
				rfb_recv_rawrect(x, y, width, height);
				break;
			case 1:	/* copyrect */
				rfb_recv_cpyrect(x, y, width, height);
				break;
			case 2:	/* RRE */
				rfb_recv_rre(x, y, width, height);
				break;
			case 4:	/* CoRRE */
				rfb_recv_corre(x, y, width, height);
				break;
			case 5:	/* hextile */
				rfb_recv_hextile(x, y, width, height);
				break;
			default:
				DEBUG("unknown encoding %d\n", encoding);
			}
//        rfb_send_updaterequest ( 0, 0, 640, 480, 1 );
		}
	} else if (*code == 1) {	/* colourmap */
		rfb_recv_colourmap();
		DEBUG("colourmap\n");
	} else if (*code == 2) {	/* bell */
		/* ring a bell */
		DEBUG("bell\n");
	} else if (*code == 3) {	/* cut text */
		rfb_recv_servercuttext();
		DEBUG("cut text\n");
	} else {		/* undefined */

		/* undefined */
		DEBUG("undefined %d\n", *code);
	}

	return s;
}

/* Initialise RFB transport data packet */
STREAM
rfb_init(int length)
{
	STREAM s;

	s = vnc_init(length);

	return s;
}

/* Send an RFB data PDU */
void
rfb_send(STREAM s)
{
	vnc_send(s);
}

/* Receive RFB transport data packet */
STREAM
rfb_recv()
{
	STREAM s;
	uint8 code;

	s = rfb_recv_msg(&code);
	if (s == NULL) {
		error("server receive problem %d\n", code);
		return False;
	}

	return s;
}

/* Establish a connection up to the RFB layer */
BOOL
rfb_connect(char *server, char *password)
{
	uint32 code;

	if (!vnc_connect(server))
		return False;

	if (!rfb_recv_version())
		goto error;

	rfb_send_version();

	if (!rfb_recv_auth(&code))
		goto error;

	if (code == 0) {
		if (!rfb_recv_failed())
			goto error;

		error("vnc connection failed\n");
		goto error;
	} else if (code == 2) {
		if (!rfb_recv_challenge())
			goto error;

		rfb_send_response(password);
		if (!rfb_recv_status(&code))
			goto error;
		if (code != 0) {
			error("expected 0, got %d\n", code);
			vnc_disconnect();
			return False;
		}
	}

	rfb_send_clientinit();

	if (!rfb_recv_serverinit())
		goto error;

	rfb_send_encodings();
	rfb_send_updaterequest(0, 0, 640, 480, 0);

	return True;

      error:

	vnc_disconnect();
	return False;
}

/* Disconnect from the ISO layer */
void
rfb_disconnect()
{
	vnc_disconnect();
}
