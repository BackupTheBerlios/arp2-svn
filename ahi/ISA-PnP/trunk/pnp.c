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

#include "CompilerSpecific.h"

#include "isapnp.h"
#include "isapnp_private.h"

#include "init.h"
#include "controller.h"


/******************************************************************************
** PnP ISA helper functions ***************************************************
******************************************************************************/

static UBYTE
GetPnPReg( UBYTE                  pnp_reg,
            struct ISAPnPResource* res )
{
  ISAC_SetRegByte( ISAPNP_ADDRESS, pnp_reg, res );

  return ISAC_GetRegByte( res->m_RegReadData, res );
}


static void
SetPnPReg( UBYTE                  pnp_reg,
           UBYTE                  value,
           struct ISAPnPResource* res )
{
  ISAC_SetRegByte( ISAPNP_ADDRESS,    pnp_reg, res );
  ISAC_SetRegByte( ISAPNP_WRITE_DATA, value,   res );
}


static void
SendInitiationKey( struct ISAPnPResource* res )
{
  static const UBYTE key[] =
  {
    ISAPNP_INITIATION_KEY
  };
  
  unsigned int i;

  // Make sure the LFSR is in its initial state

  ISAC_SetRegByte( ISAPNP_ADDRESS, 0, res );
  ISAC_SetRegByte( ISAPNP_ADDRESS, 0, res );

  for( i = 0; i < sizeof( key ); ++i )
  {
    KPrintF( "%2ld: Wrote %ld\n", i, key[ i ] );
    ISAC_SetRegByte( ISAPNP_ADDRESS, key[ i ], res );
  }
}



/******************************************************************************
** Configure all cards ********************************************************
******************************************************************************/

BOOL ASMCALL
PNPISA_ConfigureCards( REG( a6, struct ISAPnPResource* res ) )
{
  res->m_RegReadData = 0x0000;
  
  SendInitiationKey( res );
}
