/* $Id$ */

#include <proto/exec.h>

#include "isapnp.h"
#include "isapnp_private.h"

#include <stdio.h>
#include <stdlib.h>

extern struct Resident RomTag;

int 
main( int argc, const char* argv[] )
{
  struct ISAPnPResource* ISAPnPBase;

  ISAPnPBase = (struct ISAPnPResource* ) OpenResource( ISAPNPNAME );

  if( ISAPnPBase != NULL )
  {
    printf( "Located resource at $%08lx\n", ISAPnPBase );

    ISAPnPBase->m_CurrentBinding.cb_ConfigDev->cd_Flags  |= CDF_CONFIGME;
    ISAPnPBase->m_CurrentBinding.cb_ConfigDev->cd_Driver  = NULL;
    RemResource( ISAPnPBase );
  }

  return 0;
};
