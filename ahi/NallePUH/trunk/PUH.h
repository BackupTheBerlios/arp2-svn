/* $Id$ */

/*
     NallePUH -- Paula utan henne -- A minimal Paula emulator.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
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

#ifndef NallePUH_PUH_h
#define NallePUH_PUH_h

#include "CompilerSpecific.h"

struct ExceptionData;
struct Library;
struct MMUContext;
struct ExecBase;

/* Debugging */

void
KPrintFArgs( UBYTE* fmt, 
             ULONG* args );


#define KPrintF( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  KPrintFArgs( (fmt), _args );     \
})


/* Make nice accesses to hardware registers */

UWORD
ReadWord( void* address );


void
WriteWord( void* address, UWORD value );


ULONG
ReadLong( void* address );


void
WriteLong( void* address, ULONG value );


/* The emulator */

struct PUHData
{
  void*                 m_Intercepted;
  volatile void*        m_Custom;

  void*                 m_ShadowBuffer;
  ULONG	                m_ShadowSize;

  BOOL                  m_Active;

  struct MMUContext*    m_UserContext;
  struct MMUContext*    m_SuperContext;
  
  struct ExceptionHook* m_UserException;
  struct ExceptionHook* m_SuperException;
  
  ULONG                 m_UserRAMProperties;
  ULONG                 m_UserCustomProperties;
  ULONG                 m_SuperRAMProperties;
  ULONG                 m_SuperCustomProperties;
};


struct PUHData*
AllocPUH( void );


void
FreePUH( struct PUHData* pd );


BOOL
ActivatePUH( struct PUHData* pd );


void
DeactivatePUH( struct PUHData* pd );


#endif /* NallePUH_PUH_h */
