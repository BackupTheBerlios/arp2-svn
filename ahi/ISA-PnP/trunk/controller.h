/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef	ISA_PNP_controller_h
#define ISA_PNP_controller_h

#include <exec/types.h>

struct ISAPnPResource;

void
ISAC_SetMasterInt( BOOL                   on,
                   struct ISAPnPResource* res );

BOOL
ISAC_GetMasterInt( struct ISAPnPResource* res );


void
ISAC_SetWaitState( BOOL                   on,
                   struct ISAPnPResource* res );


BOOL
ISAC_GetWaitState( struct ISAPnPResource* res );


BOOL
ISAC_GetInterruptStatus( UBYTE                  interrupt,
                         struct ISAPnPResource* res );

UBYTE
ISAC_GetRegByte( UWORD                  reg,
                 struct ISAPnPResource* res );


void
ISAC_SetRegByte( UWORD                  reg, 
                 UBYTE                  value,
                 struct ISAPnPResource* res );


UWORD
ISAC_GetRegWord( UWORD                  reg,
                 struct ISAPnPResource* res );


void
ISAC_SetRegWord( UWORD                  reg, 
                 UWORD                  value,
                 struct ISAPnPResource* res );


UBYTE
ISAC_ReadByte( ULONG                  address,
               struct ISAPnPResource* res );


void
ISAC_WriteByte( ULONG                  address, 
                UBYTE                  value,
                struct ISAPnPResource* res );


UWORD
ISAC_ReadWord( ULONG                  address,
               struct ISAPnPResource* res );


void
ISAC_WriteWord( ULONG                  address, 
                UWORD                  value,
                struct ISAPnPResource* res );

#endif /* ISA_PNP_controller_h */
