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

#include "CompilerSpecific.h"

#include <exec/resident.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>

#include <proto/exec.h>
#include <proto/expansion.h>

#include "isapnp.h"
#include "isapnp_private.h"
#include "controller.h"
#include "version.h"

#if 0
/******************************************************************************
** Resource entry *************************************************************
******************************************************************************/

ULONG
Start( void )
{
  return -1;
}
#endif

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

struct ExecBase*       SysBase    = NULL;
struct ISAPnPResource* ISAPnPBase = NULL;

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


#define KPrintF( fmt, ... )        \
({                                 \
  LONG _args[] = { __VA_ARGS__ };  \
  KPrintFArgs( (fmt), _args );     \
})


/******************************************************************************
** Resource initialization ****************************************************
******************************************************************************/

struct ISAPnPResource* ASMCALL
initRoutine( REG( d0, struct ISAPnPResource* res ),
             REG( a0, APTR                   seglist ),
             REG( a6, struct ExecBase*       sysbase ) )
{
  struct ExpansionBase* ExpansionBase;

  SysBase = sysbase;

  KPrintF( "init routine called\n" );

  res->m_Library.lib_Node.ln_Type = NT_RESOURCE;
  res->m_Library.lib_Node.ln_Name = (STRPTR) ResName;
  res->m_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
  res->m_Library.lib_Version      = VERSION;
  res->m_Library.lib_Revision     = REVISION;
  res->m_Library.lib_IdString     = (STRPTR) IDString;

  ExpansionBase = (struct ExpansionBase*) OpenLibrary( EXPANSIONNAME, 37 );
  
  if( ExpansionBase == NULL )
  {
    // No expansion.library

    ISAPnPBase = NULL;
  }
  else
  {
    ULONG actual;
    
    actual = GetCurrentBinding( &res->m_CurrentBinding, 
                                sizeof( res->m_CurrentBinding ) );

    if( actual < sizeof( res->m_CurrentBinding ) )
    {
      // No legal CurrentBinding structure

    }
    else
    {
      struct ConfigDev* cd = res->m_CurrentBinding.cb_ConfigDev;
      if( cd == NULL )
      {
        // No card found
      }
      else
      {
        if( cd->cd_Rom.er_Manufacturer != 2150 ||
            cd->cd_Rom.er_Product      != 1 )
        {
          // Unsupported ISA bridge

        }
        else
        {
          res->m_Base = cd->cd_BoardAddr;

          if( res->m_Base == NULL )
          {
            // No board address?

          }
          else
          {
            cd->cd_Flags  &= ~CDF_CONFIGME;
            cd->cd_Driver  = res;
          
KPrintF( "Installed.\n" );
            ISAPnPBase = res;
          }
        }
      }
    }

    CloseLibrary( (struct Library*) ExpansionBase );
  }

  return ISAPnPBase;
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

  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct ISAPnPResource ),
  (APTR) &funcTable,
  0,
  (APTR) initRoutine
};
