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


struct ResourceIteratorRef
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
** Helper functions for conflict handling *************************************
******************************************************************************/

static void
AddConflictIterator( struct ResourceIterator* iter,
                     struct MinList*          list )
{
  struct ResourceIteratorRef* node;

  if( list == NULL )
  {
    return;
  }

  node = AllocVec( sizeof( *node ), MEMF_PUBLIC );

  if( node != NULL )
  {
    node->m_Iterator = iter;
    AddTail( (struct List*) list, (struct Node*) node );
  }
}

static void
ClearConflictIteratorList( struct MinList* list )
{
  struct Node* n;

  if( list == NULL )
  {
    return;
  }

  while( ( n = RemHead( (struct List* ) list ) ) != NULL )
  {
    FreeVec( n );
  }  
}

static void
AddConflictIteratorList( struct MinList* src, 
                         struct MinList* dst )
{
  struct Node* n;

  if( src == NULL || dst== NULL )
  {
    return;
  }

  while( ( n = RemHead( (struct List* ) src ) ) != NULL )
  {
    AddTail( (struct List* ) dst, n );
  }  
}


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
  struct ResourceIteratorRef* io;

  if( ctx == NULL )
  {
    return;
  }

  while( ( io = (struct ResourceIteratorRef*) 
                RemHead( (struct List*) &ctx->m_IO ) ) )
  {
    FreeVec( io );
  }

  FreeVec( ctx );
}


/******************************************************************************
** Lock resources from the context ********************************************
******************************************************************************/

static struct ResourceIterator*
LockResource( struct ResourceIterator* iter,
              struct ResourceContext*  ctx )
{
  struct ResourceIterator* result = NULL;

  switch( iter->m_Resource->m_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
KPrintF( "L IRQ %ld", iter->m_IRQBit );
      if( ctx->m_IRQ[ iter->m_IRQBit ] == NULL )
      {
        ctx->m_IRQ[ iter->m_IRQBit ] = iter;
      }
      else
      {
        result = ctx->m_IRQ[ iter->m_IRQBit ];
      }
      
      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
KPrintF( "L DMA %ld", iter->m_ChannelBit );
      if( ctx->m_DMA[ iter->m_ChannelBit ] == NULL )
      {
        ctx->m_DMA[ iter->m_ChannelBit ] = iter;
      }
      else
      {
        result = ctx->m_DMA[ iter->m_ChannelBit ];
      }

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ResourceIteratorRef* io;
      struct ResourceIteratorRef* new_io;
      struct Node*                position = NULL;

      UWORD base   = iter->m_Base;
      UWORD length = iter->m_Length;

KPrintF( "L IO %lx-%lx", base, base + length );

//if( 0x220 == base )
{
  //KPrintF( "L %04lx: ", base );
}

      for( io = (struct ResourceIteratorRef*) ctx->m_IO.mlh_Head;
           io->m_MinNode.mln_Succ != NULL;
           io = (struct ResourceIteratorRef*) io->m_MinNode.mln_Succ )
      {
        UWORD io_base   = io->m_Iterator->m_Base;
        UWORD io_length = io->m_Iterator->m_Length;

//if( 0x220 == base )
{
  //KPrintF( "[T %04lx] ", io_base );
}

        if( ( base <= io_base && ( base + length ) > io_base )    ||
            ( base >= io_base && base < ( io_base + io_length ) ) )
        {
          // Collision!
//if( 0x220 == base )
{
  //KPrintF( ":(  " );
}
          result = io->m_Iterator;
          break;
        }
//if( 0x220 == base )
{
  //KPrintF( ":)  " );
}
        
        if( base + length <= io_base )
        {
//if( 0x220 == base )
{
  //KPrintF( "[A] " );
}
          // No more collisions possible; insert before this one
    
          position = (struct Node*) io->m_MinNode.mln_Pred;

          break;
        }
      }
//if( 0x220 == base )
{
  //KPrintF( "\n" );
}
      if( result == NULL )
      {
        // Insert the node
    
        new_io = AllocVec( sizeof( *new_io ), MEMF_PUBLIC );

        if( new_io == NULL )
        {
          result = (struct ResourceIterator*) -1;
        }
        else
        {
          new_io->m_Iterator = iter;

          if( position == NULL )
          {
            AddTail( (struct List*) &ctx->m_IO, (struct Node*) new_io );
          }
          else
          {
            Insert( (struct List*) &ctx->m_IO, (struct Node*) new_io, position );
          }
        }
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;
  }

  iter->m_HasLock = ( result == NULL );

  if( result == NULL ) KPrintF( " OK\n" ); else KPrintF( " failed\n" );

  return result;
}


/******************************************************************************
** Unlock resources from the context ******************************************
******************************************************************************/

static void
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
KPrintF( "U IRQ %ld\n", iter->m_IRQBit );
      ctx->m_IRQ[ iter->m_IRQBit ] = NULL;

      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
KPrintF( "U DMA %ld\n", iter->m_ChannelBit );
      ctx->m_DMA[ iter->m_ChannelBit ] = NULL;

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ResourceIteratorRef* io;

KPrintF( "U IO %lx-%lx\n", iter->m_Base, 
         iter->m_Base + iter->m_Length );
      for( io = (struct ResourceIteratorRef*) ctx->m_IO.mlh_Head;
           io->m_MinNode.mln_Succ != NULL;
           io = (struct ResourceIteratorRef*) io->m_MinNode.mln_Succ )
      {
        if( io->m_Iterator->m_Base == iter->m_Base )
        {
          Remove( (struct Node*) io );
          FreeVec( io );
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
                       struct MinList*          conflicts,
                       struct ResourceContext*  ctx )
{
  BOOL rc = FALSE;

  struct MinList potential_conflicts;

  NewList( (struct List*) &potential_conflicts );

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
          struct ResourceIterator* conflict;

          conflict = LockResource( iter, ctx );

          if( conflict == NULL )
          {
            rc = TRUE;
            break;
          }
          else
          {
            AddConflictIterator( conflict, &potential_conflicts );
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
          struct ResourceIterator* conflict;

          conflict = LockResource( iter, ctx );

          if( conflict == NULL )
          {
            rc = TRUE;
            break;
          }
          else
          {
            AddConflictIterator( conflict, &potential_conflicts );
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
        struct ResourceIterator* conflict;

        conflict = LockResource( iter, ctx );

        if( conflict == NULL )
        {
          rc = TRUE;
          break;
        }
        else
        {
          AddConflictIterator( conflict, &potential_conflicts );
        }

        iter->m_Base += r->m_Alignment;
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;
  }

  if( rc )
  {
    ClearConflictIteratorList( &potential_conflicts );
  }
  else
  {
    AddConflictIteratorList( &potential_conflicts, conflicts );
  }

  return rc;
}
                      

/******************************************************************************
** Allocate a resource iterator ***********************************************
******************************************************************************/

struct ResourceIterator*
AllocResourceIterator( struct ISAPNP_Resource* resource,
                       struct MinList*         conflicts,
                       struct ResourceContext* ctx )
{
  struct ResourceIterator* iter;
  
  iter = AllocVec( sizeof( *iter ), MEMF_PUBLIC | MEMF_CLEAR );

  if( iter != NULL )
  {
    iter->m_Resource = resource;

    if( ! ResetResourceIterator( iter, conflicts, ctx ) )
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

// This function cannot handle conflicts within the list itself

struct ResourceIteratorList*
AllocResourceIteratorList( struct MinList*         resource_list,
                           struct MinList*         conflicts,
                           struct ResourceContext* ctx )
{
  struct ResourceIteratorList* result;
  
KPrintF( "AllocResourceIteratorList()\n" );
  result = AllocVec( sizeof( *result ), MEMF_PUBLIC | MEMF_CLEAR );
  
  if( result != NULL )
  {
    struct ISAPNP_Resource* r;

    NewList( (struct List*) &result->m_ResourceIterators );
    
    r = (struct ISAPNP_Resource*) resource_list->mlh_Head;
    
    while( r->m_MinNode.mln_Succ != NULL )
    {
      struct ResourceIterator* iter;
      
      iter = AllocResourceIterator( r, conflicts, ctx );
      
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

if( result )
{
  KPrintF( "AllocResourceIteratorList() succeeded\n" );
}
else
{
  KPrintF( "AllocResourceIteratorList() failed\n" );
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

KPrintF( "FreeResourceIteratorList()\n" );
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
KPrintF( "FreeResourceIteratorList() OK\n" );
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
          rc = LockResource( iter, ctx ) == NULL;
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
          rc = LockResource( iter, ctx ) == NULL;
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
        iter->m_Base += r->m_Alignment;

        if( iter->m_Base <= r->m_MaxBase )
        {
          rc = LockResource( iter, ctx ) == NULL;
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

KPrintF( "IncResourceIteratorList()\n" );
  current = (struct ResourceIterator*) iter_list->m_ResourceIterators.mlh_Head;

  while( ! rc && current->m_MinNode.mln_Succ != NULL )
  {
    rc = IncResourceIterator( current, ctx );
    
    if( ! rc )
    {
      if( ! ResetResourceIterator( current, NULL, ctx ) )
      {
        // This should never happen, but better safe than sorry...
        break;
      }

      current = (struct ResourceIterator*) current->m_MinNode.mln_Succ;
    }
  }
  
KPrintF( "IncResourceIteratorList(): %ld\n", rc );
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


/******************************************************************************
** Create resources from a list of iterators **********************************
******************************************************************************/

BOOL
CreateResouces( struct ResourceIteratorList* ril,
                struct List*                 result,
                struct ISAPNPBase*           res )
{
  // Allocate resources for current iterators

  struct ResourceIterator* iter;

  for( iter = (struct ResourceIterator*) 
              ril->m_ResourceIterators.mlh_Head;
       iter->m_MinNode.mln_Succ != NULL;
       iter = (struct ResourceIterator*) 
              iter->m_MinNode.mln_Succ )
  {
    struct ISAPNP_Resource* resource;

    resource = CreateResource( iter, res );
    
    if( resource == NULL )
    {
      return FALSE;
    }
    
    AddTail( result, (struct Node*) resource );
  }

  return TRUE;
}
