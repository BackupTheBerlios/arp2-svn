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
#include "init.h"
#include "pnp_iterators.h"
#include "pnp_structs.h"
#include "isapnp_private.h"


struct ResourceIterator
{
  struct MinNode          m_MinNode;
  struct ISAPNP_Resource* m_Resource;
  UBYTE                   m_IRQBit;
  UBYTE                   m_ChannelBit;
  UWORD                   m_Base;
  UWORD                   m_Length;
  BOOL                    m_HasLock;
};


struct ResourceIteratorList
{
  struct MinList m_ResourceIterators;
};



struct ResourceIOArea
{
  struct MinNode           m_MinNode;
  struct ResourceIterator* m_Iterator;
};


struct ResourceContext
{
  struct ResourceIterator* m_IRQ[ 16 ];
  struct ResourceIterator* m_DMA[ 8 ];
  struct MinList           m_IO;
};



/******************************************************************************
** Allocate a clean resource iterator context *********************************
******************************************************************************/

struct ResourceContext*
AllocResourceIteratorContext( void )
{
  struct ResourceContext* ctx;
  
  ctx = AllocVec( sizeof( *ctx ), MEMF_PUBLIC | MEMF_CLEAR );
  
  if( ctx != NULL )
  {
    NewList( (struct List*) &ctx->m_IO );
  }
  
  return ctx;
}


/******************************************************************************
** Free a resource iterator context *******************************************
******************************************************************************/

void
FreeResourceIteratorContext( struct ResourceContext* ctx )
{
  struct ResourceIOArea* io;

  if( ctx == NULL )
  {
    return;
  }

  while( ( io = (struct ResourceIOArea*) RemHead( (struct List*) &ctx->m_IO ) ) )
  {
    FreeVec( io );
  }

  FreeVec( ctx );
}


/******************************************************************************
** Lock resources from the context ********************************************
******************************************************************************/

BOOL
LockResource( struct ResourceIterator* iter,
              struct ResourceContext*  ctx )
{
  BOOL rc = FALSE;

  switch( iter->m_Resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      if( ctx->m_IRQ[ iter->m_IRQBit ] == NULL )
      {
        ctx->m_IRQ[ iter->m_IRQBit ] = iter;
//        KPrintF( "Locked IRQ %ld\n", iter->m_IRQBit );
        rc = TRUE;
      }
      else
      {
//        KPrintF( "Failed to lock IRQ %ld\n", iter->m_IRQBit );
      }
      
      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
      if( ctx->m_DMA[ iter->m_ChannelBit ] == NULL )
      {
        ctx->m_DMA[ iter->m_ChannelBit ] = iter;
//        KPrintF( "Locked DMA %ld\n", iter->m_ChannelBit );
        rc = TRUE;
      }
      else
      {
//        KPrintF( "Failed to lock DMA %ld\n", iter->m_ChannelBit );
      }

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ResourceIOArea* io;
      struct ResourceIOArea* new_io;
      struct Node*           position = NULL;

      UWORD base   = iter->m_Base;
      UWORD length = iter->m_Length;

      rc = TRUE;

      for( io = (struct ResourceIOArea*) ctx->m_IO.mlh_Head;
           io->m_MinNode.mln_Succ != NULL;
           io = (struct ResourceIOArea*) io->m_MinNode.mln_Succ )
      {
        UWORD io_base   = io->m_Iterator->m_Base;
        UWORD io_length = io->m_Iterator->m_Length;

        if( ( base <= io_base && ( base + length ) > io_base )    ||
            ( base >= io_base && base < ( io_base + io_length ) ) )
        {
          // Collision!
          
//          KPrintF( "Failed to lock IO 0x%04lx, length %ld\n", base, length );
          rc = FALSE;
          break;
        }
        
        if( base >= io_base + io_length )
        {
          // No more collisions possible; insert before this one
    
          if( io->m_MinNode.mln_Pred->mln_Pred != NULL )
          {
            position = (struct Node*) io->m_MinNode.mln_Pred;
          }
    
          break;
        }
      }
    
      if( rc )
      {
        // Insert the node
    
        new_io = AllocVec( sizeof( *new_io ), MEMF_PUBLIC | MEMF_CLEAR );

        if( new_io == NULL )
        {
          rc = FALSE;
        }
        else
        {
          new_io->m_Iterator = iter;
    
          Insert( (struct List*) &ctx->m_IO, (struct Node*) new_io, position );
    
//          KPrintF( "Locked IO 0x%04lx, length %ld\n", base, length );
        }
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;
  }

  iter->m_HasLock = rc;

  return rc;
}


/******************************************************************************
** Unlock resources from the context ******************************************
******************************************************************************/

void
UnlockResource( struct ResourceIterator* iter,
                struct ResourceContext*  ctx )
{
  if( ! iter->m_HasLock )
  {
    return;
  }

  switch( iter->m_Resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
//      KPrintF( "Unlocked IRQ %ld\n", iter->m_IRQBit );
      ctx->m_IRQ[ iter->m_IRQBit ] = NULL;

      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
//      KPrintF( "Unlocked DMA %ld\n", iter->m_ChannelBit );
      ctx->m_DMA[ iter->m_ChannelBit ] = NULL;

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ResourceIOArea* io;

//      KPrintF( "Unlocked IO 0x%04lx, length %ld\n", iter->m_Base, iter->m_Length );

      for( io = (struct ResourceIOArea*) ctx->m_IO.mlh_Head;
           io->m_MinNode.mln_Succ != NULL;
           io = (struct ResourceIOArea*) io->m_MinNode.mln_Succ )
      {
        if( io->m_Iterator->m_Base == iter->m_Base )
        {
          Remove( (struct Node*) io );
          break;
        }
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;
  }

  iter->m_HasLock = FALSE;
}


/******************************************************************************
** Reset a resource iterator **************************************************
******************************************************************************/

static BOOL
ResetResourceIterator( struct ResourceIterator* iter,
                       struct ResourceContext* ctx )
{
  switch( iter->m_Resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r;

      r = (struct ISAPNP_IRQResource*) iter->m_Resource;

      iter->m_IRQBit = 0;
      
      while( iter->m_IRQBit < 16 )
      {
        if( r->m_IRQMask & ( 1 << iter->m_IRQBit ) )
        {
          if( LockResource( iter, ctx ) )
          {
            return TRUE;
          }
        }

        ++iter->m_IRQBit;
      }

      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r;

      r = (struct ISAPNP_DMAResource*) iter->m_Resource;

      iter->m_ChannelBit = 0;
      
      while( iter->m_ChannelBit < 8 )
      {
        if( r->m_ChannelMask & ( 1 << iter->m_ChannelBit ) )
        {
          if( LockResource( iter, ctx ) )
          {
            return TRUE;
          }
        }

        ++iter->m_ChannelBit;
      }

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r;
      
      r = (struct ISAPNP_IOResource*) iter->m_Resource;

      iter->m_Base   = r->m_MinBase;
      iter->m_Length = r->m_Length;


      while( iter->m_Base <= r->m_MaxBase )
      {
        if( LockResource( iter, ctx ) )
        {
          return TRUE;
        }

        iter->m_Base = r->m_MaxBase + r->m_Alignment;
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      return FALSE;
      break;
  }

  return FALSE;
}
                      

/******************************************************************************
** Allocate a resource iterator ***********************************************
******************************************************************************/

struct ResourceIterator*
AllocResourceIterator( struct ISAPNP_Resource* resource,
                       struct ResourceContext* ctx )
{
  struct ResourceIterator* iter;
  
  iter = AllocVec( sizeof( *iter ), MEMF_PUBLIC | MEMF_CLEAR );

  if( iter != NULL )
  {
    iter->m_Resource = resource;

    if( ! ResetResourceIterator( iter, ctx ) )
    {
      FreeResourceIterator( iter, ctx );
      iter = NULL;
    }
  }
  
  return iter;
}


/******************************************************************************
** Deallocate a resource iterator *********************************************
******************************************************************************/

void
FreeResourceIterator( struct ResourceIterator* iter,
                      struct ResourceContext* ctx )
{
  if( iter == NULL )
  {
    return;
  }

  UnlockResource( iter, ctx );

  FreeVec( iter );
}


/******************************************************************************
** Allocate a resource iterator list ******************************************
******************************************************************************/

struct ResourceIteratorList*
AllocResourceIteratorList( struct MinList*         resource_list,
                           struct ResourceContext* ctx )
{
  struct ResourceIteratorList* result;
  
  result = AllocVec( sizeof( *result ), MEMF_PUBLIC | MEMF_CLEAR );
  
  if( result != NULL )
  {
    struct ISAPNP_Resource* r;

    NewList( (struct List*) &result->m_ResourceIterators );
    
    r = (struct ISAPNP_Resource*) resource_list->mlh_Head;
    
    while( r->m_MinNode.mln_Succ != NULL )
    {
      struct ResourceIterator* iter;
      
      iter = AllocResourceIterator( r, ctx );
      
      if( iter == NULL )
      {
        FreeResourceIteratorList( result, ctx );
        result = NULL;
        break;
      }
      
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
FreeResourceIteratorList( struct ResourceIteratorList* list,
                          struct ResourceContext*      ctx )
{
  struct ResourceIterator* iter;

  if( list == NULL )
  {
    return;
  }

  while( ( iter = (struct ResourceIterator*) 
                  RemHead( (struct List*) &list->m_ResourceIterators ) ) )
  {  
    FreeResourceIterator( iter, ctx );
  }

  FreeVec( list );
}


/******************************************************************************
** Increase a resource iterator ***********************************************
******************************************************************************/

BOOL
IncResourceIterator( struct ResourceIterator* iter,
                     struct ResourceContext*  ctx )
{
  BOOL rc = FALSE;

  UnlockResource( iter, ctx );

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
          rc = LockResource( iter, ctx );
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
          rc = LockResource( iter, ctx );
        }
      }

      break;
    }

    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r;

      r = (struct ISAPNP_IOResource*) iter->m_Resource;
      
      while( ! rc && iter->m_Base <= r->m_MaxBase )
      {
        iter->m_Base = r->m_MaxBase + r->m_Alignment;

        if( iter->m_Base <= r->m_MaxBase )
        {
          rc = LockResource( iter, ctx );
        }
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
IncResourceIteratorList( struct ResourceIteratorList* iter_list,
                         struct ResourceContext*      ctx )
{
  BOOL                     rc = FALSE;
  struct ResourceIterator* current;

  current = (struct ResourceIterator*) iter_list->m_ResourceIterators.mlh_Head;

  while( ! rc && current->m_MinNode.mln_Succ != NULL )
  {
    rc = IncResourceIterator( current, ctx );
    
    if( ! rc )
    {
      if( ! ResetResourceIterator( current, ctx ) )
      {
        // This should never happen, but better safe than sorry...
        break;
      }

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
