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
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/utility.h>

#include "include/resources/isapnp.h"
#include "isapnp_private.h"
#include "version.h"

#include "controller.h"
#include "init.h"
#include "pnp.h"
#include "pnp_structs.h"


static BOOL
HandleToolTypes( UBYTE**            tool_types, 
                 struct ISAPNPBase* res );


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
 "$VER: " ISAPNPNAME " " VERS " ©2001 Martin Blom.\r\n";


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

KPrintF( "No libraries?.\n" );

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

KPrintF( "No legal CurrentBinding structure.\n" );

    }
    else
    {
      struct ConfigDev* cd = current_binding.cb_ConfigDev;

      if( cd == NULL )
      {
        // No card found

KPrintF( "No card found.\n" );
      }
      else
      {
        if( cd->cd_Rom.er_Manufacturer != 2150 ||
            cd->cd_Rom.er_Product      != 1 )
        {
          // Unsupported ISA bridge

KPrintF( "Unsupported ISA bridge: %ld/%ld.\n", 
         cd->cd_Rom.er_Manufacturer, cd->cd_Rom.er_Product );
        }
        else
        {
          if( cd->cd_BoardAddr == NULL )
          {
            // No board address?

KPrintF( "No board address?\n" );
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

KPrintF( "No cards found.\n" );
            }
            else
            {
              // Let's see if we're to disable any cards or devices

              if( ! HandleToolTypes( current_binding.cb_ToolTypes, res ) )
              {
KPrintF( "Unable to handle tool types.\n" );
              }
              else
              {
                if( ! ISAPNP_ConfigureCards( res ) )
                {
                  // Unable to configure cards

KPrintF( "Unable to configure cards.\n" );
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

  return ISAPNPBase;
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

  /* Utility Library (libnix depends on it, and out startup-code is not
     executed when BindDriver LoadSeg()s us!) */

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

  TimerIO       = NULL;
  TimerBase     = NULL;
  DOSBase       = NULL;
  ExpansionBase = NULL;  
  UtilityBase   = NULL;
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

static BOOL
ParseID( UBYTE* string,
         LONG*  manufacturer,
         WORD*  product,
         BYTE*  revision,
         LONG*  serial )
{
  *manufacturer = ISAPNP_MAKE_ID( ToUpper( string[ 0 ] ),
                                  ToUpper( string[ 1 ] ),
                                  ToUpper( string[ 2 ] ) );

  *product      = ( HexToInt( string[ 3 ] ) << 8 ) |
                  ( HexToInt( string[ 4 ] ) << 4 ) |
                  ( HexToInt( string[ 5 ] ) );


  if( *product == -1 )
  {
    return FALSE;
  }

  *revision = HexToInt( string[ 6 ] );

  if( *revision == -1 )
  {
    return FALSE;
  }
  
  if( serial != NULL )
  {
    if( string[ 7 ] == '/' )
    {
      if( StrToLong( string + 8, serial ) == -1 )
      {
        return FALSE;
      }
    }
    else if( string[ 7 ] == 0 )
    {
      *serial = -1;
    }
    else
    {
      return FALSE;
    }
  }
  else
  {
    if( string[ 7 ] != 0 )
    {
      return FALSE;
    }
  }

  return TRUE;
}


static BOOL
HandleToolTypes( UBYTE**            tool_types, 
                 struct ISAPNPBase* res )
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
        KPrintF( "Illegal tool type: %s\n", *tool_types );
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
        KPrintF( "Illegal tool type: %s\n", *tool_types );
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
