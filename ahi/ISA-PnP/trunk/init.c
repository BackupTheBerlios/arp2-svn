/* $Id$ */

#include "CompilerSpecific.h"

#include <exec/resident.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>

#include <proto/exec.h>
#include <proto/expansion.h>

#include "isapnp.h"
#include "isapnp_private.h"
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
#if defined( ENABLE_MORPHOS )
  RTF_PPC | RTF_AUTOINIT,
#else
  RTF_AUTOINIT,
#endif
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
** Resource initialization ****************************************************
******************************************************************************/

struct ISAPnPResource*
initRoutine( struct ISAPnPResource*  res,
             APTR                    seglist,
             struct ExecBase*        sysbase )
{
  struct ExpansionBase* ExpansionBase;

  SysBase    = sysbase;

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
            cd->cd_Flags &= ~CDF_CONFIGME;
          
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
** m68k wrappers **************************************************************
******************************************************************************/

/* gw_initRoutine ************************************************************/

struct ISAPnPResource* ASMCALL
gw_initRoutine( REG( d0, struct ISAPnPResource* res ),
                REG( a0, APTR                   seglist ),
                REG( a6, struct ExecBase*       sysbase ) )
{
  return initRoutine( res, seglist, sysbase );
}


/******************************************************************************
** Initialization tables ******************************************************
******************************************************************************/

static const APTR funcTable[] =
{

#if defined( ENABLE_MORPHOS )
  (APTR) FUNCARRAY_32BIT_NATIVE,
#endif

  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct ISAPnPResource ),
  (APTR) &funcTable,
  0,
  (APTR) gw_initRoutine
};


#if 0

ISA::ISA( ULONG manufacturer,
          ULONG product,
          int   card )
  throw( NoCard )
{
  struct ConfigDev *cd = NULL;

  do
  {
    cd = FindConfigDev( cd , manufacturer, product );
    card--;
  } while( cd != NULL && card > 0 );

  if( cd == NULL )
  {
    throw NoCard();
  }

  m_Base         = reinterpret_cast<UBYTE*>( cd->cd_BoardAddr );
  m_Manufacturer = manufacturer;
  m_Product      = product;
}


#endif
