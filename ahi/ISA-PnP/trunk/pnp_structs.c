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

  if( card != NULL )
  {
    card->m_Node.ln_Type = ISAPNP_NT_CARD;

    NewList( &card->m_Devices );
  }

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

//  KPrintF( "Nuking card %s%03lx%lx ('%s')\n",
//           card->m_ID.m_Vendor, card->m_ID.m_ProductID, card->m_ID.m_Revision,
//           card->m_Node.ln_Name != NULL ? card->m_Node.ln_Name : "" );

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

  if( dev != NULL )
  {
    dev->m_Node.ln_Type = ISAPNP_NT_DEVICE;

    NewList( (struct List*) &dev->m_IDs );
    
    dev->m_Options = PNPISA_AllocResourceGroup( ISAPNP_RG_PRI_GOOD, res );
    
    if( dev->m_Options == NULL )
    {
      PNPISA_FreeDevice( dev, res );
      dev = NULL;
    }
    
    NewList( (struct List*) &dev->m_Resources );
  }

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
  struct ISAPNP_Resource*   r;

  if( dev == NULL )
  {
    return;
  }

//  KPrintF( "Nuking logical device '%s'\n",
//           dev->m_Node.ln_Name != NULL ? dev->m_Node.ln_Name : "" );


  while( ( id = (struct ISAPNP_Identifier*) 
             RemHead( (struct List*) &dev->m_IDs ) ) )
  {
//    KPrintF( "Nuking (compatible) device %s%03lx%lx\n",
//             id->m_Vendor, id->m_ProductID, id->m_Revision );

    FreeVec( id );
  }

  PNPISA_FreeResourceGroup( dev->m_Options, res );

  while( ( r = (struct ISAPNP_Resource*) 
               RemHead( (struct List*) &dev->m_Resources ) ) )
  {
    PNPISA_FreeResource( r, res );
  }


  if( dev->m_Node.ln_Name != NULL )
  {
    FreeVec( dev->m_Node.ln_Name );
  }

  FreeVec( dev );
}


/******************************************************************************
** Allocate a resource group **************************************************
******************************************************************************/

struct ISAPNP_ResourceGroup* ASMCALL
PNPISA_AllocResourceGroup( REG( d0, UBYTE              pri ),
                           REG( a6, struct ISAPNPBase* res ) )
{
  struct ISAPNP_ResourceGroup* rg;

  rg = AllocVec( sizeof( *rg ), MEMF_PUBLIC | MEMF_CLEAR );

  if( rg != NULL )
  {
    rg->m_Type = ISAPNP_NT_RESOURCE_GROUP;
    rg->m_Pri  = pri;

    NewList( (struct List*) &rg->m_Resources );
    NewList( (struct List*) &rg->m_ResourceGroups );
  }

  return rg;
}


/******************************************************************************
** Deallocate a resource group ************************************************
******************************************************************************/

void ASMCALL
PNPISA_FreeResourceGroup( REG( a0, struct ISAPNP_ResourceGroup* rg ),
                          REG( a6, struct ISAPNPBase*           res ) )
{
  struct ISAPNP_ResourceGroup* child_rg;
  struct ISAPNP_Resource*      r;

  if( rg == NULL )
  {
    return;
  }

//  KPrintF( "Nuking resource group.\n" );

  while( ( r = (struct ISAPNP_Resource*) 
               RemHead( (struct List*) &rg->m_Resources ) ) )
  {
    PNPISA_FreeResource( r, res );
  }

  while( ( child_rg = (struct ISAPNP_ResourceGroup*) 
                      RemHead( (struct List*) &rg->m_ResourceGroups ) ) )
  {
    PNPISA_FreeResourceGroup( child_rg, res );
  }

  FreeVec( rg );
}


/******************************************************************************
** Allocate a resource ********************************************************
******************************************************************************/

struct ISAPNP_Resource* ASMCALL
PNPISA_AllocResource( REG( d0, UBYTE              type ),
                      REG( a6, struct ISAPNPBase* res ) )
{
  struct ISAPNP_Resource* r;
  ULONG                   size;

  switch( type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
      size = sizeof( struct ISAPNP_IRQResource );
      break;

    case ISAPNP_NT_DMA_RESOURCE:
      size = sizeof( struct ISAPNP_DMAResource );
      break;

    case ISAPNP_NT_IO_RESOURCE:
      size = sizeof( struct ISAPNP_IOResource );
      break;

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      return NULL;
  }

  r = AllocVec( size, MEMF_PUBLIC | MEMF_CLEAR );

  if( r != NULL )
  {
    r->m_Type = type;
  }

  return r;
}


/******************************************************************************
** Deallocate a resource ******************************************************
******************************************************************************/

void ASMCALL
PNPISA_FreeResource( REG( a0, struct ISAPNP_Resource* r ),
                     REG( a6, struct ISAPNPBase*      res ) )
{
  if( r == NULL )
  {
    return;
  }

//  KPrintF( "Nuking resource %ld.\n", r->m_Type );

  FreeVec( r );
}
