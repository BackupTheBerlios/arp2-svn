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

#ifndef	ISA_PNP_pnp_iterators_h
#define ISA_PNP_pnp_iterators_h

#include "CompilerSpecific.h"

#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/types.h>


struct ISAPNP_Resource;
struct ISAPNPBase;


struct ResourceIterator
{
  struct MinNode          m_MinNode;
  struct ISAPNP_Resource* m_Resource;
  UBYTE                   m_IRQBit;
  UBYTE                   m_ChannelBit;
  UWORD                   m_Base;
};


struct ResourceIteratorList
{
  struct MinList m_ResourceIterators;
};


struct ResourceIterator*
AllocResourceIterator( struct ISAPNP_Resource* resource );

void
FreeResourceIterator( struct ResourceIterator* iter );


struct ResourceIteratorList*
AllocResourceIteratorList( struct MinList* resource_list );

void
FreeResourceIteratorList( struct ResourceIteratorList* list );


BOOL
IncResourceIterator( struct ResourceIterator* iter );

BOOL
IncResourceIteratorList( struct ResourceIteratorList* iter_list );


struct ISAPNP_Resource*
CreateResource( struct ResourceIterator* iter,
                struct ISAPNPBase*       res );

#endif /* ISA_PNP_pnp_iterators_h */
