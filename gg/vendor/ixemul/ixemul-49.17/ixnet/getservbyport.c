/*
 *  This file is part of ixnet.library for the Amiga.
 *  Copyright (C) 1996 Jeff Shepherd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  $Id: getservbyport.c,v 1.1.1.1 2000/05/07 19:37:44 emm Exp $
 *
 *  $Log: getservbyport.c,v $
 *  Revision 1.1.1.1  2000/05/07 19:37:44  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:45:01  nobody
 *  Initial import
 *
 */
#define _KERNEL
#include "ixnet.h"
#include <netdb.h>
#include <string.h>

struct servent *
getservbyport(int port, const char *proto)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    switch (network_protocol) {

	case IX_NETWORK_AMITCP:
	    return TCP_GetServByPort(port,proto);

	default: /*case IX_NETWORK_AS225:*/
	    return SOCK_getservbyport(port,proto);
    }
}
