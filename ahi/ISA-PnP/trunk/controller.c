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

#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>

#include <proto/expansion.h>

#include "controller.h"
#include "isapnp_private.h"

extern struct Custom custom;


/******************************************************************************
*** Some local functions used to make memory accesses to hardware register ****
******************************************************************************/

static inline UBYTE
ReadByte( const volatile UBYTE* address )
{
  return *address;
}


static inline UWORD
ReadWord( const volatile UWORD* address )
{
  return *address;
}


static inline ULONG
ReadLong( const volatile ULONG* address )
{
  return *address;
}


static inline void
WriteByte( volatile UBYTE* address,
           UBYTE           value )
{
  *address = value;
}


static inline void
WriteWord( volatile UWORD* address,
           UWORD           value )
{
  *address = value;
}


static inline void
WriteLong( volatile ULONG* address,
           ULONG           value )
{
  *address = value;
}


/******************************************************************************
*** Controller functions ******************************************************
******************************************************************************/

void ASMCALL
ISAC_SetMasterInt( REG( d0, BOOL                   on ),
                   REG( a6, struct ISAPnPResource* res ) )
{
  UWORD* reg2 = (UWORD*)( res->m_Base + 0x18002 );

  if( on )
  {
    WriteWord( reg2, 0 );
  }
  else
  {
    ReadWord( reg2 );
  }
}


BOOL ASMCALL
ISAC_GetMasterInt( REG( a6, struct ISAPnPResource* res ) )
{
  UWORD* reg1 = (UWORD*)( res->m_Base + 0x18000 );

  return ( ReadWord( reg1 ) & 1 ) != 0;
}


void ASMCALL
ISAC_SetWaitState( REG( d0, BOOL                   on ),
                   REG( a6, struct ISAPnPResource* res ) )
{
#if 0
  UBYTE* reg3 = (UBYTE*) ( isa_Base + 0x18007 );

  if( (  on && !WaitState() ) ||
      ( !on &&  WaitState() ) )
  {
    *reg3 = 0;             // Toggle
  }
#endif
}


BOOL ASMCALL
ISAC_GetWaitState( REG( a6, struct ISAPnPResource* res ) )
{
  UWORD* reg1 = (UWORD*)( res->m_Base + 0x18000 );

  return ( ReadWord( reg1 ) & 2 ) != 0;
}


BOOL ASMCALL
ISAC_GetInterruptStatus( REG( d0, UBYTE                  interrupt ),
                         REG( a6, struct ISAPnPResource* res ) )
{
  UWORD* reg1 = (UWORD*)( res->m_Base + 0x18000 );

  switch( interrupt )
  {
    case 3:
      return ( ReadWord( reg1 ) & 4 ) != 0;

    case 4:
      return ( ReadWord( reg1 ) & 8 ) != 0;

    case 5:
      return ( ReadWord( reg1 ) & 16 ) != 0;

    case 6:
      return ( ReadWord( reg1 ) & 32 ) != 0;

    case 7:
      return ( ReadWord( reg1 ) & 64 ) != 0;

    case 9:
      return ( ReadWord( reg1 ) & 128 ) != 0;

    case 10:
      return ( ReadWord( reg1 ) & 256 ) != 0;

    case 11:
      return ( ReadWord( reg1 ) & 512 ) != 0;

    case 12:
      return ( ReadWord( reg1 ) & 1024 ) != 0;

    case 14:
      return ( ReadWord( reg1 ) & 2048 ) != 0;

    case 15:
      return ( ReadWord( reg1 ) & 4096 ) != 0;
  }
  
  return FALSE;
}


UBYTE ASMCALL
ISAC_GetRegByte( REG( d0, UWORD                  reg ),
                 REG( a6, struct ISAPnPResource* res ) )
{
  return ReadByte( res->m_Base + 2 * reg + 1 );
}


void ASMCALL
ISAC_SetRegByte( REG( d0, UWORD                  reg ),
                 REG( d1, UBYTE                  value ),
                 REG( a6, struct ISAPnPResource* res ) )
{
  WriteByte( res->m_Base + 2 * reg + 1, value );
}


UWORD ASMCALL
ISAC_GetRegWord( REG( d0, UWORD                  reg ),
                 REG( a6, struct ISAPnPResource* res ) )
{
  ULONG value;

  value = ReadLong( (ULONG*)( res->m_Base + 2 * reg ) );

  return ( value << 8 ) | ( ( value & 0x00ff0000 ) >> 16 );
}


void ASMCALL
ISAC_SetRegWord( REG( d0, UWORD                  reg ),
                 REG( d1, UWORD                  value ),
                 REG( a6, struct ISAPnPResource* res ) )
{
  WriteLong( (ULONG*)( res->m_Base + 2 * reg ),
             ( (ULONG)( value & 0xff ) << 16 ) | ( value >> 8 ) );
}


UBYTE ASMCALL
ISAC_ReadByte( REG( d0, ULONG                  address ),
               REG( a6, struct ISAPnPResource* res ) )
{
  return ReadByte( res->m_Base + ( ( 2 * address ) & 0xfffff ) + 1 );
}


void ASMCALL
ISAC_WriteByte( REG( d0, ULONG                  address ),
                REG( d1, UBYTE                  value ),
                REG( a6, struct ISAPnPResource* res ) )
{
  WriteByte( res->m_Base + ( ( 2 * address ) & 0xfffff ) + 1, value );
}


UWORD ASMCALL
ISAC_ReadWord( REG( d0, ULONG                  address ),
               REG( a6, struct ISAPnPResource* res ) )
{
  return ReadWord( (UWORD*)( res->m_Base + ( ( 2 * address ) & 0xfffff ) ) );
}


void ASMCALL
ISAC_WriteWord( REG( d0, ULONG                  address ),
                REG( d1, UWORD                  value ),
                REG( a6, struct ISAPnPResource* res ) )
{
  WriteWord( (UWORD*)( res->m_Base + ( ( 2 * address ) & 0xfffff ) ),
             value );
}
