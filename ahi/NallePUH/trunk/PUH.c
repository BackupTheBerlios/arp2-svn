/* $Id$ */

/*
     NallePUH -- Paula utan henne -- A minimal Paula emulator.
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

#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <mmu/context.h>
#include <mmu/descriptor.h>
#include <mmu/exceptions.h>
#include <mmu/mmubase.h>
#include <mmu/mmutags.h>

#include <proto/exec.h>
#include <proto/mmu.h>

#include <stdio.h>

#include "PUH.h"

#define ROMEND       0x01000000L
#define MAGIC_ROMEND 0x14L


static BOOL
RemapMemory( struct PUHData* pd );


static BOOL
RestoreMemory( struct PUHData* pd );


static void
PatchROMShadowBuffer( struct PUHData* pd );


ASMCALL INTERRUPT ULONG
PUHHandler( REG( a0, struct ExceptionData* ed ),
            REG( a1, struct PUHData*       d ),
            REG( a6, struct Library*       MMUBase ) );


UWORD
PUHRead( UWORD            reg, 
         struct PUHData*  pd,
         struct ExecBase* SysBase );


void
PUHWrite( UWORD            reg, 
          UWORD            value,
          struct PUHData*  pd,
          struct ExecBase* SysBase );



/******************************************************************************
** The chip registers we trigger on *******************************************
******************************************************************************/

#define DMACONR 0x002
#define ADKCONR 0x010
#define INTENAR 0x01c
#define INTREQR 0x01e
#define DMACON  0x096
#define INTENA  0x09a
#define INTREQ  0x09c
#define ADKCON  0x09e

#define AUD0LCH 0x0a0
#define AUD0LCL 0x0a2
#define AUD0LEN 0x0a4
#define AUD0PER 0x0a6
#define AUD0VOL 0x0a8
#define AUD0DAT 0x0aa

#define AUD1LCH 0x0b0
#define AUD1LCL 0x0b2
#define AUD1LEN 0x0b4
#define AUD1PER 0x0b6
#define AUD1VOL 0x0b8
#define AUD1DAT 0x0ba

#define AUD2LCH 0x0c0
#define AUD2LCL 0x0c2
#define AUD2LEN 0x0c4
#define AUD2PER 0x0c6
#define AUD2VOL 0x0c8
#define AUD2DAT 0x0ca

#define AUD3LCH 0x0d0
#define AUD3LCL 0x0d2
#define AUD3LEN 0x0d4
#define AUD3PER 0x0d6
#define AUD3VOL 0x0d8
#define AUD3DAT 0x0da

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
             ULONG* args )
{
  RawDoFmt( fmt, args, (void(*)(void)) rawputchar_m68k, SysBase );
}


/******************************************************************************
** Read and write hardware registers ******************************************
******************************************************************************/

UWORD
ReadWord( void* address )
{
  return *((UWORD*) address);
}


void
WriteWord( void* address, UWORD value )
{
  *((UWORD*) address) = value;
}


ULONG
ReadLong( void* address )
{
  return *((ULONG*) address);
}


void
WriteLong( void* address, ULONG value )
{
  *((ULONG*) address) = value;
}



/******************************************************************************
** Initialize PUH *************************************************************
******************************************************************************/

struct PUHData*
AllocPUH( void )
{
  struct PUHData* pd = NULL;

  if( MMUBase == NULL )
  {
    printf( "MMUBase not initialized!\n" );
  }


  if( GetMMUType() == MUTYPE_NONE )
  {
    printf( "This program requires an MMU.\n" );
  }
  else
  {
    struct MMUContext* uctx;
    struct MMUContext* sctx;
    struct MMUContext* ctxs[ 3 ];
    void*              location;
    ULONG              page_size;
    ULONG              size;

    // Get default and supervisor contexts

    uctx = DefaultContext();
    sctx = SuperContext( uctx );

    ctxs[ 0 ] = uctx;
    ctxs[ 1 ] = sctx;
    ctxs[ 2 ] = NULL;

    page_size = max( GetPageSize( uctx ), GetPageSize( sctx ) );

    // Allocate memory for the re-mapped registers

    size = ( ( 0x200 - 1 ) / page_size + 1 ) * page_size;

    location = AllocAligned( size, 
                             MEMF_PUBLIC | MEMF_CLEAR,
                             page_size );


    if( location == NULL )
    {
      printf( "Out of memory!\n" );
    }
    else
    {
      pd = (struct PUHData*) AllocVec( sizeof( struct PUHData ),
                                       MEMF_PUBLIC | MEMF_CLEAR );

      if( pd == NULL )
      {
        FreeMem( location, size );
        printf( "Out of memory!\n" );
      }
      else
      {
        void* rom_start    = NULL;
        ULONG rom_size     = NULL;

        // Get location of Kickstart ROM  
        rom_size  = *( (ULONG*) ( ROMEND - MAGIC_ROMEND ) );
        rom_start = ( (UBYTE*) ROMEND ) - rom_size;

        pd->m_Flags                = 0L;

        pd->m_Active               = FALSE;
        pd->m_Pad                  = 0;

#ifdef TEST_MODE
        pd->m_Intercepted          = location;
        pd->m_Custom               = (void*) 0xdff000;
#else
        pd->m_Intercepted          = (void*) 0xdff000;
        pd->m_Custom               = location;
#endif

        pd->m_UserContext          = uctx;        
        pd->m_SuperContext         = sctx;

        pd->m_UserException        = NULL;        
        pd->m_SuperException       = NULL;

        pd->m_ROM                  = rom_start;
        pd->m_ROMShadowBuffer      = NULL;
        pd->m_CustomShadowBuffer   = location;

        pd->m_ROMSize              = rom_size;
        pd->m_CustomSize           = size;

        pd->m_Properties.m_UserROMShadow     = NULL;
        pd->m_Properties.m_UserROM           = NULL;
        pd->m_Properties.m_UserCustomShadow  = NULL;
        pd->m_Properties.m_UserCustom        = NULL;
        pd->m_Properties.m_SuperROMShadow    = NULL;
        pd->m_Properties.m_SuperROM          = NULL;
        pd->m_Properties.m_SuperCustomShadow = NULL;
        pd->m_Properties.m_SuperCustom       = NULL;
      }
    }
  }

  return pd;
}


/******************************************************************************
** Deallocate PUH *************************************************************
******************************************************************************/

void
FreePUH( struct PUHData* pd )
{
  if( pd != NULL )
  {
    DeactivatePUH( pd );

    if( pd->m_CustomShadowBuffer != NULL )
    {
      FreeMem( pd->m_CustomShadowBuffer, pd->m_CustomSize );
    }
    
    FreeVec( pd );
  }
}


/******************************************************************************
** Installl PUH ***************************************************************
******************************************************************************/

BOOL
InstallPUH( ULONG           flags,
            struct PUHData* pd )
{
  pd->m_Flags = flags;

  pd->m_UserException = 
      AddContextHook( MADTAG_CONTEXT, (ULONG) pd->m_UserContext,
                      MADTAG_TYPE,    MMUEH_SEGFAULT,
                      MADTAG_CODE,    (ULONG) PUHHandler,
                      MADTAG_DATA,    (ULONG) pd,
                      MADTAG_PRI,     127,
                      TAG_DONE );

  if( pd->m_UserException == NULL )
  {
    printf( "Unable to install user context hook.\n" );
  }
  else
  {
    pd->m_SuperException =
        AddContextHook( MADTAG_CONTEXT, (ULONG) pd->m_SuperContext,
                        MADTAG_TYPE,    MMUEH_SEGFAULT,
                        MADTAG_CODE,    (ULONG) PUHHandler,
                        MADTAG_DATA,    (ULONG) pd,
                        MADTAG_PRI,     127,
                        TAG_DONE );

    if( pd->m_SuperException == NULL )
    {
      printf( "Unable to install supervisor context hook.\n" );
    }
    else
    {
      ActivateException( pd->m_UserException );
      ActivateException( pd->m_SuperException );

      // Are we supposed to patch the ROM?

      if( flags & PUHF_PATCH_ROM )
      {
        pd->m_ROMShadowBuffer = AllocAligned( pd->m_ROMSize,
                                              MEMF_PUBLIC | MEMF_FAST,
                                              RemapSize( pd->m_UserContext  ) );
       
        if( pd->m_ROMShadowBuffer == NULL )
        {
          printf( "Out of memory!\n" );
        }
        else
        {
          CopyMemQuick( pd->m_ROM, pd->m_ROMShadowBuffer, pd->m_ROMSize );

          PatchROMShadowBuffer( pd );

          CacheClearU();
        }
      }

      if( ! ( flags & PUHF_PATCH_ROM ) || pd->m_ROMShadowBuffer != NULL )
      {
        // Re-map 

        if( ! RemapMemory( pd ) )
        {
          printf( "Unable to install remap.\n" );
        }
        else
        {
          pd->m_Active = TRUE;
        }
      }
    }
  }

  return pd->m_Active;
}


/******************************************************************************
** Uninstall PUH **************************************************************
******************************************************************************/

void
UninstallPUH( struct PUHData* pd )
{
  if( pd == NULL )
  {
    return;
  }

  if( pd->m_Active )
  {
    RestoreMemory( pd );
  }

  if( pd->m_ROMShadowBuffer != NULL )
  {
    FreeMem( pd->m_ROMShadowBuffer, pd->m_ROMSize );
    pd->m_ROMShadowBuffer = NULL;
  }

  if( pd->m_SuperException != NULL )
  {
    DeactivateException( pd->m_SuperException );
    RemContextHook( pd->m_SuperException );
    pd->m_SuperException = NULL;
  }

  if( pd->m_UserException != NULL )
  {
    DeactivateException( pd->m_UserException );
    RemContextHook( pd->m_UserException );
    pd->m_UserException = NULL;
  }
  
  pd->m_Flags  = 0L;
  pd->m_Active = FALSE;
}


/******************************************************************************
** Activate PUH ***************************************************************
******************************************************************************/

BOOL
ActivatePUH( struct PUHData* pd )
{
  if( ! SetPageProperties( pd->m_UserContext,
                           MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                           MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                           (ULONG) pd->m_Intercepted, TAG_DONE ) )
  {
    return FALSE;
  }

  if( ! SetPageProperties( pd->m_SuperContext,
                           MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                           MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                           (ULONG) pd->m_Intercepted, TAG_DONE ) )
  {
    // Try to deactivate
    
    DeactivatePUH( pd );

    return FALSE;
  }
  
  return TRUE;
}


/******************************************************************************
** Deactivate PUH *************************************************************
******************************************************************************/

void
DeactivatePUH( struct PUHData* pd )
{
  SetPageProperties( pd->m_UserContext,
                     MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                     MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                     (ULONG) pd->m_Intercepted, TAG_DONE );

  SetPageProperties( pd->m_SuperContext,
                     MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                     MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                     (ULONG) pd->m_Intercepted, TAG_DONE );
}


/******************************************************************************
** Activate MMU ***************************************************************
******************************************************************************/

static BOOL
RemapMemory( struct PUHData* pd )
{
  BOOL  rc = FALSE;

  // Get properties for the areas we will modify

  pd->m_Properties.m_UserROM =
      GetProperties( pd->m_UserContext, (ULONG) pd->m_ROM, TAG_DONE );

  pd->m_Properties.m_UserROMShadow =
      GetProperties( pd->m_UserContext, (ULONG) pd->m_ROMShadowBuffer, TAG_DONE );

  pd->m_Properties.m_UserCustom = 
      GetProperties( pd->m_UserContext, (ULONG) 0xdff000, TAG_DONE );

  pd->m_Properties.m_UserCustomShadow = 
      GetProperties( pd->m_UserContext, (ULONG) pd->m_CustomShadowBuffer, TAG_DONE );


  pd->m_Properties.m_SuperROM =
      GetProperties( pd->m_SuperContext, (ULONG) pd->m_ROM, TAG_DONE );

  pd->m_Properties.m_SuperROMShadow =
      GetProperties( pd->m_SuperContext, (ULONG) pd->m_ROMShadowBuffer, TAG_DONE );

  pd->m_Properties.m_SuperCustom = 
      GetProperties( pd->m_SuperContext, (ULONG) 0xdff000, TAG_DONE );

  pd->m_Properties.m_SuperCustomShadow = 
      GetProperties( pd->m_SuperContext, (ULONG) pd->m_CustomShadowBuffer, TAG_DONE );


  if( ( pd->m_Properties.m_UserCustom  & MAPP_REPAIRABLE ) ||
      ( pd->m_Properties.m_SuperCustom & MAPP_REPAIRABLE ) )
  {
    // This is odd ...

    printf( "Custom chip register area marked 'repairable'!" );
  }
  else
  {
    struct MinList* user_mapping;
    struct MinList* super_mapping;

    LockContextList();
    LockMMUContext( pd->m_UserContext );
    LockMMUContext( pd->m_SuperContext );

    // Save current mapping if we have to abort!

    user_mapping  = GetMapping( pd->m_UserContext );
    super_mapping = GetMapping( pd->m_SuperContext );

    if( user_mapping == NULL || super_mapping == NULL )
    {
      printf( "Failed to get mappings.\n" );
    }
    else
    {
      if( ! SetProperties( pd->m_UserContext,
                           ( pd->m_Properties.m_UserCustom | 
                             MAPP_REMAPPED ),
                           ~0UL,
                           (ULONG) pd->m_CustomShadowBuffer, 
                           pd->m_CustomSize,
                           MAPTAG_DESTINATION, 0xdff000,
                           TAG_DONE ) ||
          ! SetProperties( pd->m_SuperContext,
                           ( pd->m_Properties.m_SuperCustom | 
                             MAPP_REMAPPED ),
                           ~0UL,
                           (ULONG) pd->m_CustomShadowBuffer, 
                           pd->m_CustomSize,
                           MAPTAG_DESTINATION, 0xdff000,
                           TAG_DONE ) )
      {
        printf( "Failed to set properties for re-mapped area.\n" );
      }
      else
      {
        ULONG address;

#ifdef TEST_MODE
        address = (ULONG) pd->m_CustomShadowBuffer;
#else
        address = 0xdff000;
#endif

        if( ! SetProperties( pd->m_UserContext,
                             ( pd->m_Properties.m_UserCustom | 
                               MAPP_SINGLEPAGE |
                               MAPP_REPAIRABLE ),
                             ~0UL,
                             address, 
                             pd->m_CustomSize,
                             TAG_DONE ) ||
            ! SetProperties( pd->m_SuperContext,
                             ( pd->m_Properties.m_SuperCustom | 
                               MAPP_SINGLEPAGE |
                               MAPP_REPAIRABLE ),
                             ~0UL,
                             address, 
                             pd->m_CustomSize,
                             TAG_DONE ) )
        {
          printf( "Failed to set properties for custom chip register area.\n" );
        }
        else
        {
          BOOL activate = TRUE;

          // Remap Kickstart ROM, if provided
          
          if( pd->m_ROMShadowBuffer != NULL )
          {
            activate = 
            SetProperties( pd->m_UserContext,
                           ( pd->m_Properties.m_UserROM | 
                             MAPP_ROM ),
                           ~0UL,
                           (ULONG) pd->m_ROMShadowBuffer, 
                           pd->m_ROMSize,
                           TAG_DONE ) &&
            SetProperties( pd->m_SuperContext,
                           ( pd->m_Properties.m_SuperROM | 
                             MAPP_ROM ),
                             ~0UL,
                             (ULONG) pd->m_ROMShadowBuffer,
                             pd->m_ROMSize,
                             TAG_DONE ) &&
            SetProperties( pd->m_UserContext,
                           ( pd->m_Properties.m_UserROM |
                             MAPP_ROM | MAPP_REMAPPED ),
                             ~0UL,
                             (ULONG) pd->m_ROM,
                             pd->m_ROMSize,
                             MAPTAG_DESTINATION, (ULONG) pd->m_ROMShadowBuffer,
                             TAG_DONE ) &&
            SetProperties( pd->m_SuperContext,
                           ( pd->m_Properties.m_SuperROM |
                             MAPP_ROM | MAPP_REMAPPED ),
                             ~0UL,
                             (ULONG) pd->m_ROM,
                             pd->m_ROMSize,
                             MAPTAG_DESTINATION, (ULONG) pd->m_ROMShadowBuffer,
                             TAG_DONE );
          }
          
          if( activate )
          {
            struct MMUContext* ctxs[ 3 ] = 
            {
              pd->m_UserContext,
              pd->m_SuperContext,
              NULL
            };

            // We need to disable, since we may have patched exec.library!

            Disable();
            rc = RebuildTreesA( ctxs );
            Enable();

            if( ! rc )
            {
              printf( "Failed to rebuild MMU trees.\n" );
            }
          }
        }
      }

      if( ! rc )
      {
        // Roll-back!

        SetPropertyList( pd->m_UserContext, user_mapping );
        SetPropertyList( pd->m_SuperContext, super_mapping );
      }
    }

    if( user_mapping != NULL )
    {
      ReleaseMapping( pd->m_UserContext, user_mapping );
    }

    if( super_mapping != NULL )
    {
      ReleaseMapping( pd->m_SuperContext, super_mapping );
    }

    UnlockMMUContext( pd->m_SuperContext );
    UnlockMMUContext( pd->m_UserContext );
    UnlockContextList();
  }

  return rc;
}


/******************************************************************************
** Restore MMU ****************************************************************
******************************************************************************/

static BOOL
RestoreMemory( struct PUHData* pd )
{
  BOOL            rc = FALSE;
  struct MinList* user_mapping;
  struct MinList* super_mapping;

  LockContextList();
  LockMMUContext( pd->m_UserContext );
  LockMMUContext( pd->m_SuperContext );

  // Save current mapping if we have to abort!

  user_mapping  = GetMapping( pd->m_UserContext );
  super_mapping = GetMapping( pd->m_SuperContext );

  if( user_mapping == NULL || super_mapping == NULL )
  {
    printf( "Failed to get mappings.\n" );
  }
  else
  {
    if( ! SetProperties( pd->m_UserContext,
                         pd->m_Properties.m_UserCustomShadow, ~0UL,
                         (ULONG) pd->m_CustomShadowBuffer, 
                         pd->m_CustomSize,
                         TAG_DONE ) ||
        ! SetProperties( pd->m_SuperContext,
                         pd->m_Properties.m_SuperCustomShadow, ~0UL,
                         (ULONG) pd->m_CustomShadowBuffer,
                         pd->m_CustomSize,
                         TAG_DONE ) )
    {
      printf( "Failed to set properties for for re-mapped area.\n" );
    }
    else
    {
      if( ! SetProperties( pd->m_UserContext,
                           pd->m_Properties.m_UserCustom, ~0UL,
                           0xdff000, pd->m_CustomSize,
                           TAG_DONE ) ||
          ! SetProperties( pd->m_SuperContext,
                           pd->m_Properties.m_SuperCustom, ~0UL,
                           0xdff000, pd->m_CustomSize,
                           TAG_DONE ) )
      {
        printf( "Failed to set properties for custom chip register area.\n" );
      }
      else
      {
        BOOL activate = TRUE;
        
        if( pd->m_ROMShadowBuffer != NULL )
        {
          if( ! SetProperties( pd->m_UserContext,
                               pd->m_Properties.m_UserROMShadow, ~0UL,
                               (ULONG) pd->m_ROMShadowBuffer, 
                               pd->m_ROMSize,
                               TAG_DONE ) ||
              ! SetProperties( pd->m_SuperContext,
                               pd->m_Properties.m_SuperROMShadow, ~0UL,
                               (ULONG) pd->m_ROMShadowBuffer,
                               pd->m_ROMSize,
                               TAG_DONE ) )
          {
            printf( "Failed to set properties for for patched ROM area.\n" );
            activate = FALSE;
          }
          else
          {
            if( ! SetProperties( pd->m_UserContext,
                                 pd->m_Properties.m_UserROM, ~0UL,
                                 (ULONG) pd->m_ROM, 
                                 pd->m_ROMSize,
                                 TAG_DONE ) ||
                ! SetProperties( pd->m_SuperContext,
                                 pd->m_Properties.m_SuperROM, ~0UL,
                                 (ULONG) pd->m_ROM,
                                 pd->m_ROMSize,
                                 TAG_DONE ) )
            {
              printf( "Failed to set properties for for ROM area.\n" );
              activate = FALSE;
            }
          }
        }

        if( activate )
        {
          struct MMUContext* ctxs[ 3 ] = 
          {
            pd->m_UserContext,
            pd->m_SuperContext,
            NULL
          };

          // We need to disable, since we may have patched exec.library!

          Disable();
          rc = RebuildTreesA( ctxs );
          Enable();

          if( ! rc )
          {
            printf( "Failed to rebuild MMU trees.\n" );
          }
        }
      }
    }

    if( ! rc )
    {
      // Roll-back!

      SetPropertyList( pd->m_UserContext, user_mapping );
      SetPropertyList( pd->m_SuperContext, super_mapping );
    }
  }

  if( user_mapping != NULL )
  {
    ReleaseMapping( pd->m_UserContext, user_mapping );
  }

  if( super_mapping != NULL )
  {
    ReleaseMapping( pd->m_SuperContext, super_mapping );
  }

  UnlockMMUContext( pd->m_SuperContext );
  UnlockMMUContext( pd->m_UserContext );
  UnlockContextList();
  
  return rc;
}


/******************************************************************************
** Patch kickstart ROM ********************************************************
******************************************************************************/

static void
PatchROMShadowBuffer( struct PUHData* pd )
{
  union
  {
    void*            m_Void;
    WORD*            m_Word;
    LONG*            m_Long;
    struct Resident* m_Resident;
  } ptr;

  void* end;

  ptr.m_Void = pd->m_ROMShadowBuffer;
  end        = (void*) ( (ULONG) pd->m_ROMShadowBuffer + pd->m_ROMSize );

  while( ptr.m_Void < end )
  {
    // Check for new module

    if( *ptr.m_Word == RTC_MATCHWORD )
    {
      if( ptr.m_Resident->rt_MatchTag == 
          pd->m_ROM + ( ptr.m_Void - pd->m_ROMShadowBuffer ) )
      {
        printf( "Found module %s at $%08lx-$%08lx\n", 
                ptr.m_Resident->rt_Name,
                (ULONG) ptr.m_Void, 
                (ULONG) pd->m_ROMShadowBuffer + ( ptr.m_Resident->rt_EndSkip - pd->m_ROM ) );
      }
 
      if( strcmp( ptr.m_Resident->rt_Name, "audio.device" ) == 0 )
      {
        printf( "Skipping %s...\n", ptr.m_Resident->rt_Name );

        ptr.m_Void += ( ptr.m_Resident->rt_EndSkip - pd->m_ROM );
      }
    }

    if( ( *ptr.m_Long & ~0xfff ) == 0xdff000 )
    {
      *ptr.m_Long = pd->m_Custom + ( *ptr.m_Long & 0xfff );

      printf( "Patched chip access at $%08lx (%03lx).\n",
              (ULONG) ptr.m_Void, *ptr.m_Long & 0xfff );
    }

    // Skip to next word

    ++ptr.m_Word;
  }
}


/******************************************************************************
** MMU exception handler ******************************************************
******************************************************************************/

ASMCALL INTERRUPT ULONG
PUHHandler( REG( a0, struct ExceptionData* ed ),
            REG( a1, struct PUHData*       pd ),
            REG( a6, struct Library*       MMUBase ) )
{
  struct ExecBase* SysBase;
  int              rc = 1;

  SysBase = ed->exd_SysBase;

  if( ( (ULONG) ed->exd_FaultAddress & ~0x1ffUL ) != (ULONG) pd->m_Intercepted )
  {
    // Not for us!
    return 1;
  }

  // Remove write-protection for us.

  if( ! SetPageProperties( pd->m_SuperContext,
                           MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                           MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                           (ULONG) pd->m_Intercepted, TAG_DONE ) )
  {
    // What to do?
    return 1;
  }

  if( ed->exd_Flags & ( EXDF_INSTRUCTION | 
                        EXDF_WRITEDATAUNKNOWN | 
                        EXDF_MISALIGNED ) )
  {
    KPrintF( "Illegal flag(s): %08lx, address $%08lx\n", 
             ed->exd_Flags, (ULONG) ed->exd_FaultAddress );
    rc = 1;
  }
  else
  { 
    int    size;
    ULONG  reg;

    size = (ULONG) ed->exd_NextFaultAddress - (ULONG) ed->exd_FaultAddress;
    reg  = (ULONG) ed->exd_FaultAddress - (ULONG) pd->m_Intercepted;

    if( ed->exd_Flags & EXDF_WRITE )
    {
      ed->exd_Flags |= EXDF_WRITECOMPLETE;

      if( size == 0 )
      {
        // Fake byte accesses, even though illegal

        if( reg & 1 )
        {
          PUHWrite( reg & ~1UL, ed->exd_Data & 0x00ffUL, pd, SysBase );
        }
        else
        {
          PUHWrite( reg, ( ed->exd_Data << 8 ) & 0xff00UL, pd, SysBase );
        }
      }
      else if( size == 1 )
      {
        PUHWrite( reg, ed->exd_Data & 0xffffUL, pd, SysBase );
      }
      else if( size == 3 )
      {
        PUHWrite( reg, ( ed->exd_Data >> 16 ) & 0xffffUL, pd, SysBase );
        PUHWrite( reg + 2, ed->exd_Data & 0xffffUL, pd, SysBase );
      }
      else
      {
        KPrintF( "Illegal size: %ld", size );
      }
    }
    else
    {
      ed->exd_Flags |= EXDF_READBACK;

      if( size == 0 )
      {
        // Fake byte accesses, even though illegal
        
        if( reg & 1 )
        {
          ed->exd_Data = PUHRead( reg & ~1UL, pd, SysBase ) & 0xffUL;
        }
        else
        {
          ed->exd_Data = PUHRead( reg, pd, SysBase ) >> 8;
        }
      }
      else if( size == 1 )
      {
        ed->exd_Data = PUHRead( reg, pd, SysBase );
      }
      else if( size == 3 )
      {
        ed->exd_Data =  PUHRead( reg, pd, SysBase ) << 16;
        ed->exd_Data |= PUHRead( reg + 2, pd, SysBase );
      }
      else
      {
        KPrintF( "Illegal size: %ld", size );
      }
    }

    // Ok, we handled it
    rc = 0;
  }

  // Restore write-protection for us

  SetPageProperties( pd->m_SuperContext,
                     MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                     MAPP_INVALID | MAPP_SINGLEPAGE | MAPP_REPAIRABLE,
                     (ULONG) pd->m_Intercepted, TAG_DONE );

  return rc;
}


/******************************************************************************
** Handle reads ***************************************************************
******************************************************************************/

UWORD
PUHRead( UWORD            reg, 
         struct PUHData*  pd,
         struct ExecBase* SysBase )
{
  UWORD  result;
  UWORD* address = (UWORD*) ( (ULONG) pd->m_Custom + reg );

  switch( reg )
  {
    case DMACONR:
    case INTENAR:
    case INTREQR:
    case ADKCONR:
      result = ReadWord( address );

//      KPrintF( "Read $%04lx from $%08lx.\n", result, (ULONG) pd->m_Intercepted + reg );
      break;

    default:
      // Just carry out the intercepted read operation

      result = ReadWord( address );

      break;
  }

  return result;
}


/******************************************************************************
** Handle writes **************************************************************
******************************************************************************/

void
PUHWrite( UWORD            reg, 
          UWORD            value,
          struct PUHData*  pd,
          struct ExecBase* SysBase )
{
  UWORD* address = (UWORD*) ( (ULONG) pd->m_Custom + reg );

  switch( reg )
  {
    case DMACON:
    case INTENA:
    case INTREQ:
    case ADKCON:
      WriteWord( address, value );

//      KPrintF( "Wrote $%04lx to $%08lx.\n", value, (ULONG) pd->m_Intercepted + reg );
      break;

    case AUD0LCH:
    case AUD0LCL:
    case AUD0LEN:
    case AUD0PER:
    case AUD0VOL:
    case AUD0DAT:

    case AUD1LCH:
    case AUD1LCL:
    case AUD1LEN:
    case AUD1PER:
    case AUD1VOL:
    case AUD1DAT:

    case AUD2LCH:
    case AUD2LCL:
    case AUD2LEN:
    case AUD2PER:
    case AUD2VOL:
    case AUD2DAT:

    case AUD3LCH:
    case AUD3LCL:
    case AUD3LEN:
    case AUD3PER:
    case AUD3VOL:
    case AUD3DAT:
      WriteWord( address, value );

//      KPrintF( "Wrote $%04lx to $%08lx.\n", value, (ULONG) pd->m_Intercepted + reg );
      break;

    default:
      // Just carry out the intercepted write operation

      WriteWord( address, value );

      break;
  }
}
