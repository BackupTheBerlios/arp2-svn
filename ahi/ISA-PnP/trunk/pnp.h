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

struct ISAPNPBase;
struct ISAPNP_Card;
struct ISAPNP_Device;

BOOL ASMCALL
ISAPNP_ScanCards( REG( a6, struct ISAPNPBase* res ) );

BOOL ASMCALL
ISAPNP_ConfigureCards( REG( a6, struct ISAPNPBase* res ) );


struct ISAPNP_Card* ASMCALL
ISAPNP_FindCard( REG( a0, struct ISAPNP_Card* last_card ), 
                 REG( d0, LONG                manufacturer ),
                 REG( d1, WORD                product ),
                 REG( d2, BYTE                revision ),
                 REG( d3, LONG                serial ),
                 REG( a6, struct ISAPNPBase*  res ) );


struct ISAPNP_Device* ASMCALL
ISAPNP_FindDevice( REG( a0, struct ISAPNP_Device* last_device ), 
                   REG( d0, LONG                  manufacturer ),
                   REG( d1, WORD                  product ),
                   REG( d2, BYTE                  revision ),
                   REG( a6, struct ISAPNPBase*    res ) );

#endif /* ISA_PNP_controller_h */
