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

#include <proto/timer.h>

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

  return ISAC_GetRegByte( ( res->m_RegReadData << 2 ) | 3, res );
}


static UBYTE
GetLastPnPReg( struct ISAPnPResource* res )
{
  return ISAC_GetRegByte( ( res->m_RegReadData << 2 ) | 3, res );
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
    ISAC_SetRegByte( ISAPNP_ADDRESS, key[ i ], res );
  }
}

void
BusyWait( ULONG micro_seconds )
{
  typedef unsigned long long uint64_t;

  ULONG            freq;
  struct EClockVal eclock;
  uint64_t         current;
  uint64_t         end;

  freq = ReadEClock( &eclock );

  current = ( ( (uint64_t) eclock.ev_hi ) << 32 ) + eclock.ev_lo;
  end     = current + (uint64_t) freq * micro_seconds / 1000000;

  while( current < end )
  {
    ReadEClock( &eclock );

    current = ( ( (uint64_t) eclock.ev_hi ) << 32 ) + eclock.ev_lo;
  }
}

static BOOL
ReadSerialIdentifier( struct ISAPNP_SerialIdentifier* id,
                      struct ISAPnPResource*          res )
{
  UBYTE buf[ 9 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int   i;
  UBYTE check_sum;
  
  ISAC_SetRegByte( ISAPNP_ADDRESS, ISAPNP_REG_SERIAL_ISOLATION, res );

  // Wait 1 ms before we read the first pair

  BusyWait( 1000 );

  check_sum = 0x6a;

  for( i = 0; i < 72; ++i )
  {
    UBYTE value1;
    UBYTE value2;

    value1 = GetLastPnPReg( res );
    value2 = GetLastPnPReg( res );
    
    if( value1 == 0x55 && value2 == 0xaa )
    {
      buf[ i >> 3 ] |= ( 1 << ( i & 7 ) );
    }

    if( i < 64 )
    {
      UBYTE bitten = value1 == 0x55 && value2 == 0xaa ? 1 : 0;

      check_sum = ISAPNP_LFSR( check_sum, bitten );
    }

    // Wait 250 µs before we read the next pair
    
    BusyWait( 250 );
  }

  if( check_sum != buf[ 8 ] )
  {
    return FALSE;
  }
  

  // Decode information
  
  if( buf[ 0 ] & 0x80 )
  {
    return FALSE;
  }
  
  id->m_Vendor[ 0 ]  = 0x40 + ( buf[ 0 ] >> 2 );
  id->m_Vendor[ 1 ]  = 0x40 + ( ( ( buf[ 0 ] & 0x03 ) << 3 ) | ( buf[ 1 ] >> 5 ) );
  id->m_Vendor[ 2 ]  = 0x40 + ( buf[ 1 ] & 0x1f );
  id->m_Vendor[ 3 ]  = 0;
  
  id->m_ProductID    = ( buf[ 2 ] << 4 ) | ( buf[ 3 ] >> 4 );
  id->m_Revision     = buf[ 3 ] & 0x0f;
  
  id->m_SerialNumber = ( buf[ 7 ] << 25 ) |
                       ( buf[ 6 ] << 16 ) |
                       ( buf[ 5 ] << 8 )  | 
                         buf[ 4 ];

  return TRUE;
}


/******************************************************************************
** Configure all cards ********************************************************
******************************************************************************/

BOOL ASMCALL
PNPISA_ConfigureCards( REG( a6, struct ISAPnPResource* res ) )
{
  res->m_RegReadData = 0x0000;
  
  SendInitiationKey( res );

  // Make all cards lose their CSN

  SetPnPReg( ISAPNP_REG_CONFIG_CONTROL,
             ISAPNP_CCF_RESET_CSN,
             res );

  // Move all cards to the isolate state
  
  SetPnPReg( ISAPNP_REG_WAKE, 0, res );
  
  // Set port to read from
  
  SetPnPReg( ISAPNP_REG_SET_RD_DATA_PORT, 0x80, res );
  res->m_RegReadData = 0x80;

  {
    struct ISAPNP_SerialIdentifier id;
    
    if( ReadSerialIdentifier( &id, res ) )
    {
      KPrintF( "Vendor: %s, Product: %ld, Revision: %ld, Serial: %08lx\n",
               id.m_Vendor, id.m_ProductID, id.m_Revision, id.m_SerialNumber );
    }
  }

  // Reset all cards

  SetPnPReg( ISAPNP_REG_CONFIG_CONTROL,
             ISAPNP_CCF_RESET        |
             ISAPNP_CCF_WAIT_FOR_KEY |
             ISAPNP_CCF_RESET_CSN,
             res );

  return FALSE;
}
