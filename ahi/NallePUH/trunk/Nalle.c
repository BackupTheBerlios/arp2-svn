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
#include <exec/memory.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <hardware/custom.h>
#include <hardware/dmabits.h>

#include <stdio.h>

#include "PUH.h"

struct Library*  MMUBase = NULL;


int
RemapCustom( void*              location, 
             ULONG              size,
             ULONG*             user_ram_properties,
             ULONG*             user_custom_properties,
             ULONG*             super_ram_properties,
             ULONG*             super_custom_properties,
             struct MMUContext* uctx,
             struct MMUContext* sctx );


int
RestoreCustom( void*              location, 
               ULONG              size,
               ULONG              user_ram_properties,
               ULONG              user_custom_properties,
               ULONG              super_ram_properties,
               ULONG              super_custom_properties,
               struct MMUContext* uctx,
               struct MMUContext* sctx );



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
      if( ! ActivatePUH( pd ) )
      {
        rc = 20;
      }
      else
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
      
      FreePUH( pd );
    }
  }


  CloseLibrary( MMUBase );
  CloseLibrary( (struct Library*) DOSBase );

  return rc;
}



BYTE samples[] = 
{
  0, 90, 127, 90, 0, -90, -127, -90
};



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
