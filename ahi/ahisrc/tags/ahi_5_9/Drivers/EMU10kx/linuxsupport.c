/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2003 Martin Blom <martin@blom.org>
     
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

#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/openpci.h>

#include "linuxsupport.h"
#include <string.h>

static void*
AllocPages( size_t size, ULONG req )
{
  void* address;

  // FIXME: This shoule be non-cachable, DMA-able memory
  address = AllocMem( size + PAGE_SIZE - 1, req & ~MEMF_CLEAR );

  if( address != NULL )
  {
    Forbid();
    FreeMem( address, size + PAGE_SIZE - 1 );
    address = AllocAbs( size,
			(void*) ((ULONG) ( address + PAGE_SIZE - 1 )
				 & ~(PAGE_SIZE-1) ) );
    Permit();
  }

  if( address != NULL && ( req & MEMF_CLEAR ) )
  {
    memset( address, 0, size );
  }

  return address;
}

unsigned long
__get_free_page( unsigned int gfp_mask )
{
  return (unsigned long) AllocPages( PAGE_SIZE, MEMF_PUBLIC );
}

void
free_page( unsigned long addr )
{
//  printf( "Freeing page at %08x\n", addr );
  FreeMem( (void*) addr, PAGE_SIZE );
}

void*
pci_alloc_consistent( void* pci_dev, size_t size, dma_addr_t* dma_handle )
{
  void* res;

//  res = pci_alloc_dmamem( pci_dev, size );
  res = (void*) AllocPages( size, MEMF_PUBLIC );
  *dma_handle = (dma_addr_t) pci_logic_to_physic_addr( res, pci_dev );
  return res;
}

void
pci_free_consistent( void* pci_dev, size_t size, void* addr, dma_addr_t dma_handle )
{
//  printf( "Freeing pages (%d bytes) at %08x\n", size, addr );
  FreeMem( addr, size );
//  pci_free_dmamem( pci_dev, addr, size );
}
