/*
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - RDP layer
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
#include <sys/time.h>
#include <errno.h>

#include "rdesktop.h"
extern uint16 mcs_userid;
extern char *username;
extern BOOL bitmap_compression;
extern BOOL orders;
extern BOOL use_encryption;
extern BOOL w2k;
extern BOOL desktop_save;
extern char rdp_bitmap_bits;

uint8 *next_packet;
static uint32 rdp_shareid;

/* flag to reset order structures only the first time */
static int first_demand = 1;

/* Initialise an RDP packet */
static STREAM
rdp_init(int maxlen)
{
	STREAM s;

	s = sec_init(use_encryption ? SEC_ENCRYPT : 0, maxlen + 6);
	s_push_layer(s, rdp_hdr, 6);
	return s;
}

/* Send an RDP packet */
static void
rdp_send(STREAM s, uint8 pdu_type)
{
	uint16 length;

	s_pop_layer(s, rdp_hdr);
	length = s->end - s->p;

	out_uint16_le(s, length);
	out_uint16_le(s, (pdu_type | 0x10));	/* Version 1 */
	out_uint16_le(s, (mcs_userid + 1001));

	sec_send(s, use_encryption ? SEC_ENCRYPT : 0);
}

static STREAM rdp_s;

/* Receive an RDP packet */
static STREAM
rdp_recv(uint8 * type)
{
	uint16 length, pdu_type;
	if ((rdp_s == NULL) || (next_packet >= rdp_s->end)) {
		rdp_s = sec_recv();
		if (rdp_s == NULL)
			return NULL;
		next_packet = rdp_s->p;
	}

	else {
		rdp_s->p = next_packet;
	}
	in_uint16_le(rdp_s, length);
	in_uint16_le(rdp_s, pdu_type);
	in_uint8s(rdp_s, 2);	/* userid */
	*type = pdu_type & 0xf;

#if WITH_DEBUG_RDP
	DEBUG("RDP packet (type %x):\n", *type);
	hexdump(next_packet, length);
#endif				/*  */

	next_packet += length;
	return rdp_s;
}

/* Initialise an RDP data packet */
static STREAM
rdp_init_data(int maxlen)
{
	STREAM s;

	s = sec_init(use_encryption ? SEC_ENCRYPT : 0, maxlen + 18);
	s_push_layer(s, rdp_hdr, 18);
	return s;
}

/* Send an RDP data packet */
static void
rdp_send_data(STREAM s, uint8 data_pdu_type)
{
	uint16 length;
	s_pop_layer(s, rdp_hdr);
	length = s->end - s->p;
	out_uint16_le(s, length);
	out_uint16_le(s, (RDP_PDU_DATA | 0x10));
	out_uint16_le(s, (mcs_userid + 1001));
	out_uint32_le(s, rdp_shareid);
	out_uint8(s, 0);	/* pad */
	out_uint8(s, 1);	/* streamid */
	out_uint16(s, (length - 14));
	out_uint8(s, data_pdu_type);
	out_uint8(s, 0);	/* compress_type */
	out_uint16(s, 0);	/* compress_len */

	sec_send(s, use_encryption ? SEC_ENCRYPT : 0);
}

/* Output a string in Unicode */
void
rdp_out_unistr(STREAM s, char *string, int len)
{
	int i = 0, j = 0;

	/* don't assume that the string exists if it is
	   of zero length.
	   We still need to write it though....
	 */
	len += 2;
	if (string != 0)
		while (i < len) {
			s->p[i++] = string[j++];
			s->p[i++] = 0;
	} else
		while (i < len) {
			s->p[i++] = 0;
			s->p[i++] = 0;
		}

	s->p += len;
}

/* Parse a logon info packet */
static void
rdp_send_logon_info(uint32 flags, char *domain, char *user, char *password,
		    char *program, char *directory)
{
	int len_domain = 2 * (domain == 0 ? 0 : strlen(domain));
	int len_user = 2 * (user == 0 ? 0 : strlen(user));
	int len_password = 2 * (password == 0 ? 0 : strlen(password));
	int len_program = 2 * (program == 0 ? 0 : strlen(program));
	int len_directory = 2 * (directory == 0 ? 0 : strlen(directory));

/*      uint32 sec_flags = SEC_LOGON_INFO | SEC_ENCRYPT; */
	uint32 sec_flags =
	    use_encryption ? (SEC_LOGON_INFO | SEC_ENCRYPT) : SEC_LOGON_INFO;
	STREAM s;
	s =
	    sec_init(sec_flags,
		     18 + len_domain + len_user + len_password + len_program +
		     len_directory + 10);
	out_uint32(s, 0);
	out_uint32_le(s, flags);
	out_uint16_le(s, len_domain);
	out_uint16_le(s, len_user);
	out_uint16_le(s, len_password);
	out_uint16_le(s, len_program);
	out_uint16_le(s, len_directory);
	rdp_out_unistr(s, domain, len_domain);
	rdp_out_unistr(s, user, len_user);
	rdp_out_unistr(s, password, len_password);
	rdp_out_unistr(s, program, len_program);
	rdp_out_unistr(s, directory, len_directory);
	s_mark_end(s);
	sec_send(s, sec_flags);
}

/* Send a control PDU */
static void
rdp_send_control(uint16 action)
{
	STREAM s;
	s = rdp_init_data(8);
	out_uint16_le(s, action);
	out_uint16(s, 0);	/* userid */
	out_uint32(s, 0);	/* control id */
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_CONTROL);
}

/* Send a synchronisation PDU */
static void
rdp_send_synchronise()
{
	STREAM s;
	s = rdp_init_data(4);
	out_uint16_le(s, 1);	/* type */
	out_uint16_le(s, 1002);
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_SYNCHRONISE);
}

/* Send a single input event */
void
rdp_send_input(uint32 time, uint16 message_type, uint16 device_flags,
	       uint16 param1, uint16 param2)
{
	STREAM s;
	s = rdp_init_data(16);
	out_uint16_le(s, 1);	/* number of events */
	out_uint16(s, 0);	/* pad */
	out_uint32_le(s, time);
	out_uint16_le(s, message_type);
	out_uint16_le(s, device_flags);
	out_uint16_le(s, param1);
	out_uint16_le(s, param2);
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_INPUT);
}

/* Send an (empty) font information PDU */
static void
rdp_send_fonts(uint16 seq)
{
	STREAM s;
	s = rdp_init_data(8);
	out_uint16(s, 0);	/* number of fonts */
	out_uint16_le(s, 0x3e);	/* unknown */
	out_uint16_le(s, seq);	/* unknown */
	out_uint16_le(s, 0x32);	/* entry size */
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_FONT2);
}

/* Output general capability set */
static void
rdp_out_general_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_GENERAL);
	out_uint16_le(s, RDP_CAPLEN_GENERAL);
	out_uint16_le(s, 1);	/* OS major type */
	out_uint16_le(s, 3);	/* OS minor type */
	out_uint16_le(s, 0x200);	/* Protocol version */
	out_uint16(s, 0);	/* Pad */
	out_uint16(s, 0);	/* Compression types */
	out_uint16(s, 0);	/* Pad */
	out_uint16(s, 0);	/* Update capability */
	out_uint16(s, 0);	/* Remote unshare capability */
	out_uint16(s, 0);	/* Compression level */
	out_uint16(s, 0);	/* Pad */
}

/* Output bitmap capability set */
static void
rdp_out_bitmap_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_BITMAP);
	out_uint16_le(s, RDP_CAPLEN_BITMAP);
	out_uint16_le(s, 8);	/* Preferred BPP */
	out_uint16(s, 1);	/* Receive 1 BPP */
	out_uint16(s, 1);	/* Receive 4 BPP */
	out_uint16_le(s, 1);	/* Receive 8 BPP */
	out_uint16_le(s, 800);	/* Desktop width */
	out_uint16_le(s, 600);	/* Desktop height */
	out_uint16(s, 0);	/* Pad */
	out_uint16(s, 0);	/* Allow resize */
	out_uint16_le(s, bitmap_compression ? 1 : 0);	/* Support compression */
	out_uint16(s, 0);	/* Unknown */
	out_uint16_le(s, 1);	/* Unknown */
	out_uint16(s, 0);	/* Pad */
}

/* Output order capability set */
static void
rdp_out_order_caps(STREAM s)
{
	uint8 order_caps[32];

	/* memset(order_caps, orders, 32); */
	memset(order_caps, 0, 32);
	order_caps[0] = 1;	/* dest blt */
	order_caps[1] = 1;	/* pat blt */
	order_caps[2] = 1;	/* screen blt */
	order_caps[8] = 1;	/* line */
	order_caps[9] = 1;	/* line */
	order_caps[10] = 1;	/* rect */
	order_caps[11] = (desktop_save == False ? 0 : 1);	/* desksave */
	order_caps[13] = 1;	/* memblt */
	order_caps[14] = 1;	/* triblt */
	order_caps[22] = 1;	/* polyline */
	order_caps[27] = 1;	/* text2 */
	out_uint16_le(s, RDP_CAPSET_ORDER);
	out_uint16_le(s, RDP_CAPLEN_ORDER);
	out_uint8s(s, 20);	/* Terminal desc, pad */
	out_uint16_le(s, 1);	/* Cache X granularity */
	out_uint16_le(s, 20);	/* Cache Y granularity */
	out_uint16(s, 0);	/* Pad */
	out_uint16_le(s, 1);	/* Max order level */
	out_uint16_le(s, 0x147);	/* Number of fonts */
	out_uint16_le(s, 0x2a);	/* Capability flags */
	out_uint8p(s, order_caps, 32);	/* Orders supported */
	out_uint16_le(s, 0x6a1);	/* Text capability flags */
	out_uint8s(s, 6);	/* Pad */
	out_uint32(s, desktop_save == False ? 0 : 0x38400);	/* Desktop cache size */
	out_uint32(s, 0);	/* Unknown */
	out_uint32(s, 0x4e4);	/* Unknown */
}

/* Output bitmap cache capability set */
static void
rdp_out_bmpcache_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_BMPCACHE);
	out_uint16_le(s, RDP_CAPLEN_BMPCACHE);
	out_uint8s(s, 24);	/* unused */
	out_uint16_le(s, 0x258);	/* entries */
	out_uint16_le(s, 0x100);	/* max cell size */
	out_uint16_le(s, 0x12c);	/* entries */
	out_uint16_le(s, 0x400);	/* max cell size */
	out_uint16_le(s, 0x106);	/* entries */
	out_uint16_le(s, 0x1000);	/* max cell size */
}

/* Output control capability set */
static void
rdp_out_control_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_CONTROL);
	out_uint16_le(s, RDP_CAPLEN_CONTROL);
	out_uint16(s, 0);	/* Control capabilities */
	out_uint16(s, 0);	/* Remote detach */
	out_uint16_le(s, 2);	/* Control interest */
	out_uint16_le(s, 2);	/* Detach interest */
}

/* Output activation capability set */
static void
rdp_out_activate_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_ACTIVATE);
	out_uint16_le(s, RDP_CAPLEN_ACTIVATE);
	out_uint16(s, 0);	/* Help key */
	out_uint16(s, 0);	/* Help index key */
	out_uint16(s, 0);	/* Extended help key */
	out_uint16(s, 0);	/* Window activate */
}

/* Output pointer capability set */
static void
rdp_out_pointer_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_POINTER);
	out_uint16_le(s, RDP_CAPLEN_POINTER);
	out_uint16(s, 0);	/* Colour pointer */
	out_uint16_le(s, 20);	/* Cache size */
}

/* Output share capability set */
static void
rdp_out_share_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_SHARE);
	out_uint16_le(s, RDP_CAPLEN_SHARE);
	out_uint16(s, 0);	/* userid */
	out_uint16(s, 0);	/* pad */
}

/* Output colour cache capability set */
static void
rdp_out_colcache_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_COLCACHE);
	out_uint16_le(s, RDP_CAPLEN_COLCACHE);
	out_uint16_le(s, 6);	/* cache size */
	out_uint16(s, 0);	/* pad */
}
static uint8 canned_caps[] =
    { 0x01, 0x00, 0x00, 0x00, 0x09, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x08, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x34, 0x00, 0xFE, 0x00, 0x04, 0x00,
	0xFE, 0x00, 0x04, 0x00, 0xFE, 0x00, 0x08, 0x00, 0xFE, 0x00, 0x08, 0x00,
	0xFE, 0x00, 0x10, 0x00, 0xFE, 0x00, 0x20, 0x00, 0xFE, 0x00, 0x40, 0x00,
	0xFE, 0x00, 0x80, 0x00, 0xFE, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x08,
	0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00
};

/* Output unknown capability set */
static void
rdp_out_unknown_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_UNKNOWN);
	out_uint16_le(s, 0x58);
	out_uint8p(s, canned_caps, RDP_CAPLEN_UNKNOWN - 4);
}

/* Send a confirm active PDU */
static void
rdp_send_confirm_active()
{
	STREAM s;
	uint16 caplen =
	    RDP_CAPLEN_GENERAL + RDP_CAPLEN_BITMAP + RDP_CAPLEN_ORDER +
	    RDP_CAPLEN_BMPCACHE + RDP_CAPLEN_COLCACHE + RDP_CAPLEN_ACTIVATE +
	    RDP_CAPLEN_CONTROL + RDP_CAPLEN_POINTER + RDP_CAPLEN_SHARE +
	    RDP_CAPLEN_UNKNOWN;
     if (w2k)
		caplen += 4;
	s = rdp_init(14 + caplen + sizeof (RDP_SOURCE));
	out_uint32_le(s, rdp_shareid);
	out_uint16_le(s, 0x3ea);	/* userid */
	out_uint16_le(s, sizeof (RDP_SOURCE));
	out_uint16_le(s, caplen);
	out_uint8p(s, RDP_SOURCE, sizeof (RDP_SOURCE));
	out_uint16_le(s, 0xd);	/* num_caps */
	out_uint8s(s, 2);	/* pad */
	rdp_out_general_caps(s);
	rdp_out_bitmap_caps(s);
	rdp_out_order_caps(s);
	rdp_out_bmpcache_caps(s);
	rdp_out_colcache_caps(s);
	rdp_out_activate_caps(s);
	rdp_out_control_caps(s);
	rdp_out_pointer_caps(s);
	rdp_out_share_caps(s);
	rdp_out_unknown_caps(s);
	s_mark_end(s);
	rdp_send(s, RDP_PDU_CONFIRM_ACTIVE);
}

/* Respond to a demand active PDU */
static void
process_demand_active(STREAM s)
{
	uint8 type;
	in_uint32_le(s, rdp_shareid);
	DEBUG("DEMAND_ACTIVE(id=0x%x)\n", rdp_shareid);
	rdp_send_confirm_active();
	rdp_send_synchronise();
	rdp_send_control(RDP_CTL_COOPERATE);
	rdp_send_control(RDP_CTL_REQUEST_CONTROL);
	rdp_recv(&type);	/* RDP_PDU_SYNCHRONIZE */
	rdp_recv(&type);	/* RDP_CTL_COOPERATE */
	rdp_recv(&type);	/* RDP_CTL_GRANT_CONTROL */
	rdp_send_input(0, RDP_INPUT_SYNCHRONIZE, 0, ui_get_toggle_keys(), 0);
	rdp_send_fonts(1);
	rdp_send_fonts(2);
	rdp_recv(&type);	/* RDP_PDU_UNKNOWN 0x28 */

	if (first_demand == 1) {
		DEBUG("RESET ORDER\n");
		reset_order_state();
		first_demand = 0;
	}
}

/* Process a pointer PDU */
static void
process_pointer_pdu(STREAM s)
{
	uint16 message_type;
	uint16 x, y, width, height, cache_idx, masklen, datalen;
	uint8 *mask, *data;
	HCURSOR cursor;

	in_uint16_le(s, message_type);
	in_uint8s(s, 2);	/* pad */

	switch (message_type) {
	case RDP_POINTER_MOVE:
		in_uint16_le(s, x);
		in_uint16_le(s, y);
		if (s_check(s))
			ui_move_pointer(x, y);
		break;
	case RDP_POINTER_COLOR:
		in_uint16_le(s, cache_idx);
		in_uint16_le(s, x);
		in_uint16_le(s, y);
		in_uint16_le(s, width);
		in_uint16_le(s, height);
		in_uint16_le(s, masklen);
		in_uint16_le(s, datalen);
		in_uint8p(s, data, datalen);
		in_uint8p(s, mask, masklen);
		cursor = ui_create_cursor(x, y, width, height, mask, data);
		ui_set_cursor(cursor);
		cache_put_cursor(cache_idx, cursor);
		break;
	case RDP_POINTER_CACHED:
		in_uint16_le(s, cache_idx);
		ui_set_cursor(cache_get_cursor(cache_idx));
		break;
	default:
		DEBUG("Pointer message 0x%x\n", message_type);
	}
}

/* Process bitmap updates */
static void
process_bitmap_updates(STREAM s)
{
	uint16 num_updates;
	uint16 left, top, right, bottom, width, height;
	uint16 cx, cy, bpp, compress, bufsize, size;
	uint8 *data, *bmpdata;
	int i;
	in_uint16_le(s, num_updates);
	for (i = 0; i < num_updates; i++) {
		in_uint16_le(s, left);
		in_uint16_le(s, top);
		in_uint16_le(s, right);
		in_uint16_le(s, bottom);
		in_uint16_le(s, width);
		in_uint16_le(s, height);
		in_uint16_le(s, bpp);
		in_uint16_le(s, compress);
		in_uint16_le(s, bufsize);
		cx = right - left + 1;
		cy = bottom - top + 1;
		DEBUG("UPDATE(l=%d,t=%d,r=%d,b=%d,w=%d,h=%d,cmp=%d)\n", left,
		      top, right, bottom, width, height, compress);
		if (!compress) {
			int y;
			bmpdata = xmalloc(width * height);
			for (y = 0; y < height; y++) {
				in_uint8a(s, &bmpdata[(height - y - 1) * width],
					  width);
			}
			ui_paint_bitmap(left, top, cx, cy, width, height,
					bmpdata);
			xfree(bmpdata);
			continue;
		}
		in_uint8s(s, 2);	/* pad */
		in_uint16_le(s, size);
		in_uint8s(s, 4);	/* line_size, final_size */
		in_uint8p(s, data, size);
		bmpdata = xmalloc(width * height);
		if (bitmap_decompress(bmpdata, width, height, data, size, rdp_bitmap_bits)) {
			ui_paint_bitmap(left, top, cx, cy, width, height,
					bmpdata);
		}
		xfree(bmpdata);
	}
}

/* Process a palette update */
static void
process_palette(STREAM s)
{
	HCOLOURMAP hmap;
	COLOURMAP map;
	in_uint8s(s, 2);	/* pad */
	in_uint16_le(s, map.ncolours);
	in_uint8s(s, 2);	/* pad */
	in_uint8p(s, (uint8 *) map.colours, (map.ncolours * 3));
	hmap = ui_create_colourmap(&map);
	ui_set_colourmap(hmap);
//	ui_destroy_colourmap(hmap);
}

/* Process an update PDU */
static void
process_update_pdu(STREAM s)
{
	uint16 update_type;
	in_uint16_le(s, update_type);
	switch (update_type) {
	case RDP_UPDATE_ORDERS:
		process_orders(s);
		break;
	case RDP_UPDATE_BITMAP:
		process_bitmap_updates(s);
		break;
	case RDP_UPDATE_PALETTE:
		process_palette(s);
		break;
	case RDP_UPDATE_SYNCHRONIZE:
		break;
	default:
		unimpl("update %d\n", update_type);
	}
}

/* Process data PDU */
static void
process_data_pdu(STREAM s)
{
	uint8 data_pdu_type;
	uint8 ctype;
	uint16 clen;
	int roff, rlen, len, ret;
	static struct stream ns;
	static signed char *dict = 0;

	in_uint8s(s, 6); /* shareid, pad, streamid */
	in_uint16(s, len);
	in_uint8(s, data_pdu_type);
	in_uint8(s, ctype);
	in_uint16(s, clen);
	clen -= 18;

	if(ctype & 0x20) {
		if(!dict) {
			dict = (signed char*)malloc(8200*sizeof(signed char));
			dict = (signed char*)memset(dict, 0, 8200*sizeof(signed char));
		}

		ret = decompress(s->p, clen, ctype, (signed char*)dict, &roff, &rlen);

		len -= 18;

		ns.data = xrealloc(ns.data, len);

		ns.data = (unsigned char*)memcpy(ns.data, (unsigned char*)(dict + roff), len);

		ns.size = len;
		ns.end = ns.data + ns.size;
		ns.p = ns.data;
		ns.rdp_hdr = ns.p;

		s = &ns;
	}

	switch (data_pdu_type) {
	case RDP_DATA_PDU_UPDATE:
		process_update_pdu(s);
		break;
	case RDP_DATA_PDU_POINTER:
		process_pointer_pdu(s);
		break;
	case RDP_DATA_PDU_BELL:
		ui_bell();
		break;
	case RDP_DATA_PDU_LOGON:
		/* User logged on */
		break;
	default:
		unimpl("data PDU %d\n", data_pdu_type);
	}
}

#ifndef NEW_STYLE_MAIN_LOOP

#define MAX(a,b) (((a)>(b))?(a):(b))

int
do_select(int s, int r, int timout /* msec */ , fd_set * set)
{
	struct timeval tv;
	int ret = 0;

	FD_ZERO(set);
	FD_SET(s, set);
	FD_SET(r, set);

	if (timout >= 0) {
		tv.tv_sec = 0;
		tv.tv_usec = timout * 1000;
	}

	while (
	       (ret =
		select(MAX(s, r) + 1, set, NULL, NULL,
		       (timout < 0) ? NULL : &tv)) < 0) {
		if (errno != EAGAIN && errno != EINTR) {
			STATUS("tcp_recv: select: %s\n", strerror(errno));
			exit(1);
		}
	}

	return (ret);

}

/* Process incoming packets */
void
rdp_main_loop()
{
	int cnt = 0;
	uint8 type;
	STREAM s;

	fd_set rfds;
	int rdpsocket = rdp_socket();
	int uisocket = ui_select_fd();
	int ret = 0;

	if (rdpsocket < 0) {
		STATUS("rdp socket");
		return;
	}

	if (uisocket < 0) {
		STATUS("ui socket");
		return;
	}

	for (;;) {

		ret =
		    do_select(rdpsocket, uisocket, (cnt == 0) ? -1 : 0, &rfds);

		if (ret == 0) {
			if (ui_pending())
				ui_process_events();
			else
				ui_sync();
			cnt = 0;
		} else {
			if (FD_ISSET(uisocket, &rfds) || ui_pending()) {
				ui_process_events();
				cnt = 0;
			}

			if (FD_ISSET(rdpsocket, &rfds)) {

				do {
					if ((s = rdp_recv(&type)) == NULL)
						return;

					DEBUG("main loop type=%d\n", type);
					switch (type) {
					case RDP_PDU_DEMAND_ACTIVE:
						process_demand_active(s);
						break;
					case RDP_PDU_DEACTIVATE:
						break;
					case RDP_PDU_DATA:
						process_data_pdu(s);
						break;
					default:
						unimpl("PDU %d\n", type);
					}
				} while (next_packet < rdp_s->end);

				cnt++;
			}
		}
	}

}

#else

/* Process incoming packets */
void
rdp_main_loop()
{
  uint8 type;
  STREAM s;


  for (;;)
  {
    if( ! ui_select( rdp_socket() ) )
    {
      break;
    }

    s = rdp_recv(&type);

    if (s == NULL )
    {
      break;
    }

    switch (type)
    {
      case RDP_PDU_DEMAND_ACTIVE:
	process_demand_active(s);
	break;

      case RDP_PDU_DEACTIVATE:
	break;

      case RDP_PDU_DATA:
	process_data_pdu(s);
	break;

      default:
	unimpl("PDU %d\n", type);
    }
  }
}
#endif

/* Establish a connection up to the RDP layer */
BOOL
rdp_connect(char *server, uint32 flags, char *domain, char *password,
	    char *command, char *directory)
{
	if (!sec_connect(server))
		return False;
	rdp_send_logon_info(flags, domain, username, password, command,
			    directory);
	return True;
}

/* Disconnect from the RDP layer */
void
rdp_disconnect()
{
	sec_disconnect();
}

#ifdef SERVER

/* Process a control PDU */
static void
process_control_pdu(STREAM s)
{
	uint16 message_type;
	uint16 x, y, width, height, cache_idx, masklen, datalen;
	uint8 *mask, *data;
	HCURSOR cursor;

	in_uint16_le(s, message_type);
	in_uint8s(s, 2);	/* pad */

	switch (message_type) {
	case RDP_CTL_REQUEST_CONTROL:
//      break;
	case RDP_CTL_GRANT_CONTROL:
//      break;
	case RDP_CTL_DETACH:
//      break;
	case RDP_CTL_COOPERATE:
//      break;
	default:
		DEBUG("Control message 0x%x\n", message_type);
	}
}

/* Process input pdu */
static void
process_input_pdu(STREAM s)
{
	uint16 num_updates;
	uint16 message_type;
	uint16 flags, param1, param2;
	uint32 timestamp;
	HCURSOR cursor;
	int i;
	static int buttonmask;
	int keydown;

	in_uint16_le(s, num_updates);
	in_uint8s(s, 2);	/* pad */
	for (i = 0; i < num_updates; i++) {
		in_uint32_le(s, timestamp);
		in_uint16_le(s, message_type);
		in_uint16_le(s, flags);
		in_uint16_le(s, param1);
		in_uint16_le(s, param2);

		switch (message_type) {
		case RDP_INPUT_SYNCHRONIZE:
			DEBUG("Sync %04x %04x %04x\n", flags, param1, param2);
			break;
		case RDP_INPUT_CODEPOINT:
			DEBUG("Code %04x %04x %04x\n", flags, param1, param2);
			break;
		case RDP_INPUT_VIRTKEY:
			DEBUG("Virt %04x ", flags);
			if (flags & KBD_FLAG_RIGHT) {
				DEBUG("right ");
			}
			if (flags & KBD_FLAG_EXT) {
				DEBUG("ext ");
			}
			if (flags & KBD_FLAG_QUIET) {
				DEBUG("quiet ");
			}
			if (flags & KBD_FLAG_DOWN) {
				DEBUG("down ");
			}
			if (flags & KBD_FLAG_UP) {
				DEBUG("up ");
			}
			DEBUG("%04x %04x\n", param1, param2);
			break;
		case RDP_INPUT_SCANCODE:
			DEBUG("Scan %04x ", flags);
			if (flags & KBD_FLAG_RIGHT) {
				DEBUG("right ");
			}
			if (flags & KBD_FLAG_EXT) {
				DEBUG("ext ");
			}
			if (flags & KBD_FLAG_QUIET) {
				DEBUG("quiet ");
			}
			if ((flags & (KBD_FLAG_DOWN | KBD_FLAG_UP)) == 0) {
				keydown = 1;
				DEBUG("down ");
			} else if ((flags & (KBD_FLAG_DOWN | KBD_FLAG_UP))
				   == (KBD_FLAG_DOWN | KBD_FLAG_UP)) {
				keydown = 0;
				DEBUG("up ");
			} else if (flags & KBD_FLAG_DOWN) {
				keydown = 0;
				DEBUG("down? ");
			} else if (flags & KBD_FLAG_UP) {
				keydown = 0;
				DEBUG("up? ");
			}
			DEBUG("%04x %04x\n", param1, param2);
			if (keydown) {
				DEBUG("KEY: %4x\n",
				      x_key_to_sym(param1, keydown));
			} else {
				DEBUG("key: %4x\n",
				      x_key_to_sym(param1, keydown));
			}
			rfb_send_keyevent(x_key_to_sym(param1, keydown),
					  keydown);
			break;
		case RDP_INPUT_MOUSE:
			DEBUG("Mouse %04x ", flags);
			if (flags & MOUSE_FLAG_MOVE) {
				DEBUG("move ");
			}
			if (flags & MOUSE_FLAG_BUTTON1) {
				DEBUG("button1 ");
				if (flags & MOUSE_FLAG_DOWN)
					buttonmask |= 1;
				else
					buttonmask &= ~1;
			}
			if (flags & MOUSE_FLAG_BUTTON2) {
				DEBUG("button2 ");
				if (flags & MOUSE_FLAG_DOWN)
					buttonmask |= 4;
				else
					buttonmask &= ~4;
			}
			if (flags & MOUSE_FLAG_BUTTON3) {
				DEBUG("button3 ");
				if (flags & MOUSE_FLAG_DOWN)
					buttonmask |= 2;
				else
					buttonmask &= ~2;
			}
			if (flags & MOUSE_FLAG_DOWN) {
				DEBUG("down ");
			}
				else if (flags &
					 (MOUSE_FLAG_BUTTON1 |
					  MOUSE_FLAG_BUTTON2 |
					  MOUSE_FLAG_BUTTON3)) {
				DEBUG("up ");
			}
			DEBUG("x=%d y=%d\n", param1, param2);
			rfb_send_pointerevent(param1, param2, buttonmask);
			s = rdp_init_data(6);
			out_uint16_le(s, RDP_POINTER_MOVE);
			out_uint16_le(s, param1);
			out_uint16_le(s, param2);
			s_mark_end(s);
			rdp_send_data(s, RDP_DATA_PDU_POINTER);
			break;
		default:
			DEBUG("Input message 0x%x\n", message_type);
		}
	}
}

/* Process synchronise pdu */
static void
process_synchronise_pdu(STREAM s)
{
	uint16 message_type;
	uint16 x, y, width, height, cache_idx, masklen, datalen;
	uint8 *mask, *data;
	HCURSOR cursor;

	in_uint16_le(s, message_type);
	in_uint8s(s, 2);	/* pad */

	switch (message_type) {
	case RDP_INPUT_SYNCHRONIZE:
//      break;
	case RDP_INPUT_CODEPOINT:
//      break;
	case RDP_INPUT_VIRTKEY:
//      break;
	case RDP_INPUT_SCANCODE:
//      break;
	case RDP_INPUT_MOUSE:
//      break;
	default:
		DEBUG("Synchronise message 0x%x\n", message_type);
	}
}

/* Process font2 pdu */
static void
process_font2_pdu(STREAM s)
{
	uint16 message_type;
	uint16 x, y, width, height, cache_idx, masklen, datalen;
	uint8 *mask, *data;
	HCURSOR cursor;

	in_uint16_le(s, message_type);
	in_uint8s(s, 2);	/* pad */

	switch (message_type) {
	case RDP_INPUT_SYNCHRONIZE:
//      break;
	case RDP_INPUT_CODEPOINT:
//      break;
	case RDP_INPUT_VIRTKEY:
//      break;
	case RDP_INPUT_SCANCODE:
//      break;
	case RDP_INPUT_MOUSE:
//      break;
	default:
		DEBUG("Font2 message 0x%x\n", message_type);
	}
}

/* Process data PDU */
static void
process_sdata_pdu(STREAM s)
{
	uint8 data_pdu_type;
	in_uint8s(s, 8);	/* shareid, pad, streamid, length */
	in_uint8(s, data_pdu_type);
	in_uint8s(s, 3);	/* compress_type, compress_len */
	switch (data_pdu_type) {
	case RDP_DATA_PDU_CONTROL:
		process_control_pdu(s);
		break;
	case RDP_DATA_PDU_INPUT:
		process_input_pdu(s);
		break;
	case RDP_DATA_PDU_SYNCHRONISE:
		process_synchronise_pdu(s);
		break;
	case RDP_DATA_PDU_FONT2:
		process_font2_pdu(s);
		break;
	default:
		unimpl("data PDU %d\n", data_pdu_type);
	}
}

unsigned char curs_mask[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

unsigned char curs_data[768];

/* Send a pointer color PDU */
static void
send_pointer_color()
{
	STREAM s;
	s = rdp_init_data(818);
	out_uint16_le(s, RDP_POINTER_COLOR);
	out_uint16_le(s, 0);
	out_uint16_le(s, 12);
	out_uint16_le(s, 0);
	out_uint16_le(s, 0);
	out_uint16_le(s, 16);
	out_uint16_le(s, 16);
	out_uint16_le(s, 32);
	out_uint16_le(s, 768);
	out_uint8p(s, curs_data, 768);
	out_uint8p(s, curs_mask, 32);
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_POINTER);
}

#include "orders.h"
extern int width, height;

/* Send a pointer color PDU */
static void
send_background()
{
	STREAM s;
	s = rdp_init_data(20);
	out_uint16_le(s, RDP_UPDATE_ORDERS);
	out_uint16_le(s, 0);
	out_uint16_le(s, 1);
	out_uint16_le(s, 0);
	out_uint8(s, RDP_ORDER_STANDARD | RDP_ORDER_CHANGE);
	out_uint8(s, RDP_ORDER_DESTBLT);
	out_uint8(s, 0x1f);
	out_uint16_le(s, 0);
	out_uint16_le(s, 0);
	out_uint16_le(s, width);
	out_uint16_le(s, height);
	out_uint8(s, 0x00);	/* opcode = 0x0f */
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_UPDATE);
}

extern char vnccolormap[];

/* Send a palette */
void
rdp_send_palette()
{
	STREAM s;
	int length;

	length = width * height;
	s = rdp_init_data(776);
	out_uint16_le(s, RDP_UPDATE_PALETTE);
	out_uint16_le(s, 0);	/* padding */
	out_uint16_le(s, 256);	/* nb of colours */
	out_uint16_le(s, 0);	/* padding */
	out_uint8a(s, vnccolormap, 768);	/* colormap */
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_UPDATE);
}

/* Send a raw rectangle */
void
rdp_send_rawrect(int x, int y, int width, int height, char *rect, int length)
{
	STREAM s;

	s = rdp_init_data(22 + length);
	out_uint16_le(s, RDP_UPDATE_BITMAP);
	out_uint16_le(s, 1);	/* num updates */
	out_uint16_le(s, x);	/* left */
	out_uint16_le(s, y);	/* top */
	out_uint16_le(s, x + width - 1);	/* right */
	out_uint16_le(s, y + height - 1);	/* bottom */
	out_uint16_le(s, (width + 3) & ~3);	/* width */
	out_uint16_le(s, height);	/* height */
	out_uint16_le(s, 8);	/* bpp */
	out_uint16_le(s, 0);	/* compress */
	out_uint16_le(s, length);	/* buf-size */
	out_uint8a(s, rect, length);	/* raw data */
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_UPDATE);
}

/* Send a copy rectangle */
void
rdp_send_cpyrect(int x_src, int y_src, int x_dest, int y_dest, int width,
		 int height)
{
	STREAM s;

	s = rdp_init_data(24);
	out_uint16_le(s, RDP_UPDATE_ORDERS);
	out_uint16_le(s, 0);
	out_uint16_le(s, 1);
	out_uint16_le(s, 0);
	out_uint8(s, RDP_ORDER_STANDARD | RDP_ORDER_CHANGE);
	out_uint8(s, RDP_ORDER_SCREENBLT);
	out_uint8(s, 0x7f);
	out_uint16_le(s, x_dest);
	out_uint16_le(s, y_dest);
	out_uint16_le(s, width);
	out_uint16_le(s, height);
	out_uint8(s, 0x0c);	/* opcode = 0x0f */
	out_uint16_le(s, x_src);
	out_uint16_le(s, y_src);
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_UPDATE);
}

/* Send a rectangle */
void
rdp_send_rect(int x_dest, int y_dest, int width, int height, int colour)
{
	STREAM s;

	s = rdp_init_data(20);
	out_uint16_le(s, RDP_UPDATE_ORDERS);
	out_uint16_le(s, 0);
	out_uint16_le(s, 1);
	out_uint16_le(s, 0);
	out_uint8(s, RDP_ORDER_STANDARD | RDP_ORDER_CHANGE);
	out_uint8(s, RDP_ORDER_RECT);
	out_uint8(s, 0x1f);
	out_uint16_le(s, x_dest);
	out_uint16_le(s, y_dest);
	out_uint16_le(s, width);
	out_uint16_le(s, height);
	out_uint8(s, colour);
	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_UPDATE);
}

/* Process incoming packets */
void
rdp_smain_loop()
{
	uint8 type;
	STREAM s;

	fd_set rfds;
	int rdpsocket = rdp_socket();
//  int uisocket = ui_select_fd();
	int vncsocket = vnc_socket();
//  int uisocket = 0;
	int ret = 0;

	if (rdpsocket < 0) {
		STATUS("rdp socket");
		return;
	}

	if (vncsocket < 0) {
		STATUS("vnc socket");
		return;
	}

	ret = do_select(rdpsocket, vncsocket, -1, &rfds);

	for (;;) {

		if (FD_ISSET(rdpsocket, &rfds)) {
			do {
				if ((s = rdp_recv(&type)) == NULL)
					return;

				DEBUG("smain loop type=%d\n", type);
				switch (type) {
				case RDP_PDU_DEMAND_ACTIVE:
					process_demand_active(s);
					break;
				case RDP_PDU_CONFIRM_ACTIVE:
					break;
				case RDP_PDU_DEACTIVATE:
					break;
				case RDP_PDU_DATA:
					process_sdata_pdu(s);
					break;
				default:
					unimpl("sPDU %d\n", type);
				}
			} while (next_packet < rdp_s->end);

		}
		if (FD_ISSET(vncsocket, &rfds))

			rfb_recv();

		if ((ret = do_select(rdpsocket, vncsocket, 0, &rfds)) > 0)
			continue;

		rfb_send_updaterequest(0, 0, width, height, 1);
//               ui_sync();

		ret = do_select(rdpsocket, vncsocket, -1, &rfds);
	}

}

/* Establish a connection up to the RDP layer */
BOOL
rdp_listen(char *server, uint32 flags, char *domain, char *password,
	   char *command, char *directory)
{
	if (!sec_listen(server))
		return False;

//rdp_recv_logon_info(flags, domain, username, password, command, directory);
	send_pointer_color();
//    send_background();
	return True;
}

#endif				/* SERVER */
