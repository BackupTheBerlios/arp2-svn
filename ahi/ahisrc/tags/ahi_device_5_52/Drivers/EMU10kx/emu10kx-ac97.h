/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2004 Martin Blom <martin@blom.org>

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

#ifndef AHI_Drivers_EMU10kx_emu10kx_ac97_h
#define AHI_Drivers_EMU10kx_emu10kx_ac97_h

#include <config.h>
#include <exec/semaphores.h>
#include <utility/hooks.h>

#define EMU10KX_AC97_SEMAPHORE "emu10kx.ac97"

/* This is the global, public data structure used to communicate with
 * the AC97 part of emu10kx.audio. Find it, lock it, use it and then
 * release it. Beware of the license of this file and the driver!! */
   
struct EMU10kxAC97
{
    struct SignalSemaphore  Semaphore;
    UWORD                   Cards;
    UWORD                   Version;
    UWORD                   Revision;
    struct Hook             GetFunc;
    struct Hook             SetFunc;
};


/* Message for GetFunc */
struct AC97GetMessage
{
    ULONG         CardNum;
    ULONG         Register;
};

/* Message for SetFunc */
struct AC97SetMessage
{
    ULONG         CardNum;
    ULONG         Register;
    ULONG         Value;
};

struct EMU10kxBase;

ULONG
AC97GetFunc( struct Hook*           hook,
	     struct EMU10kxBase*    EMU10kxBase,
	     struct AC97GetMessage* msg );

VOID
AC97SetFunc( struct Hook*           hook,
	     struct EMU10kxBase*    EMU10kxBase,
	     struct AC97SetMessage* msg );

#endif /* AHI_Drivers_EMU10kx_emu10kx_ac97_h */
