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

#include <proto/exec.h>

#include "include/resources/isapnp.h"
#include "pnp_iterators.h"
#include "pnp_structs.h"
#include "isapnp_private.h"


/******************************************************************************
** Reset a resource iterator **************************************************
******************************************************************************/

static BOOL
ResetResourceIterator( struct ResourceIterator* iter )
{
  switch( iter->m_Resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r;

      r = (struct ISAPNP_IRQResource*) iter->m_Resource;

      iter->m_IRQBit = 0;
      
      while( ( r->m_IRQMask & ( 1 << iter->m_IRQBit ) ) == 0 )
      {
        ++iter->m_IRQBit;
      }

      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r;

      r = (struct ISAPNP_DMAResource*) iter->m_Resource;

      iter->m_IRQBit = 0;
      
      while( ( r->m_ChannelMask & ( 1 << iter->m_ChannelBit ) ) == 0 )
      {
        ++iter->m_IRQBit;
      }

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
      iter->m_Base = ( (struct ISAPNP_IOResource*) 
                       iter->m_Resource )->m_MinBase;
      break;

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      return FALSE;
  }

  return TRUE;
}
                      

/******************************************************************************
** Allocate a resource iterator ***********************************************
******************************************************************************/

struct ResourceIterator*
AllocResourceIterator( struct ISAPNP_Resource* resource )
{
  struct ResourceIterator* iter;
  
  iter = AllocVec( sizeof( *iter ), MEMF_PUBLIC | MEMF_CLEAR );

  if( iter != NULL )
  {
    iter->m_Resource = resource;

    if( ! ResetResourceIterator( iter ) )
    {
      FreeResourceIterator( iter );
      iter = NULL;
    }
  }
  
  return iter;
}


/******************************************************************************
** Deallocate a resource iterator *********************************************
******************************************************************************/

void
FreeResourceIterator( struct ResourceIterator* iter )
{
  FreeVec( iter );
}


/******************************************************************************
** Allocate a resource iterator list ******************************************
******************************************************************************/

struct ResourceIteratorList*
AllocResourceIteratorList( struct MinList* resource_list )
{
  struct ResourceIteratorList* result;
  
  result = AllocVec( sizeof( *result ), MEMF_PUBLIC | MEMF_CLEAR );
  
  if( result != NULL )
  {
    struct ISAPNP_Resource* r;
    
    r = (struct ISAPNP_Resource*) resource_list->mlh_Head;
    
    while( r->m_MinNode.mln_Succ != NULL )
    {
      struct ResourceIterator* iter;
      
      iter = AllocResourceIterator( r );
      
      AddTail( (struct List*) &result->m_ResourceIterators,
               (struct Node*) iter );
      
      r = (struct ISAPNP_Resource*) r->m_MinNode.mln_Succ;
    }
  }
  
  return result;
}


/******************************************************************************
** Deallocate a resource iterator list ****************************************
******************************************************************************/

void
FreeResourceIteratorList( struct ResourceIteratorList* list )
{
  struct ResourceIterator* iter;

  if( list == NULL )
  {
    return;
  }

  while( ( iter = (struct ResourceIterator*) 
                  RemHead( (struct List*) &list->m_ResourceIterators ) ) )
  {  
    FreeResourceIterator( iter );
  }

  FreeVec( list );
}


/******************************************************************************
** Increase a resource iterator ***********************************************
******************************************************************************/

BOOL
IncResourceIterator( struct ResourceIterator* iter )
{
  BOOL rc = FALSE;

  switch( iter->m_Resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r;

      r = (struct ISAPNP_IRQResource*) iter->m_Resource;
      
      while( ! rc && iter->m_IRQBit < 16 )
      {
        ++iter->m_IRQBit;

        if( r->m_IRQMask & ( 1 << iter->m_IRQBit ) )
        {
          rc = TRUE;
        }
      }

      break;
    }

    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r;

      r = (struct ISAPNP_DMAResource*) iter->m_Resource;
      
      while( ! rc && iter->m_ChannelBit < 8 )
      {
        ++iter->m_ChannelBit;

        if( r->m_ChannelMask & ( 1 << iter->m_ChannelBit ) )
        {
          rc = TRUE;
        }
      }

      break;
    }

    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r;

      r = (struct ISAPNP_IOResource*) iter->m_Resource;
      
      iter->m_Base = r->m_MaxBase + r->m_Alignment;

      if( iter->m_Base <= r->m_MaxBase )
      {
        rc = TRUE;
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;;
  }
  
  return rc;
}


/******************************************************************************
** Increase a resource iterator list ******************************************
******************************************************************************/

BOOL
IncResourceIteratorList( struct ResourceIteratorList* iter_list )
{
  BOOL                     rc = FALSE;
  struct ResourceIterator* current;

  current = (struct ResourceIterator*) iter_list->m_ResourceIterators.mlh_Head;

  while( ! rc && current->m_MinNode.mln_Succ != NULL )
  {
    rc = IncResourceIterator( current );
    
    if( ! rc )
    {
      ResetResourceIterator( current );
      current = (struct ResourceIterator*) current->m_MinNode.mln_Succ;
    }
  }
  
  return rc;
}

/******************************************************************************
** Create a resource from an iterator *****************************************
******************************************************************************/

struct ISAPNP_Resource*
CreateResource( struct ResourceIterator* iter,
                struct ISAPNPBase*       res )
{
  struct ISAPNP_Resource* result = NULL;

  result = ISAPNP_AllocResource( iter->m_Resource->m_Type, res );

  if( result == NULL )
  {
    return NULL;
  }

  switch( iter->m_Resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r;

      r = (struct ISAPNP_IRQResource*) result;
      
      // Make a copy of the iterators resource

      CopyMem( iter->m_Resource, r, sizeof( *r ) );
      r->m_MinNode.mln_Succ = NULL;
      r->m_MinNode.mln_Pred = NULL;
      
      r->m_IRQMask = 1 << iter->m_IRQBit;

      break;
    }

    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r;

      r = (struct ISAPNP_DMAResource*) result;
      
      // Make a copy of the iterators resource

      CopyMem( iter->m_Resource, r, sizeof( *r ) );
      r->m_MinNode.mln_Succ = NULL;
      r->m_MinNode.mln_Pred = NULL;
      
      r->m_ChannelMask = 1 << iter->m_ChannelBit;

      break;
    }

    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r;

      r = (struct ISAPNP_IOResource*) result;
      
      // Make a copy of the iterators resource

      CopyMem( iter->m_Resource, r, sizeof( *r ) );
      r->m_MinNode.mln_Succ = NULL;
      r->m_MinNode.mln_Pred = NULL;

      r->m_MinBase   = iter->m_Base;
      r->m_MaxBase   = iter->m_Base;
      r->m_Alignment = 1;

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      ISAPNP_FreeResource( result, res );
      result = NULL;
      break;;
  }
  
  return result;
}
