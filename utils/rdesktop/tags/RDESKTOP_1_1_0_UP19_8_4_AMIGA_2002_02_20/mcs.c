/*
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - Multipoint Communications Service
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

#include "rdesktop.h"

uint16 mcs_userid;

extern BOOL use_encryption;
extern BOOL licence_issued;

/* Parse an ASN.1 BER header */
static BOOL
ber_parse_header(STREAM s, int tagval, int *length)
{
	int tag, len;

	if (tagval > 0xff) {
		in_uint16_be(s, tag);
	} else {
	in_uint8(s, tag)}

	if (tag != tagval) {
		error("expected tag %d, got %d\n", tagval, tag);
		return False;
	}

	in_uint8(s, len);

	if (len & 0x80) {
		len &= ~0x80;
		*length = 0;
		while (len--)
			next_be(s, *length);
	} else
		*length = len;

	return s_check(s);
}

/* Output an ASN.1 BER header */
static void
ber_out_header(STREAM s, int tagval, int length)
{
	if (tagval > 0xff) {
		out_uint16_be(s, tagval);
	} else {
		out_uint8(s, tagval);
	}

	if (length >= 0x80) {
		out_uint8(s, 0x82);
		out_uint16_be(s, length);
	} else
		out_uint8(s, length);
}

/* Output an ASN.1 BER integer */
static void
ber_out_integer(STREAM s, int value)
{
	ber_out_header(s, BER_TAG_INTEGER, 2);
	out_uint16_be(s, value);
}

/* Output a DOMAIN_PARAMS structure (ASN.1 BER) */
static void
mcs_out_domain_params(STREAM s, int max_channels, int max_users,
		      int max_tokens, int max_pdusize)
{
	ber_out_header(s, MCS_TAG_DOMAIN_PARAMS, 32);
	ber_out_integer(s, max_channels);
	ber_out_integer(s, max_users);
	ber_out_integer(s, max_tokens);
	ber_out_integer(s, 1);	/* num_priorities */
	ber_out_integer(s, 0);	/* min_throughput */
	ber_out_integer(s, 1);	/* max_height */
	ber_out_integer(s, max_pdusize);
	ber_out_integer(s, 2);	/* ver_protocol */
}

/* Parse a DOMAIN_PARAMS structure (ASN.1 BER) */
static BOOL
mcs_parse_domain_params(STREAM s)
{
	int length;

	ber_parse_header(s, MCS_TAG_DOMAIN_PARAMS, &length);
	in_uint8s(s, length);

	return s_check(s);
}

/* Send an MCS_CONNECT_INITIAL message (ASN.1 BER) */
static void
mcs_send_connect_initial(STREAM mcs_data)
{
	int datalen = mcs_data->end - mcs_data->data;
	int length = 7 + 3 * 34 + 4 + datalen;
	STREAM s;

	s = iso_init(length + 5);

	ber_out_header(s, MCS_CONNECT_INITIAL, length);
	ber_out_header(s, BER_TAG_OCTET_STRING, 0);	/* calling domain */
	ber_out_header(s, BER_TAG_OCTET_STRING, 0);	/* called domain */

	ber_out_header(s, BER_TAG_BOOLEAN, 1);
	out_uint8(s, 0xff);	/* upward flag */

	mcs_out_domain_params(s, 2, 2, 0, 0xffff);	/* target params */
	mcs_out_domain_params(s, 1, 1, 1, 0x420);	/* min params */
	mcs_out_domain_params(s, 0xffff, 0xfc17, 0xffff, 0xffff);	/* max params */

	ber_out_header(s, BER_TAG_OCTET_STRING, datalen);
	out_uint8p(s, mcs_data->data, datalen);

	s_mark_end(s);
	iso_send(s);
}

/* Expect a MCS_CONNECT_RESPONSE message (ASN.1 BER) */
static BOOL
mcs_recv_connect_response(STREAM mcs_data)
{
	uint8 result;
	int length;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return False;

	ber_parse_header(s, MCS_CONNECT_RESPONSE, &length);

	ber_parse_header(s, BER_TAG_RESULT, &length);
	in_uint8(s, result);
	if (result != 0) {
		error("MCS connect: %d\n", result);
		return False;
	}

	ber_parse_header(s, BER_TAG_INTEGER, &length);
	in_uint8s(s, length);	/* connect id */
	mcs_parse_domain_params(s);

	ber_parse_header(s, BER_TAG_OCTET_STRING, &length);
	if (length > mcs_data->size) {
		WARN("MCS data length %d\n", length);
		length = mcs_data->size;
	}

	in_uint8a(s, mcs_data->data, length);
	mcs_data->p = mcs_data->data;
	mcs_data->end = mcs_data->data + length;

	return s_check_end(s);
}

/* Send an EDrq message (ASN.1 PER) */
static void
mcs_send_edrq()
{
	STREAM s;

	s = iso_init(5);

	out_uint8(s, (MCS_EDRQ << 2));
	out_uint16_be(s, 0x0100);	/* height */
	out_uint16_be(s, 0x0100);	/* interval */

	s_mark_end(s);
	iso_send(s);
}

/* Send an AUrq message (ASN.1 PER) */
static void
mcs_send_aurq()
{
	STREAM s;

	s = iso_init(1);

	out_uint8(s, (MCS_AURQ << 2));

	s_mark_end(s);
	iso_send(s);
}

/* Expect a AUcf message (ASN.1 PER) */
static BOOL
mcs_recv_aucf(uint16 * mcs_userid)
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != MCS_AUCF) {
		error("expected AUcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0) {
		error("AUrq: %d\n", result);
		return False;
	}

	if (opcode & 2)
		in_uint16_be(s, *mcs_userid);

	return s_check_end(s);
}

/* Send a CJrq message (ASN.1 PER) */
static void
mcs_send_cjrq(uint16 chanid)
{
	STREAM s;

	s = iso_init(5);

	out_uint8(s, (MCS_CJRQ << 2));
	out_uint16_be(s, mcs_userid);
	out_uint16_be(s, chanid);

	s_mark_end(s);
	iso_send(s);
}

/* Expect a CJcf message (ASN.1 PER) */
static BOOL
mcs_recv_cjcf()
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != MCS_CJCF) {
		error("expected CJcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0) {
		error("CJrq: %d\n", result);
		return False;
	}

	in_uint8s(s, 4);	/* mcs_userid, req_chanid */
	if (opcode & 2)
		in_uint8s(s, 2);	/* join_chanid */

	return s_check_end(s);
}

/* Initialise an MCS transport data packet */
STREAM
mcs_init(int length)
{
	STREAM s;

	s = iso_init(length + 8);
	s_push_layer(s, mcs_hdr, 8);

	return s;
}

/* Send an MCS transport data packet */
void
mcs_send(STREAM s)
{
	uint16 length;

	s_pop_layer(s, mcs_hdr);
	length = s->end - s->p - 8;
	length |= 0x8000;

#ifdef SERVER
	out_uint8(s, (MCS_SDIN << 2));
#else				/* SERVER */
	out_uint8(s, (MCS_SDRQ << 2));
#endif				/* SERVER */
	out_uint16_be(s, mcs_userid);
	out_uint16_be(s, MCS_GLOBAL_CHANNEL);
	out_uint8(s, 0x70);	/* flags */
	out_uint16_be(s, length);

	iso_send(s);
}

/* Receive an MCS transport data packet */
STREAM
mcs_recv()
{
	uint8 opcode, appid, length;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return NULL;

	in_uint8(s, opcode);
	appid = opcode >> 2;
#ifdef SERVER
	if (appid != MCS_SDRQ)
#else				/* SERVER */
	if (appid != MCS_SDIN)
#endif				/* SERVER */
	{
		if (appid != MCS_DPUM) {
			error("expected data, got %d\n", opcode);
		}
		return NULL;
	}

	in_uint8s(s, 5);	/* userid, chanid, flags */
	in_uint8(s, length);
	if (length & 0x80)
		in_uint8s(s, 1);	/* second byte of length */

	return s;
}

/* Establish a connection up to the MCS layer */
BOOL
mcs_connect(char *server, STREAM mcs_data)
{
	if (!iso_connect(server))
		return False;

	mcs_send_connect_initial(mcs_data);
	if (!mcs_recv_connect_response(mcs_data))
		goto error;

	mcs_send_edrq();

	mcs_send_aurq();
	if (!mcs_recv_aucf(&mcs_userid))
		goto error;

	mcs_send_cjrq(mcs_userid + 1001);
	if (!mcs_recv_cjcf())
		goto error;

	mcs_send_cjrq(MCS_GLOBAL_CHANNEL);
	if (!mcs_recv_cjcf())
		goto error;

	return True;

      error:
	iso_disconnect();
	return False;
}

/* Disconnect from the MCS layer */
void
mcs_disconnect()
{
	iso_disconnect();
}

#ifdef SERVER

/* Send an MCS_CONNECT_RESPONSE message (ASN.1 BER) */
static void
mcs_send_connect_response(STREAM mcs_data)
{
	int datalen = mcs_data->end - mcs_data->data;
	int length = 7 + 3 * 34 + 4 + datalen;
	STREAM s;

	s = iso_init(length + 5);

	ber_out_header(s, MCS_CONNECT_RESPONSE, 0x82);
	ber_out_header(s, BER_TAG_RESULT, 1);
	out_uint8(s, 0);	/* result, 0 == OK */
	ber_out_header(s, BER_TAG_INTEGER, 1);
	out_uint8(s, 0);	/* connect id */
	mcs_out_domain_params(s, 2, 2, 0, 0xffff);	/* target params */
	ber_out_header(s, BER_TAG_OCTET_STRING, datalen);
	out_uint8p(s, mcs_data->data, datalen);

	s_mark_end(s);
	iso_send(s);
}

/* Expect a MCS_CONNECT_INITIAL message (ASN.1 BER) */
static BOOL
mcs_recv_connect_initial(STREAM mcs_data)
{
	uint8 result;
	int length;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return False;

	ber_parse_header(s, MCS_CONNECT_INITIAL, &length);
	ber_parse_header(s, BER_TAG_OCTET_STRING, &length);	/* calling domain */
	if (length > mcs_data->size) {
		WARN("MCS data length %d\n", length);
		length = mcs_data->size;
	}
	in_uint8a(s, mcs_data->data, length);
	mcs_data->p = mcs_data->data;
	mcs_data->end = mcs_data->data + length;
	ber_parse_header(s, BER_TAG_OCTET_STRING, &length);	/* called domain */
	if (length > mcs_data->size) {
		WARN("MCS data length %d\n", length);
		length = mcs_data->size;
	}
	in_uint8a(s, mcs_data->data, length);
	mcs_data->p = mcs_data->data;
	mcs_data->end = mcs_data->data + length;
	ber_parse_header(s, BER_TAG_BOOLEAN, &length);	/* 0xff = upward flag */
	if (length > mcs_data->size) {
		WARN("MCS data length %d\n", length);
		length = mcs_data->size;
	}
	in_uint8a(s, mcs_data->data, length);
	mcs_data->p = mcs_data->data;
	mcs_data->end = mcs_data->data + length;
	mcs_parse_domain_params(s);
	mcs_parse_domain_params(s);
	mcs_parse_domain_params(s);
	ber_parse_header(s, BER_TAG_OCTET_STRING, &length);	/* mcs_data */
	if (length > mcs_data->size) {
		WARN("MCS data length %d\n", length);
		length = mcs_data->size;
	}
	in_uint8a(s, mcs_data->data, length);
	mcs_data->p = mcs_data->data;
	mcs_data->end = mcs_data->data + length;

	return s_check_end(s);
}

/* Expect a EDrq message (ASN.1 PER) */
static BOOL
mcs_recv_edrq(uint16 * mcs_userid)
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != MCS_EDRQ) {
		error("expected EDrq, got %d\n", opcode);
		return False;
	}
	in_uint16_be(s, result);	/* height */
	in_uint16_be(s, result);	/* interval */

//if (result != 0)
//{
//  error("EDrq: %d\n", result);
//  return False;
//}

	if (opcode & 2)
		in_uint16_be(s, *mcs_userid);

	return s_check_end(s);
}

/* Send an AUcf message (ASN.1 PER) */
static void
mcs_send_aucf()
{
	STREAM s;

	s = iso_init(1);

	out_uint8(s, (MCS_AUCF << 2));
	out_uint8(s, 0);

	s_mark_end(s);
	iso_send(s);
}

/* Expect a AUrq message (ASN.1 PER) */
static BOOL
mcs_recv_aurq(uint16 * mcs_userid)
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != MCS_AURQ) {
		error("expected AUrq, got %d\n", opcode);
		return False;
	}

	if (opcode & 2)
		in_uint16_be(s, *mcs_userid);

	return s_check_end(s);
}

/* Send a CJcf message (ASN.1 PER) */
static void
mcs_send_cjcf(uint16 chanid)
{
	STREAM s;

	s = iso_init(5);

	out_uint8(s, (MCS_CJCF << 2));
	out_uint8(s, 0);
	out_uint16_be(s, mcs_userid);
	out_uint16_be(s, chanid);

	s_mark_end(s);
	iso_send(s);
}

/* Expect a CJrq message (ASN.1 PER) */
static BOOL
mcs_recv_cjrq()
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv();
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != MCS_CJRQ) {
		error("expected CJRQ, got %d\n", opcode);
		return False;
	}

	in_uint8s(s, 4);	/* mcs_userid, req_chanid */
	if (opcode & 2)
		in_uint8s(s, 2);	/* join_chanid */

	return s_check_end(s);
}

/* Establish a connection up to the MCS layer */
BOOL
mcs_listen(char *server, STREAM mcs_data, STREAM mcs_sdata)
{
	if (!iso_listen(server))
		return False;

	if (!mcs_recv_connect_initial(mcs_sdata))
		goto error;
	mcs_send_connect_response(mcs_data);

	if (!mcs_recv_edrq(&mcs_userid))
		goto error;

	if (!mcs_recv_aurq(&mcs_userid))
		goto error;
	mcs_send_aucf();

	if (!mcs_recv_cjrq())
		goto error;
	mcs_send_cjcf(mcs_userid + 1001);

	if (!mcs_recv_cjrq())
		goto error;
	mcs_send_cjcf(MCS_GLOBAL_CHANNEL);

	return True;

      error:
	iso_disconnect();
	return False;
}

#endif				/* SERVER */
