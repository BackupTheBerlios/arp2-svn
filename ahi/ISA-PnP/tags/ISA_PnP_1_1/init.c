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
#include <exec/resident.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <stdlib.h>

#include "include/resources/isapnp.h"
#include "isapnp_private.h"
#include "version.h"

#include "controller.h"
#include "init.h"
#include "pnp.h"
#include "pnp_structs.h"


static BOOL
HandleToolTypes( UBYTE**             tool_types, 
                 struct ISAPNP_Card* card,
                 struct ISAPNPBase*  res );

void
ReqA( const char* text, APTR args );

#define Req( text, args...) \
        ( { ULONG _args[] = { args }; ReqA( (text), (APTR) _args ); } )


/******************************************************************************
** Resource resident structure ************************************************
******************************************************************************/

extern const char ResName[];
extern const char IDString[];
static const APTR InitTable[4];

const struct Resident RomTag =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (struct Resident *) &RomTag + 1,
  RTF_AUTOINIT,
  VERSION,
  NT_RESOURCE,
  0,                      /* priority */
  (BYTE *) ResName,
  (BYTE *) IDString,
  (APTR) &InitTable
};


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

struct Device*         TimerBase     = NULL;
struct DosLibrary*     DOSBase       = NULL;
struct ExecBase*       SysBase       = NULL;
struct ExpansionBase*  ExpansionBase = NULL;
struct IntuitionBase*  IntuitionBase = NULL;
struct ISAPNPBase*     ISAPNPBase    = NULL;
struct UtilityBase*    UtilityBase   = NULL;

/* linker can use symbol b for symbol a if a is not defined */
#define ALIAS(a,b) asm(".stabs \"_" #a "\",11,0,0,0\n.stabs \"_" #b "\",1,0,0,0")

// Make libnix use our UtilityBase instead of her own */
ALIAS( __UtilityBase, UtilityBase );

static struct timerequest *TimerIO   = NULL;

const char ResName[]   = ISAPNPNAME;
const char IDString[]  = ISAPNPNAME " " VERS "\r\n";

static const char VersTag[] =
 "$VER: " ISAPNPNAME " " VERS " �2001 Martin Blom.\r\n";


/******************************************************************************
** Send debug to serial port **************************************************
******************************************************************************/

static UWORD rawputchar_m68k[] = 
{
  0x2C4B,             // MOVEA.L A3,A6
  0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
  0x4E75              // RTS
};


void
KPrintFArgs( UBYTE* fmt, 
             LONG*  args )
{
  RawDoFmt( fmt, args, (void(*)(void)) rawputchar_m68k, SysBase );
}


/******************************************************************************
** Resource initialization ****************************************************
******************************************************************************/

struct ISAPNPBase* ASMCALL
initRoutine( REG( d0, struct ISAPNPBase* res ),
             REG( a0, APTR                    seglist ),
             REG( a6, struct ExecBase*        sysbase ) )
{
  SysBase = sysbase;

  if( ! OpenLibs() )
  {
    // No libraries?

    Req( "Failed to open required libraries." );
  }
  else
  {
    ULONG                 actual;
    struct CurrentBinding current_binding;

    actual = GetCurrentBinding( &current_binding, 
                                sizeof( current_binding ) );

    if( actual < sizeof( current_binding ) )
    {
      // No legal CurrentBinding structure

      Req( "No legal CurrentBinding structure found." );

    }
    else
    {
      struct ConfigDev* cd = current_binding.cb_ConfigDev;

      if( cd == NULL )
      {
        // No card found

        Req( "No bridge card found." );
      }
      else
      {
        if( cd->cd_Rom.er_Manufacturer != 2150 ||
            cd->cd_Rom.er_Product      != 1 )
        {
          // Unsupported ISA bridge

          Req( "Unsupported ISA bridge: %ld/%ld.\n"
               "Only the GG2 Bus+ card is supported.", 
               cd->cd_Rom.er_Manufacturer, 
               cd->cd_Rom.er_Product );
        }
        else
        {
          if( cd->cd_BoardAddr == NULL )
          {
            // No board address?

            Req( "No board address?" );
          }
          else
          {
            // Set up the ISAPNPBase structure

            res->m_Library.lib_Node.ln_Type = NT_RESOURCE;
            res->m_Library.lib_Node.ln_Name = (STRPTR) ResName;
            res->m_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
            res->m_Library.lib_Version      = VERSION;
            res->m_Library.lib_Revision     = REVISION;
            res->m_Library.lib_IdString     = (STRPTR) IDString;

            NewList( &res->m_Cards );

            res->m_Base        = cd->cd_BoardAddr;
            res->m_RegReadData = 0x0000;

            res->m_ConfigDev   = cd;

            if( ! ISAPNP_ScanCards( res ) )
            {
              // No cards found

              Req( "No PnP ISA cards found." );
              FreeISAPNPBase( res );
            }
            else
            {
              struct ISAPNP_Card* card;

              card = ISAPNP_AllocCard( res );

              if( card == NULL )
              {
                Req( "Out of memory!" );
                FreeISAPNPBase( res );
              }
              else
              {
                static const char descr[] = "Non-PnP devices";
                char*             d;                
                
                d = AllocVec( sizeof( descr ), MEMF_PUBLIC );
                
                if( d != NULL )
                {
                  CopyMem( (void*) descr, d, sizeof( descr ) );
                  card->isapnpc_Node.ln_Name = d;
                }

                card->isapnpc_ID.isapnpid_Vendor[ 0 ] = '?';
                card->isapnpc_ID.isapnpid_Vendor[ 1 ] = '?';
                card->isapnpc_ID.isapnpid_Vendor[ 2 ] = '?';
                card->isapnpc_SerialNumber = -1;

                // Add *first*
                AddHead( &res->m_Cards, (struct Node*) card );

                // Let's see if we're to disable any cards or devices etc

                if( ! HandleToolTypes( current_binding.cb_ToolTypes, 
                                       card, res ) )
                {
                  // Error requester already displayed.
                  FreeISAPNPBase( res );
                }
                else
                {
                  if( ! ISAPNP_ConfigureCards( res ) )
                  {
                    // Unable to configure cards

                    Req( "Unable to configure the cards. This is most likely\n"
                         "because of an unresolvable hardware conflict.\n\n"
                         "Use the DISABLE_DEVICE tool type to disable one of\n"
                         "the devices in conflict." );
                    FreeISAPNPBase( res );
                  }
                  else
                  {
                    cd->cd_Flags  &= ~CDF_CONFIGME;
                    cd->cd_Driver  = res;

                    ISAPNPBase = res;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return ISAPNPBase;
}


/******************************************************************************
** Free all resources from ISAPNPBase *****************************************
******************************************************************************/

void
FreeISAPNPBase( struct ISAPNPBase* res )
{
  struct ISAPNP_Card* card;

  while( ( card = (struct ISAPNP_Card*) RemHead( &res->m_Cards ) ) )
  {
    ISAPNP_FreeCard( card, ISAPNPBase );
  }
}


/******************************************************************************
** Initialization tables ******************************************************
******************************************************************************/

static const APTR funcTable[] =
{
  ISAC_SetMasterInt,
  ISAC_GetMasterInt,
  ISAC_SetWaitState,
  ISAC_GetWaitState,
  ISAC_GetInterruptStatus,
  ISAC_GetRegByte,
  ISAC_SetRegByte,
  ISAC_GetRegWord,
  ISAC_SetRegWord,
  ISAC_ReadByte,
  ISAC_WriteByte,
  ISAC_ReadWord,
  ISAC_WriteWord,

  ISAPNP_AllocCard,
  ISAPNP_FreeCard,
  ISAPNP_AllocDevice,
  ISAPNP_FreeDevice,
  ISAPNP_AllocResourceGroup,
  ISAPNP_FreeResourceGroup,
  ISAPNP_AllocResource,
  ISAPNP_FreeResource,

  ISAPNP_ScanCards,
  ISAPNP_ConfigureCards,
  ISAPNP_FindCard,
  ISAPNP_FindDevice,

  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct ISAPNPBase ),
  (APTR) &funcTable,
  0,
  (APTR) initRoutine
};

/******************************************************************************
** OpenLibs *******************************************************************
******************************************************************************/

BOOL
OpenLibs( void )
{
  SysBase = *( (struct ExecBase**) 4 );

  /* Utility Library (libnix depends on it, and our startup-code is not
     executed when BindDriver LoadSeg()s us!) */

  /* Intuition Library */

  IntuitionBase  = (struct IntuitionBase*) OpenLibrary( "intuition.library", 37 );

  if( IntuitionBase == NULL )
  {
    return FALSE;
  }

  UtilityBase = (struct UtilityBase *) OpenLibrary( "utility.library", 37 );

  if( UtilityBase == NULL)
  {
    return FALSE;
  }

  /* DOS Library */

  DOSBase = (struct DosLibrary*) OpenLibrary( "dos.library", 37 );

  if( DOSBase == NULL )
  {
    return FALSE;
  }

  /* Expansion Library */

  ExpansionBase = (struct ExpansionBase*) OpenLibrary( EXPANSIONNAME, 37 );

  if( ExpansionBase == NULL )
  {
    return FALSE;
  }

  /* Timer Device */

  TimerIO = (struct timerequest *) AllocVec( sizeof(struct timerequest),
                                             MEMF_PUBLIC | MEMF_CLEAR );

  if( TimerIO == NULL)
  {
    return FALSE;
  }

  if( OpenDevice( "timer.device",
                  UNIT_MICROHZ,
                  (struct IORequest *)
                  TimerIO,
                  0) != 0 )
  {
    return FALSE;
  }

  TimerBase = (struct Device *) TimerIO->tr_node.io_Device;

  return TRUE;
}


/******************************************************************************
** CloseLibs *******************************************************************
******************************************************************************/

void
CloseLibs( void )
{
  if( TimerIO  != NULL )
  {
    CloseDevice( (struct IORequest *) TimerIO );
  }

  FreeVec( TimerIO );

  CloseLibrary( (struct Library*) DOSBase );
  CloseLibrary( (struct Library*) ExpansionBase );
  CloseLibrary( (struct Library*) UtilityBase );
  CloseLibrary( (struct Library*) IntuitionBase );

  TimerIO       = NULL;
  TimerBase     = NULL;
  DOSBase       = NULL;
  ExpansionBase = NULL;  
  IntuitionBase = NULL;
  UtilityBase   = NULL;
}


/******************************************************************************
** ReqA ***********************************************************************
******************************************************************************/

void
ReqA( const char* text, APTR args )
{
  struct EasyStruct es = 
  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) ISAPNPNAME " " VERS,
    (STRPTR) text,
    "OK"
  };

  if( IntuitionBase != NULL )
  {
    EasyRequestArgs( NULL, &es, NULL, args );
  }
}


/******************************************************************************
** Handle the tool typs *******************************************************
******************************************************************************/

static int
HexToInt( UBYTE c )
{
  if( c >= '0' && c <= '9' )
  {
    return c - '0';
  }
  else if( c >= 'A' && c <= 'F' )
  {
    return c - 'A' + 10;
  }
  else if( c >= 'a' && c <= 'f' )
  {
    return c - 'a' + 10;
  }
  else
  {
    return -1;
  }
}

// CTL0048/1236 => "CTL\0" 4 8 1236

static int
ParseID( UBYTE* string,
         LONG*  manufacturer,
         WORD*  product,
         BYTE*  revision,
         LONG*  serial )
{
  int chars = 0;

  *manufacturer = ISAPNP_MAKE_ID( ToUpper( string[ 0 ] ),
                                  ToUpper( string[ 1 ] ),
                                  ToUpper( string[ 2 ] ) );

  *product      = ( HexToInt( string[ 3 ] ) << 8 ) |
                  ( HexToInt( string[ 4 ] ) << 4 ) |
                  ( HexToInt( string[ 5 ] ) );


  if( *product == -1 )
  {
    return 0;
  }

  *revision = HexToInt( string[ 6 ] );

  if( *revision == -1 )
  {
    return 0;
  }

  chars = 7;
  
  if( serial != NULL )
  {
    if( string[ 7 ] == '/' )
    {
      int conv = StrToLong( string + 8, serial );
      
      if( conv == -1 )
      {
        return 0;
      }
      else
      {
        chars += conv;
      }
    }
    else if( string[ 7 ] == 0 || string[ 7 ] != ' ' )
    {
      *serial = -1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    if( string[ 7 ] != 0 && string[ 7 ] != ' ' )
    {
      return 0;
    }
  }

  return chars;
}


static BOOL
HandleToolTypes( UBYTE**             tool_types, 
                 struct ISAPNP_Card* card,
                 struct ISAPNPBase*  res )
{
  while( *tool_types )
  {
    if( Strnicmp( *tool_types, "DISABLE_CARD=", 13 ) == 0 )
    {
      LONG manufacturer;
      WORD product;
      BYTE revision;
      LONG serial;

      if( ParseID( *tool_types + 13, 
                   &manufacturer, &product, &revision, &serial ) )
      {
        struct ISAPNP_Card* card = NULL;

        while( ( card = ISAPNP_FindCard( card,
                                         manufacturer,
                                         product,
                                         revision,
                                         serial,
                                         res ) ) != NULL )
        {
          card->isapnpc_Disabled = TRUE;
        }
      }
      else
      {
        Req( "Illegal tool type: %s\n", (ULONG) *tool_types );
        return FALSE;
      }
    }
    else if( Strnicmp( *tool_types, "DISABLE_DEVICE=", 15 ) == 0 )
    {
      LONG manufacturer;
      WORD product;
      BYTE revision;

      if( ParseID( *tool_types + 15, 
                   &manufacturer, &product, &revision, NULL ) )
      {
        struct ISAPNP_Device* dev = NULL;

        while( ( dev = ISAPNP_FindDevice( dev,
                                          manufacturer,
                                          product,
                                          revision,
                                          res ) ) != NULL )
        {
          dev->isapnpd_Disabled = TRUE;
        }
      }
      else
      {
        Req( "Illegal tool type value: %s\n", (ULONG) *tool_types );
        return FALSE;
      }
    }
    else if( Strnicmp( *tool_types, "LEGACY_DEVICE=", 14 ) == 0 )
    {
      UBYTE* str;
      int    conv;
      LONG   manufacturer;
      WORD   product;
      BYTE   revision;
      UWORD  dev_num = 0;

      str  = *tool_types + 14;
      conv = ParseID( str,  &manufacturer, &product, &revision, NULL );

      str += conv;

      if( conv != 0 )
      {
        struct ISAPNP_Device*     dev;
        struct ISAPNP_Identifier* id;

        dev = ISAPNP_AllocDevice( res );

        if( dev == NULL )
        {
          Req( "Out of memory!" );
          return FALSE;
        }

        dev->isapnpd_Card = card;
        
        id  = AllocVec( sizeof( *id ), MEMF_PUBLIC | MEMF_CLEAR );
        
        if( id == NULL )
        {
          Req( "Out of memory!" );
          ISAPNP_FreeDevice( dev, res );
          return FALSE;
        }

        id->isapnpid_Vendor[ 0 ]  = ( manufacturer >> 24 ) & 0xff;
        id->isapnpid_Vendor[ 1 ]  = ( manufacturer >> 16 ) & 0xff;
        id->isapnpid_Vendor[ 2 ]  = ( manufacturer >>  8 ) & 0xff;
        id->isapnpid_Vendor[ 3 ]  = 0;

        id->isapnpid_ProductID    = product;
        id->isapnpid_Revision     = revision;
        
        AddTail( (struct List*) &dev->isapnpd_IDs, (struct Node*) id );
        
        if( card->isapnpc_Devices.lh_Head->ln_Succ != NULL )
        {
          dev_num = ( (struct ISAPNP_Device*) 
                      card->isapnpc_Devices.lh_TailPred )->isapnpd_DeviceNumber;
          ++dev_num;
        }
        
        dev->isapnpd_DeviceNumber = dev_num;

        AddTail( &card->isapnpc_Devices, (struct Node*) dev );
        
        while( *str != 0 )
        {
          if( *str != ' ' )
          {
            if( Strnicmp( str, "IRQ=", 4 ) == 0 )
            {
              int irq;
            
              irq = strtol( str + 4, (char**) &str, 0 );
            
              if( irq <= 0 || irq >= 16 )
              {
                Req( "Invalid IRQ value '%ld' in tooltype line\n"
                     "'%s'",
                     irq,
                     (ULONG) *tool_types );
                return FALSE;
              }
              else
              {
                struct ISAPNP_IRQResource* r;
        
                r = (struct ISAPNP_IRQResource*) 
                    ISAPNP_AllocResource( ISAPNP_NT_IRQ_RESOURCE, res );
            
                if( r == NULL )
                {
                  Req( "Out of memory!" );
                  return FALSE;
                }

                r->isapnpirqr_IRQMask = 1 << irq;
                r->isapnpirqr_IRQType = ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE;
          
                AddTail( (struct List*) &dev->isapnpd_Options->isapnprg_Resources,
                         (struct Node*) r );
              }
            }
            else if( Strnicmp( str, "DMA=", 4 ) == 0 )
            {
              int dma;
            
              dma = strtol( str + 4, (char**) &str, 0 );
            
              if( dma <= 0 || dma >= 8 )
              {
                Req( "Invalid DMA value '%ld' in tooltype line\n"
                     "'%s'",
                     dma,
                     (ULONG) *tool_types );
                return FALSE;
              }
              else
              {
                struct ISAPNP_DMAResource* r;
        
                r = (struct ISAPNP_DMAResource*) 
                    ISAPNP_AllocResource( ISAPNP_NT_DMA_RESOURCE, res );
            
                if( r == NULL )
                {
                  Req( "Out of memory!" );
                  return FALSE;
                }

                r->isapnpdmar_ChannelMask = 1 << dma;
                r->isapnpdmar_Flags       = 0;
          
                AddTail( (struct List*) &dev->isapnpd_Options->isapnprg_Resources,
                         (struct Node*) r );
              }
            }
            else if( Strnicmp( str, "IO=", 3 ) == 0 )
            {
              int base;
              int length;

              struct ISAPNP_IOResource* r;
            
              base = strtol( str + 3, (char**) &str, 0 );

              if( *str != '/' )
              {
                Req( "Length missing from IO value in tooltype line\n"
                     "'%s'",
                     (ULONG) *tool_types );
                return FALSE;
              }

              ++str;

              length = strtol( str, (char**) &str, 0 );

              if( base <= 0 || base >= 0xffff )
              {
                Req( "Invalid IO base value '%ld' in tooltype line\n"
                     "'%s'",
                     base,
                     (ULONG) *tool_types );
                return FALSE;
              }

              if( length <= 0 || length >= 0xffff )
              {
                Req( "Invalid IO length value '%ld' in tooltype line\n"
                     "'%s'",
                     length,
                     (ULONG) *tool_types );
                return FALSE;
              }

              r = (struct ISAPNP_IOResource*) 
                  ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
            
              if( r == NULL )
              {
                Req( "Out of memory!" );
                return FALSE;
              }

              r->isapnpior_MinBase   = base;
              r->isapnpior_MaxBase   = base;
              r->isapnpior_Length    = length;
              r->isapnpior_Alignment = 1;
          
              AddTail( (struct List*) &dev->isapnpd_Options->isapnprg_Resources,
                       (struct Node*) r );
            }
            else
            {
              Req( "Parse error near '%s'\n"
                   "in tooltype line\n"
                   "'%s'",
                   (ULONG) str,
                   (ULONG) *tool_types );
              return FALSE;
            }
          }
          
          if( *str )
          {
            ++str;
          }
        }
      }
      else
      {
        Req( "Illegal tool type: '%s'\n", (ULONG) *tool_types );
        return FALSE;
      }
    }
    else
    {
      // Ignore unknown tool types
    }

    ++tool_types;
  }

  return TRUE;
}
