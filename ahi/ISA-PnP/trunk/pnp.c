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

#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include "include/resources/isapnp.h"
#include "isapnp_private.h"

#include "pnpisa.h"

#include "controller.h"
#include "init.h"
#include "pnp.h"
#include "pnp_structs.h"

/******************************************************************************
** PnP ISA helper functions ***************************************************
******************************************************************************/

static UBYTE
GetPnPReg( UBYTE              pnp_reg,
           struct ISAPNPBase* res )
{
  ISAC_SetRegByte( PNPISA_ADDRESS, pnp_reg, res );

  return ISAC_GetRegByte( ( res->m_RegReadData << 2 ) | 3, res );
}


static UBYTE
GetLastPnPReg( struct ISAPNPBase* res )
{
  return ISAC_GetRegByte( ( res->m_RegReadData << 2 ) | 3, res );
}


static void
SetPnPReg( UBYTE              pnp_reg,
           UBYTE              value,
           struct ISAPNPBase* res )
{
  ISAC_SetRegByte( PNPISA_ADDRESS,    pnp_reg, res );
  ISAC_SetRegByte( PNPISA_WRITE_DATA, value,   res );
}


static UBYTE
GetNextResourceData( struct ISAPNPBase* res )
{
  // Poll for data
  while( ( GetPnPReg( PNPISA_REG_STATUS, res ) & PNPISA_SF_AVAILABLE ) == 0 );

  return GetPnPReg( PNPISA_REG_RESOURCE_DATA, res );

}


static void
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
DecodeID( UBYTE* buf, struct ISAPNP_Identifier* res )
{
  // Decode information

  if( buf[ 0 ] & 0x80 )
  {
    return FALSE;
  }

  res->m_Vendor[ 0 ]  = 0x40 + ( buf[ 0 ] >> 2 );
  res->m_Vendor[ 1 ]  = 0x40 + ( ( ( buf[ 0 ] & 0x03 ) << 3 ) | 
                                 ( buf[ 1 ] >> 5 ) );
  res->m_Vendor[ 2 ]  = 0x40 + ( buf[ 1 ] & 0x1f );
  res->m_Vendor[ 3 ]  = 0;

  res->m_ProductID    = ( buf[ 2 ] << 4 ) | ( buf[ 3 ] >> 4 );
  res->m_Revision     = buf[ 3 ] & 0x0f;
  
  return TRUE;
}


/******************************************************************************
** Move all cards to Sleep state **********************************************
******************************************************************************/

static void
SendInitiationKey( struct ISAPNPBase* res )
{
  static const UBYTE key[] =
  {
    PNPISA_INITIATION_KEY
  };
  
  unsigned int i;

  // Make sure the LFSR is in its initial state

  ISAC_SetRegByte( PNPISA_ADDRESS, 0, res );
  ISAC_SetRegByte( PNPISA_ADDRESS, 0, res );

  for( i = 0; i < sizeof( key ); ++i )
  {
    ISAC_SetRegByte( PNPISA_ADDRESS, key[ i ], res );
  }
}


/******************************************************************************
** Read the serial identifier from a card *************************************
******************************************************************************/

static BOOL
ReadSerialIdentifier( struct ISAPNP_Card* card,
                      struct ISAPNPBase*  res )
{
  UBYTE buf[ 9 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int   i;
  UBYTE check_sum;
  
  ISAC_SetRegByte( PNPISA_ADDRESS, PNPISA_REG_SERIAL_ISOLATION, res );

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

      check_sum = PNPISA_LFSR( check_sum, bitten );
    }

    // Wait 250 µs before we read the next pair
    
    BusyWait( 250 );
  }

  if( check_sum != buf[ 8 ] )
  {
    return FALSE;
  }

  if( ! DecodeID( buf, &card->m_ID ) )
  {
    return FALSE;
  }
  
  card->m_SerialNumber = ( buf[ 7 ] << 25 ) |
                         ( buf[ 6 ] << 16 ) |
                         ( buf[ 5 ] << 8 )  | 
                           buf[ 4 ];

  return TRUE;
}


/******************************************************************************
** Read the resource data from a card *****************************************
******************************************************************************/

static BOOL
ReadResourceData( struct ISAPNP_Card* card,
                  struct ISAPNPBase*  res )
{
  UBYTE                 check_sum = 0;
  struct ISAPNP_Device* dev       = NULL;

  while( TRUE )
  {
    ULONG i;
    UBYTE rd;

    UBYTE name;
    ULONG length;

    rd = GetNextResourceData( res );

    check_sum += rd;

    if( rd & PNPISA_RDF_LARGE )
    {
      name    = PNPISA_RD_LARGE_ITEM_NAME( rd );
      length  = GetNextResourceData( res );
      length |= GetNextResourceData( res ) << 8;
    }
    else
    {
      name   = PNPISA_RD_SMALL_ITEM_NAME( rd );
      length = PNPISA_RD_SMALL_LENGTH( rd );
    }

    switch( name )
    {
      case PNPISA_RDN_PNP_VERSION:
      {
        UBYTE ver;
        
        ver = GetNextResourceData( res );

        card->m_MinorPnPVersion  = ver & 0x0f;
        card->m_MajorPnPVersion  = ver >> 4;
        card->m_VendorPnPVersion = GetNextResourceData( res );
        
        length -= 2;
        break;
      }

      case PNPISA_RDN_LOGICAL_DEVICE_ID:
      {
        struct ISAPNP_Identifier* id;
        UBYTE                     buf[ 4 ];

        dev = PNPISA_AllocDevice( res );
        
        if( dev == NULL )
        {
          break;
        }

        id  = AllocVec( sizeof( *id ), MEMF_PUBLIC | MEMF_CLEAR );
        
        if( id == NULL )
        {
          PNPISA_FreeDevice( dev, res );
          dev = NULL;
          break;
        }
        
        buf[ 0 ] = GetNextResourceData( res );
        buf[ 1 ] = GetNextResourceData( res );
        buf[ 2 ] = GetNextResourceData( res );
        buf[ 3 ] = GetNextResourceData( res );

        length -= 4;

        if( ! DecodeID( buf, id ) )
        {
          FreeVec( id );
          PNPISA_FreeDevice( dev, res );
          dev = NULL;
          break;
        }
        
        AddTail( (struct List*) &dev->m_IDs, (struct Node*) id );

        dev->m_SupportedCommands = GetNextResourceData( res );
        --length;
        
        if( length > 0 )
        {
          dev->m_SupportedCommands |= GetNextResourceData( res ) << 8;
        }
        
        AddTail( &card->m_Devices, (struct Node*) dev );

        break;
      }
      
      case PNPISA_RDN_COMPATIBLE_DEVICE_ID:
      {
        struct ISAPNP_Identifier* id;
        UBYTE                     buf[ 4 ];

        if( dev == NULL )
        {
          break;
        }

        id  = AllocVec( sizeof( *id ), MEMF_PUBLIC | MEMF_CLEAR );
        
        if( id == NULL )
        {
          break;
        }

        buf[ 0 ] = GetNextResourceData( res );
        buf[ 1 ] = GetNextResourceData( res );
        buf[ 2 ] = GetNextResourceData( res );
        buf[ 3 ] = GetNextResourceData( res );

        length -= 4;

        if( ! DecodeID( buf, id ) )
        {
          FreeVec( id );
          break;
        }

        AddTail( (struct List*) &dev->m_IDs, (struct Node*) id );

        break;
      }

      case PNPISA_RDN_ANSI_IDENTIFIER:
      {
        STRPTR id = AllocVec( length + 1, MEMF_PUBLIC );
        
        if( id == NULL )
        {
          break;
        }

        // Attach identifier to either the card or the logical device

        if( dev == NULL )
        {
          FreeVec( card->m_Node.ln_Name );

          card->m_Node.ln_Name = id;
        }
        else
        {
          FreeVec( dev->m_Node.ln_Name );

          dev->m_Node.ln_Name = id;
        }
        
        while( length > 0 )
        {
          *id = GetNextResourceData( res );

          ++id;
          --length;
        }
        
        // Terminate the string

        *id = '\0';

        break;
      }
      
      default:
        KPrintF( "%02lx, length %ld: ", name, length );

        while( length > 0 )
        {
          KPrintF( "%02lx ", GetNextResourceData( res ) );
          --length;
        }

        KPrintF( "\n" );
        break;
    }

    // Make sure we have read all data for this name

    while( length > 0 )
    {
      KPrintF( "!" );
      GetNextResourceData( res );
      --length;
    }

    if( name == PNPISA_RDN_END_TAG )
    {
      break;
    }
  }

  KPrintF( "Check sum: %ld\n", check_sum );
  
  return TRUE;
}


/******************************************************************************
** Assign a CSN to all cards and build the card/device database ***************
******************************************************************************/

ULONG
AddCards( UBYTE              rd_data_port_value, 
          struct ISAPNPBase* res )
{
  UBYTE csn = 0;

  while( TRUE )
  {
    struct ISAPNP_Card* card;

    // Move all cards (without a CSN) to the isolate state and the card
    // in config state to the sleep state.
  
    SetPnPReg( PNPISA_REG_WAKE, 0, res );
  
    // Set port to read from
  
    SetPnPReg( PNPISA_REG_SET_RD_DATA_PORT, rd_data_port_value, res );
    res->m_RegReadData = rd_data_port_value;

    card = PNPISA_AllocCard( res );

    if( card == NULL )
    {
      break;
    }

    if( ReadSerialIdentifier( card, res ) )
    {
      KPrintF( "Vendor: %s, Product: %03lx, Revision: %lx, Serial: %08lx\n",
               card->m_ID.m_Vendor, card->m_ID.m_ProductID, card->m_ID.m_Revision, card->m_SerialNumber );

      // Assign a CSN to the card and move it to the config state

      ++csn;
      SetPnPReg( PNPISA_REG_CARD_SELECT_NUMBER, csn, res );

      // Read the resource data
      
      if( ReadResourceData( card, res ) )
      {
        card->m_CSN = csn;
      
        AddTail( &res->m_Cards, (struct Node*) card );
      }
      else
      {
        PNPISA_FreeCard( card, res );
      }
    }
    else
    {
      PNPISA_FreeCard( card, res );

      break;
    }
  }

  return csn;
}


/******************************************************************************
** Configure all cards ********************************************************
******************************************************************************/

BOOL ASMCALL
PNPISA_ConfigureCards( REG( a6, struct ISAPNPBase* res ) )
{
  int read_port_value;
 
  SendInitiationKey( res );

  // Make all cards lose their CSN

  SetPnPReg( PNPISA_REG_CONFIG_CONTROL,
             PNPISA_CCF_RESET_CSN,
             res );

  for( read_port_value = 0x80; 
       read_port_value <= 0xff; 
       read_port_value += 0x10 )
  {
    if( AddCards( read_port_value, res ) > 0 )
    {
      break;
    }
  }

  if( res->m_Cards.lh_Head->ln_Succ == NULL )
  {
    KPrintF( "Failed to find PNP ISA cards\n" );
  }

  // Reset all cards

  SetPnPReg( PNPISA_REG_CONFIG_CONTROL,
             PNPISA_CCF_RESET        |
             PNPISA_CCF_WAIT_FOR_KEY |
             PNPISA_CCF_RESET_CSN,
             res );

  return TRUE;
}
