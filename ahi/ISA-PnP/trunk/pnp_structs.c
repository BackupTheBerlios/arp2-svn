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

#include <clib/alib_protos.h>
#include <proto/exec.h>

#include "include/resources/isapnp.h"
#include "isapnp_private.h"

#include "init.h"
#include "pnp_structs.h"

/******************************************************************************
** Allocate a card structure **************************************************
******************************************************************************/

// You should set m_Node.ln_Name. Allocate the string with AllocVec()!

struct ISAPNP_Card* ASMCALL
PNPISA_AllocCard( REG( a6, struct ISAPNPBase* res ) )
{
  struct ISAPNP_Card* card;

  card = AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

  card->m_Node.ln_Type = ISAPNP_NT_CARD;

  NewList( &card->m_Devices );

  return card;
}

/******************************************************************************
** Deallocate a card structure ************************************************
******************************************************************************/

void ASMCALL
PNPISA_FreeCard( REG( a0, struct ISAPNP_Card* card ),
                 REG( a6, struct ISAPNPBase*  res ) )
{
  struct ISAPNP_Device* dev;

  if( card == NULL )
  {
    return;
  }

  KPrintF( "Nuking card %s%03lx%lx ('%s')\n",
           card->m_ID.m_Vendor, card->m_ID.m_ProductID, card->m_ID.m_Revision,
           card->m_Node.ln_Name != NULL ? card->m_Node.ln_Name : "" );

  while( ( dev = (struct ISAPNP_Device*) RemHead( &card->m_Devices ) ) )
  {
    PNPISA_FreeDevice( dev, res );
  }

  if( card->m_Node.ln_Name != NULL )
  {
    FreeVec( card->m_Node.ln_Name );
  }

  FreeVec( card );
}


/******************************************************************************
** Allocate a device structure ************************************************
******************************************************************************/

// You should set m_Node.ln_Name. Allocate the string with AllocVec()!

struct ISAPNP_Device* ASMCALL
PNPISA_AllocDevice( REG( a6, struct ISAPNPBase* res ) )
{
  struct ISAPNP_Device* dev;

  dev = AllocVec( sizeof( *dev ), MEMF_PUBLIC | MEMF_CLEAR );

  dev->m_Node.ln_Type = ISAPNP_NT_DEVICE;

  NewList( (struct List*) &dev->m_IDs );

  return dev;
}


/******************************************************************************
** Deallocate a device structure **********************************************
******************************************************************************/

void ASMCALL
PNPISA_FreeDevice( REG( a0, struct ISAPNP_Device* dev ),
                   REG( a6, struct ISAPNPBase*    res ) )
{
  struct ISAPNP_Identifier* id;

  if( dev == NULL )
  {
    return;
  }

  KPrintF( "Nuking logical device '%s'\n",
           dev->m_Node.ln_Name != NULL ? dev->m_Node.ln_Name : "" );


  while( ( id = (struct ISAPNP_Identifier*) 
             RemHead( (struct List*) &dev->m_IDs ) ) )
  {
    KPrintF( "Nuking (compatible) device %s%03lx%lx\n",
             id->m_Vendor, id->m_ProductID, id->m_Revision );

    FreeVec( id );
  }

  if( dev->m_Node.ln_Name != NULL )
  {
    FreeVec( dev->m_Node.ln_Name );
  }

  FreeVec( dev );
}
