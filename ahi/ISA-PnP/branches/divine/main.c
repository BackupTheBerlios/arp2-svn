/* $Id$ */

#include <proto/exec.h>

#include "isapnp.h"

#include <stdio.h>
#include <stdlib.h>

extern struct Resident RomTag;

int 
main( int argc, const char* argv[] )
{
  struct Library* ISAPnPBase;

  ISAPnPBase = OpenResource( ISAPNPNAME );

  if( ISAPnPBase != NULL )
  {
    printf( "Located resource at $%08lx\n", ISAPnPBase );

    RemResource( ISAPnPBase );
  }

  return 0;
};
