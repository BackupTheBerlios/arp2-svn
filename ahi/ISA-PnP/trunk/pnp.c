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
#include "pnp_iterators.h"
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
  UBYTE                        check_sum     = 0;
  UWORD                        dev_num       = 0;
  BOOL                         read_more     = TRUE;
  struct ISAPNP_Device*        dev           = NULL;
  struct ISAPNP_ResourceGroup* options       = NULL;
  struct ISAPNP_ResourceGroup* saved_options = NULL;

  while( read_more )
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

        dev = ISAPNP_AllocDevice( res );
        
        if( dev == NULL )
        {
          break;
        }

        dev->m_Card = card;

        id  = AllocVec( sizeof( *id ), MEMF_PUBLIC | MEMF_CLEAR );
        
        if( id == NULL )
        {
          ISAPNP_FreeDevice( dev, res );
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
          ISAPNP_FreeDevice( dev, res );
          dev = NULL;
          break;
        }
        
        AddTail( (struct List*) &dev->m_IDs, (struct Node*) id );

        options = dev->m_Options;

        dev->m_SupportedCommands = GetNextResourceData( res );
        --length;
        
        if( length > 0 )
        {
          dev->m_SupportedCommands |= GetNextResourceData( res ) << 8;
        }
        
        dev->m_DeviceNumber = dev_num;
        ++dev_num;

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

      case PNPISA_RDN_IRQ_FORMAT:
      {
        struct ISAPNP_IRQResource* r;
        
        r = (struct ISAPNP_IRQResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_IRQ_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->m_IRQMask  = GetNextResourceData( res );
        r->m_IRQMask |= GetNextResourceData( res ) << 8;
        length -= 2;
        
        if( length > 0 )
        {
          r->m_IRQType = GetNextResourceData( res );
          --length;
        }
        else
        {
          r->m_IRQType = ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE;
        }

        AddTail( (struct List*) &options->m_Resources, (struct Node*) r );

        break;
      }


      case PNPISA_RDN_DMA_FORMAT:
      {
        struct ISAPNP_DMAResource* r;
        
        r = (struct ISAPNP_DMAResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_DMA_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->m_ChannelMask = GetNextResourceData( res );
        r->m_Flags       = GetNextResourceData( res );

        length -= 2;

        AddTail( (struct List*) &options->m_Resources, (struct Node*) r );

        break;
      }

      case PNPISA_RDN_START_DF:
      {
        struct ISAPNP_ResourceGroup* rg;
        UBYTE  pri;
        
        if( length > 0 )
        {
          pri = GetNextResourceData( res );
          --length;
        }
        else
        {
          pri = 1;
        }
        
        switch( pri )
        {
          case 0:
            pri = ISAPNP_RG_PRI_GOOD;
            break;
            
          case 1:
          default:
            pri = ISAPNP_RG_PRI_ACCEPTABLE;
            break;

          case 2:
            pri = ISAPNP_RG_PRI_SUBOPTIMAL;
            break;
        }
        
        rg = ISAPNP_AllocResourceGroup( pri, res );
        
        if( rg == NULL )
        {
          break;
        }

        // Insert in priority order
        
        if( saved_options == NULL )
        {
          saved_options = options;
        }

        Enqueue( (struct List*) &saved_options->m_ResourceGroups, 
                 (struct Node*) rg );

        options = rg;

        break;
      }
      

      case PNPISA_RDN_END_DF:
      {
        options       = saved_options;
        saved_options = NULL;
        
        break;
      }

      case PNPISA_RDN_IO_PORT:
      {
        struct ISAPNP_IOResource* r;
        
        r = (struct ISAPNP_IOResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->m_Flags      = GetNextResourceData( res );

        r->m_MinBase    = GetNextResourceData( res );
        r->m_MinBase   |= GetNextResourceData( res ) << 8;

        r->m_MaxBase    = GetNextResourceData( res );
        r->m_MaxBase   |= GetNextResourceData( res ) << 8;

        r->m_Alignment  = GetNextResourceData( res );
        r->m_Length     = GetNextResourceData( res );

        length -= 7;

        AddTail( (struct List*) &options->m_Resources, (struct Node*) r );

        break;
      }


      case PNPISA_RDN_FIXED_IO_PORT:
      {
        struct ISAPNP_IOResource* r;
        
        r = (struct ISAPNP_IOResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->m_Flags      = 0;
        r->m_MinBase    = GetNextResourceData( res );
        r->m_MinBase   |= GetNextResourceData( res ) << 8;
        r->m_MaxBase    = r->m_MinBase;
        r->m_Alignment  = 1;
        r->m_Length     = GetNextResourceData( res );
        
        length -= 3;

        AddTail( (struct List*) &options->m_Resources, (struct Node*) r );

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
      

      case PNPISA_RDN_END_TAG:
      {
        UBYTE cs;
        
        cs = GetNextResourceData( res );
        --length;
        
        if( cs == 0 )
        {
          check_sum = 0;
        }
        else
        {
          check_sum += cs;
        }

        read_more = FALSE;
        break;
      }


      default:
        KPrintF( "UNKNOWN RESOURCE: %02lx, length %ld: ", name, length );

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
      KPrintF( "*** " );
      GetNextResourceData( res );
      --length;
    }
  }

  KPrintF( "Check sum: %ld\n", check_sum );
  
  return TRUE;
}


/******************************************************************************
** Assign a CSN to all cards and build the card/device database ***************
******************************************************************************/

static ULONG
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

    card = ISAPNP_AllocCard( res );

    if( card == NULL )
    {
      break;
    }

    if( ReadSerialIdentifier( card, res ) )
    {
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
        ISAPNP_FreeCard( card, res );
      }
    }
    else
    {
      ISAPNP_FreeCard( card, res );

      break;
    }
  }

  return csn;
}


/******************************************************************************
** Prints information about all cards on a serial port terminal ***************
******************************************************************************/

static void
ShowResource( struct ISAPNP_Resource* resource,
              struct ISAPNPBase*      res )
{
  switch( resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r = (struct ISAPNP_IRQResource*) resource;
      int                        b;
      
      KPrintF( "IRQ" );
      
      for( b = 0; b < 16; ++b )
      {
        if( r->m_IRQMask & ( 1 << b ) )
        {
          KPrintF( " %ld", b );
        }
      }
      
      KPrintF( ", type" );
      
      if( r->m_IRQType & ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE )
      {
        KPrintF( " +E" );
      }

      if( r->m_IRQType & ISAPNP_IRQRESOURCE_ITF_LOW_EDGE )
      {
        KPrintF( " -E" );
      }

      if( r->m_IRQType & ISAPNP_IRQRESOURCE_ITF_HIGH_LEVEL )
      {
        KPrintF( " +L" );
      }

      if( r->m_IRQType & ISAPNP_IRQRESOURCE_ITF_LOW_LEVEL )
      {
        KPrintF( " -L" );
      }
      
      KPrintF( "\n" );
      break;
    }

    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r = (struct ISAPNP_DMAResource*) resource;
      int                        b;
      
      KPrintF( "DMA" );
      
      for( b = 0; b < 8; ++b )
      {
        if( r->m_ChannelMask & ( 1 << b ) )
        {
          KPrintF( " %ld", b );
        }
      }

      KPrintF( ", " );
      switch( r->m_Flags & ISAPNP_DMARESOURCE_F_TRANSFER_MASK )
      {
        case ISAPNP_DMARESOURCE_F_TRANSFER_8BIT:
          KPrintF( "8" );
          break;

        case ISAPNP_DMARESOURCE_F_TRANSFER_BOTH:
          KPrintF( "8 and 16" );
          break;
          
        case ISAPNP_DMARESOURCE_F_TRANSFER_16BIT:
          KPrintF( "16" );
          break;
        
      }
      KPrintF( " bit transfer, " );
      
      switch( r->m_Flags & ISAPNP_DMARESOURCE_F_SPEED_MASK )
      {
        case ISAPNP_DMARESOURCE_F_SPEED_COMPATIBLE:
          KPrintF( "compatible" );
          break;

        case ISAPNP_DMARESOURCE_F_SPEED_TYPE_A:
          KPrintF( "type A" );
          break;

        case ISAPNP_DMARESOURCE_F_SPEED_TYPE_B:
          KPrintF( "type B" );
          break;

        case ISAPNP_DMARESOURCE_F_SPEED_TYPE_F:     
          KPrintF( "type F" );
          break;
      }
      
      KPrintF( " speed." );

      if( r->m_Flags & ISAPNP_DMARESOURCE_FF_BUS_MASTER )
      {
        KPrintF( " [Bus master]" );
      }

      if( r->m_Flags & ISAPNP_DMARESOURCE_FF_BYTE_MODE )
      {
        KPrintF( " [Byte mode]" );
      }

      if( r->m_Flags & ISAPNP_DMARESOURCE_FF_WORD_MODE )
      {
        KPrintF( " [Word mode]" );
      }

      KPrintF( "\n" );

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r = (struct ISAPNP_IOResource*) resource;

      if( r->m_MinBase == r->m_MaxBase )
      {
        KPrintF( "IO at %04lx, length %02lx.",
                 r->m_MinBase, r->m_Length );
      }
      else
      {
        KPrintF( "IO between %04lx and %04lx, length %02lx, %ld byte aligned.",
                 r->m_MinBase, r->m_MaxBase, r->m_Length, r->m_Alignment );
      }
           
      if( ( r->m_Flags & ISAPNP_IORESOURCE_FF_FULL_DECODE ) == 0 )
      {
        KPrintF( " [10 bit decode only]" );
      }

      KPrintF( "\n" );
      break;
    }


    case ISAPNP_NT_MEMORY_RESOURCE:
      KPrintF( "Memory\n" );
      break;

    default:
      KPrintF( "Unknown resource!" );
      break;
  }
}

static void
ShowResourceGroup( struct ISAPNP_ResourceGroup* resource_group,
                   struct ISAPNPBase* res )
{
  struct ISAPNP_Resource*      r;
  struct ISAPNP_ResourceGroup* rg;

  for( r = (struct ISAPNP_Resource*) resource_group->m_Resources.mlh_Head;
       r->m_MinNode.mln_Succ != NULL;
       r = (struct ISAPNP_Resource*) r->m_MinNode.mln_Succ )
  {
    KPrintF( "      " );
    ShowResource( r, res );
  }

  if( resource_group->m_ResourceGroups.mlh_Head->mln_Succ != NULL )  
  {
    KPrintF( "    One of\n" );

    for( rg = (struct ISAPNP_ResourceGroup*) resource_group->m_ResourceGroups.mlh_Head;
         rg->m_MinNode.mln_Succ != NULL;
         rg = (struct ISAPNP_ResourceGroup*) rg->m_MinNode.mln_Succ )
    {
      KPrintF( "    {\n" );
      ShowResourceGroup( rg, res );
      KPrintF( "    }\n" );
    }
  }
}


static void
ShowCards( struct ISAPNPBase* res )
{
  struct ISAPNP_Card* card;

  for( card = (struct ISAPNP_Card*) res->m_Cards.lh_Head; 
       card->m_Node.ln_Succ != NULL; 
       card = (struct ISAPNP_Card*) card->m_Node.ln_Succ )
  {
    struct ISAPNP_Device* dev;
    int                   dev_id;

    KPrintF( "Card %ld: %s%03lx%lx ('%s')\n",
             card->m_CSN, 
             card->m_ID.m_Vendor, card->m_ID.m_ProductID, card->m_ID.m_Revision,
             card->m_Node.ln_Name != NULL ? card->m_Node.ln_Name : "" );

    for( dev = (struct ISAPNP_Device*) card->m_Devices.lh_Head;
         dev->m_Node.ln_Succ != NULL; 
         dev = (struct ISAPNP_Device*) dev->m_Node.ln_Succ )
    {
      struct ISAPNP_Identifier* id;

      KPrintF( "  Logical device %ld: ",
               dev->m_DeviceNumber );

      for( id = (struct ISAPNP_Identifier*) dev->m_IDs.mlh_Head;
           id->m_MinNode.mln_Succ != NULL;
           id = (struct ISAPNP_Identifier*) id->m_MinNode.mln_Succ )
      {
        KPrintF( "%s%03lx%lx ", id->m_Vendor, id->m_ProductID, id->m_Revision );
      }

      if( dev->m_Node.ln_Name != NULL )
      {
        KPrintF( "('%s')", dev->m_Node.ln_Name );
      }

      KPrintF( "\n" );

//      KPrintF( "    Resources:\n" );
//      ShowResourceGroup( dev->m_Options, res );
    }
  }
}


/******************************************************************************
** Scan for all PNP ISA cards *************************************************
******************************************************************************/

BOOL ASMCALL
ISAPNP_ScanCards( REG( a6, struct ISAPNPBase* res ) )
{
  int read_port_value;
  int cards = 0;
 
  SendInitiationKey( res );

  // Make all cards lose their CSN

  SetPnPReg( PNPISA_REG_CONFIG_CONTROL,
             PNPISA_CCF_RESET_CSN,
             res );

  for( read_port_value = 0x80; 
       read_port_value <= 0xff; 
       read_port_value += 0x10 )
  {
    cards = AddCards( read_port_value, res );

    if( cards > 0 )
    {
      break;
    }
  }

  if( cards == 0 )
  {
    KPrintF( "Failed to find PNP ISA cards\n" );
  }
  else
  {
    ShowCards( res );
  }

  // Reset all cards

  SetPnPReg( PNPISA_REG_CONFIG_CONTROL,
             PNPISA_CCF_RESET        |
             PNPISA_CCF_WAIT_FOR_KEY |
             PNPISA_CCF_RESET_CSN,
             res );

  return cards != 0;
}


/******************************************************************************
** Configure all cards ********************************************************
******************************************************************************/


static BOOL
FindNextCardConfiguration( struct ISAPNP_Device*   dev,
                           struct ResourceContext* ctx,
                           struct ISAPNPBase*      res );


static BOOL
FindConfiguration( struct ISAPNP_Device*   dev,
                   struct ResourceContext* ctx,
                   struct ISAPNPBase*      res )
{
  BOOL rc = FALSE;

  struct ISAPNP_ResourceGroup* rg;
  struct ResourceIteratorList* ril = NULL;

  {
    struct ISAPNP_Identifier* id;

    KPrintF( "Logical device %ld: ",
             dev->m_DeviceNumber );

    for( id = (struct ISAPNP_Identifier*) dev->m_IDs.mlh_Head;
         id->m_MinNode.mln_Succ != NULL;
         id = (struct ISAPNP_Identifier*) id->m_MinNode.mln_Succ )
    {
      KPrintF( "%s%03lx%lx ", id->m_Vendor, id->m_ProductID, id->m_Revision );
    }

    if( dev->m_Node.ln_Name != NULL )
    {
      KPrintF( "('%s')", dev->m_Node.ln_Name );
    }

    KPrintF( "\n" );
  }

  ril = AllocResourceIteratorList( &dev->m_Options->m_Resources, ctx );

  if( ril != NULL )
  {
    BOOL ril_iter_ok = TRUE;

    while( ! rc && ril_iter_ok )
    {
      if( dev->m_Options->m_ResourceGroups.mlh_Head->mln_Succ != NULL )
      {
        // Handle resource options as well

        for( rg = (struct ISAPNP_ResourceGroup*) 
                  dev->m_Options->m_ResourceGroups.mlh_Head;
             rg->m_MinNode.mln_Succ != NULL;
             rg = (struct ISAPNP_ResourceGroup*) rg->m_MinNode.mln_Succ )
        {
          struct ResourceIteratorList* ril_option = NULL;

          ril_option = AllocResourceIteratorList( &rg->m_Resources, ctx );

          if( ril_option != NULL )
          {
            BOOL ril2_iter_ok = TRUE;

            while( ! rc && ril2_iter_ok )
            {
              rc = FindNextCardConfiguration( dev, ctx, res );
              
              if( ! rc )
              {
                ril2_iter_ok = IncResourceIteratorList( ril, ctx );
              }
            }

            FreeResourceIteratorList( ril_option, ctx );
          }
        }
      }
      else
      {
        // Fixed resources only
        
        rc = FindNextCardConfiguration( dev, ctx, res );
      }

      if( ! rc )
      {
        ril_iter_ok = IncResourceIteratorList( ril, ctx );
      }
    }

    FreeResourceIteratorList( ril, ctx );
  }
  
  return rc;
}


static BOOL
FindNextCardConfiguration( struct ISAPNP_Device*   dev,
                           struct ResourceContext* ctx,
                           struct ISAPNPBase*      res )
{
  BOOL rc = FALSE;

  // Configure next logical device, if any

  if( dev->m_Node.ln_Succ->ln_Succ != NULL )
  {
    // Same card, next device
    rc = FindConfiguration( (struct ISAPNP_Device*) dev->m_Node.ln_Succ,
                            ctx, res );
  }
  else 
  {
    struct ISAPNP_Card* next_card = (struct ISAPNP_Card*) 
                                    dev->m_Card->m_Node.ln_Succ;

    if( next_card->m_Node.ln_Succ != NULL &&
        next_card->m_Devices.lh_Head->ln_Succ != NULL )
        
    {
      rc = FindConfiguration( (struct ISAPNP_Device*)
                              next_card->m_Devices.lh_Head, ctx, res );
    }
    else
    {
      // This was the last device on the last card!
KPrintF( "End of chain!\n" );      
      rc = FALSE;
    }
  }

  return rc;
}


BOOL ASMCALL
ISAPNP_ConfigureCards( REG( a6, struct ISAPNPBase* res ) )
{
  BOOL rc = FALSE;

  struct ISAPNP_Card* card;

  card = (struct ISAPNP_Card*) res->m_Cards.lh_Head;

  // Check for non-empty card list and a non-empty device list

  if( card->m_Node.ln_Succ != NULL )
  {
    struct ISAPNP_Device* dev;
    
    dev = (struct ISAPNP_Device*) card->m_Devices.lh_Head;
    
    if( dev->m_Node.ln_Succ != NULL )
    {
      struct ResourceContext* ctx;
      
      ctx = AllocResourceIteratorContext();
      
      if( ctx != NULL )
      {
        rc = FindConfiguration( dev, ctx, res );
        KPrintF( "FindConfiguration: %ld\n", rc );
        
        FreeResourceIteratorContext( ctx );
      }

      // Force success
      rc = TRUE;
    }
  }

  return rc;
}
