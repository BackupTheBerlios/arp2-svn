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

#include "CompilerSpecific.h"

#include <dos/dos.h>
#include <devices/ahi.h>
#include <exec/errors.h>
#include <exec/memory.h>

#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <hardware/custom.h>
#include <hardware/dmabits.h>

#include <stdio.h>

#include "PUH.h"

static struct MsgPort*      AHImp     = NULL;
static struct AHIRequest*   AHIio     = NULL;
static BYTE                 AHIDevice = IOERR_OPENFAIL;

struct Library* AHIBase = NULL;
struct Library* MMUBase = NULL;


static BOOL 
OpenAHI( void );

static void 
CloseAHI( void );

void
Test( struct Custom* custom );


int
main( void )
{
  int rc = 0;

  DOSBase = (struct DosLibrary*) OpenLibrary( "dos.library", 37 );
  MMUBase = OpenLibrary( "mmu.library", 41 );

  if( DOSBase == NULL )
  {
    printf( "Unable to open dos.library version 37.\n" );
    rc = 20;
  }

  if( MMUBase == NULL )
  {
    printf( "Unable to open mmu.library version 41.\n" );
    rc = 20;
  }

  if( ! OpenAHI() )
  {
    printf( "Unable to open ahi.device version 4.\n" );
    rc = 20;
  }

  if( rc == 0 )
  {
    struct PUHData* pd;

    pd = AllocPUH();
    
    if( pd == NULL )
    {
      rc = 20;
    }
    else
    {
      if( ! InstallPUH( PUHF_PATCH_ROM, 
                        0x1f0001, 22050,
                        pd ) )
      {
        rc = 20;
      }
      else
      {
#if 1
        if( ! ActivatePUH( pd ) )
        {
          rc = 20;
        }
        else
#endif
        {
#ifdef TEST_MODE
          Test( (struct Custom*) location );
#else
          Test( (struct Custom*) 0xdff000 );
#endif

          printf( "Waiting for CTRL-C...\n" );
          Wait( SIGBREAKF_CTRL_C );
          printf( "Got it.\n" );
        
          DeactivatePUH( pd );
        }
        
        UninstallPUH( pd );
      }
      
      FreePUH( pd );
    }
  }


  CloseAHI();
  CloseLibrary( MMUBase );
  CloseLibrary( (struct Library*) DOSBase );

  return rc;
}



BYTE samples[] = 
{
  0, 90, 127, 90, 0, -90, -127, -90
};



/******************************************************************************
** Test ***********************************************************************
******************************************************************************/

void
Test( struct Custom* custom )
{
  void* chip;

  printf( "Testing with custom set to 0x%08lx\n", (ULONG) custom );

  chip = AllocVec( sizeof( samples ), MEMF_CHIP );

  CopyMem( samples, chip, sizeof( samples ) );

  WriteLong( (ULONG*) &custom->aud[ 0 ].ac_ptr, (ULONG) chip );
  WriteWord( (UWORD*) &custom->aud[ 0 ].ac_len, sizeof( samples ) / 2 );
  WriteWord( (UWORD*) &custom->aud[ 0 ].ac_per, 447 );
  WriteWord( (UWORD*) &custom->aud[ 0 ].ac_vol, 64 );

  WriteWord( (UWORD*) &custom->dmacon, DMAF_SETCLR | DMAF_AUD0 | DMAF_MASTER );

  Delay( 50 );

  WriteWord( (UWORD*) &custom->dmacon, DMAF_AUD0 );

  FreeVec( chip );
}


/******************************************************************************
** OpenAHI ********************************************************************
******************************************************************************/

/* Opens and initializes the device. */

static BOOL
OpenAHI( void )
{
  BOOL rc = FALSE;

  AHImp = CreateMsgPort();

  if( AHImp != NULL )
  {
    AHIio = (struct AHIRequest *) CreateIORequest( AHImp, 
                                                   sizeof( struct AHIRequest ) );

    if( AHIio != NULL ) 
    {
      AHIio->ahir_Version = 4;
      AHIDevice = OpenDevice( AHINAME,
                              AHI_NO_UNIT,
                              (struct IORequest *) AHIio,
                              NULL );
                              
      if( AHIDevice == 0 )
      {
        AHIBase = (struct Library *) AHIio->ahir_Std.io_Device;
        rc = TRUE;
      }
    }
  }

  return rc;
}


/******************************************************************************
** CloseAHI *******************************************************************
******************************************************************************/

/* Closes the device, cleans up. */

static void
CloseAHI( void )
{
  if( AHIDevice == 0 )
  {
    CloseDevice( (struct IORequest *) AHIio );
  }

  DeleteIORequest( (struct IORequest *) AHIio );
  DeleteMsgPort( AHImp );

  AHIBase   = NULL;
  AHImp     = NULL;
  AHIio     = NULL;
  AHIDevice = IOERR_OPENFAIL;
}
