/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2000 Martin Blom <martin@blom.org>
     
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

#ifndef ahi_devsupp_h
#define ahi_devsupp_h

#include <config.h>
#include <CompilerSpecific.h>

#include <devices/ahi.h>

#ifdef mc68000

#define RecArgs REG(d0, ULONG size),    \
                REG(d1, ULONG add),     \
                REG(a0, APTR src),      \
                REG(a2, ULONG *offset), \
                REG(a3, void **dest)

void ASMCALL  RecM8S( RecArgs );
void ASMCALL  RecS8S( RecArgs );
void ASMCALL RecM16S( RecArgs );
void ASMCALL RecS16S( RecArgs );
void ASMCALL RecM32S( RecArgs );
void ASMCALL RecS32S( RecArgs );

#endif

#endif /* ahi_devsupp_h */
