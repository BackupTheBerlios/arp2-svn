/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef	ISA_PNP_pnp_h
#define ISA_PNP_pnp_h

#include "CompilerSpecific.h"

#include <exec/types.h>

struct ISAPNP_Resource;
struct ISAPNP_Card;
struct ISAPNP_Device;

struct ISAPNP_Card* ASMCALL
PNPISA_AllocCard( REG( a6, struct ISAPNP_Resource* res ) );

void ASMCALL
PNPISA_FreeCard( REG( a0, struct ISAPNP_Card*     card ),
                 REG( a6, struct ISAPNP_Resource* res ) );

struct ISAPNP_Device* ASMCALL
PNPISA_AllocDevice( REG( a6, struct ISAPNP_Resource* res ) );

void ASMCALL
PNPISA_FreeDevice( REG( a0, struct ISAPNP_Device*   dev ),
                   REG( a6, struct ISAPNP_Resource* res ) );

BOOL ASMCALL
PNPISA_ConfigureCards( REG( a6, struct ISAPNP_Resource* res ) );


#endif /* ISA_PNP_controller_h */
