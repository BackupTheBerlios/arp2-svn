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

#include <proto/exec.h>

#include "include/resources/isapnp.h"
#include "isapnp_private.h"

#include "init.h"

extern struct Resident RomTag;


/******************************************************************************
** First instruction of the binary ********************************************
******************************************************************************/

asm( "jmp _ResourceEntry" );


/******************************************************************************
** Resource entry *************************************************************
******************************************************************************/

int 
ResourceEntry( void )
{
  struct ISAPnPResource* ISAPnPBase;

  if( ! OpenLibs() )
  {
    return 20;
  }

  ISAPnPBase = (struct ISAPnPResource* ) OpenResource( ISAPNPNAME );

  if( ISAPnPBase != NULL )
  {
    KPrintF( "Located resource at $%08lx\n", ISAPnPBase );

    ISAPnPBase->m_CurrentBinding.cb_ConfigDev->cd_Flags  |= CDF_CONFIGME;
    ISAPnPBase->m_CurrentBinding.cb_ConfigDev->cd_Driver  = NULL;
    RemResource( ISAPnPBase );
  }

  CloseLibs();

  return 0;
};
