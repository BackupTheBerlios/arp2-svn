/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2004 Martin Blom <martin@blom.org>
     
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

#ifndef ahi_dspecho_h
#define ahi_dspecho_h

#include <config.h>

#include "ahi_def.h"
#include "dsp.h"

void
do_DSPEchoMono16( struct Echo *es,
                  void *buf,
                  struct AHIPrivAudioCtrl *audioctrl );

void
do_DSPEchoStereo16( struct Echo *es,
                    void *buf,
                    struct AHIPrivAudioCtrl *audioctrl );

void
do_DSPEchoMono32( struct Echo *es,
                  void *buf,
                  struct AHIPrivAudioCtrl *audioctrl );

void
do_DSPEchoStereo32( struct Echo *es,
                    void *buf,
                    struct AHIPrivAudioCtrl *audioctrl );

void
do_DSPEchoMulti32( struct Echo *es,
		   void *buf,
		   struct AHIPrivAudioCtrl *audioctrl );

#endif /* ahi_dspecho_h */
