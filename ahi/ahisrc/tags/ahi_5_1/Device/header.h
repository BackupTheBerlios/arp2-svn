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

#ifndef ahi_header_h
#define ahi_header_h

#include <config.h>

#include <exec/types.h>
#include "addroutines.h"

struct AHIBase;

enum MixBackend_t
{
  MB_NATIVE
#if defined( ENABLE_WARPUP )
  ,MB_WARPUP
#endif
};


extern const ULONG		DriverVersion;
extern const ULONG		Version;
extern const ULONG		Revision;
extern const char		DevName[];
extern const char		IDString[];

extern enum MixBackend_t	MixBackend;

#if defined( ENABLE_WARPUP )
extern void*			PPCObject;
#endif

extern ADDFUNC*                 AddByteMonoPtr;
extern ADDFUNC*                 AddByteStereoPtr;
extern ADDFUNC*                 AddBytesMonoPtr;
extern ADDFUNC*                 AddBytesStereoPtr;
extern ADDFUNC*                 AddWordMonoPtr;
extern ADDFUNC*                 AddWordStereoPtr;
extern ADDFUNC*                 AddWordsMonoPtr;
extern ADDFUNC*                 AddWordsStereoPtr;
extern ADDFUNC*                 AddByteMonoBPtr;
extern ADDFUNC*                 AddByteStereoBPtr;
extern ADDFUNC*                 AddBytesMonoBPtr;
extern ADDFUNC*                 AddBytesStereoBPtr;
extern ADDFUNC*                 AddWordMonoBPtr;
extern ADDFUNC*                 AddWordStereoBPtr;
extern ADDFUNC*                 AddWordsMonoBPtr;
extern ADDFUNC*                 AddWordsStereoBPtr;

extern ADDFUNC*                 AddLofiByteMonoPtr;
extern ADDFUNC*                 AddLofiByteStereoPtr;
extern ADDFUNC*                 AddLofiBytesMonoPtr;
extern ADDFUNC*                 AddLofiBytesStereoPtr;
extern ADDFUNC*                 AddLofiWordMonoPtr;
extern ADDFUNC*                 AddLofiWordStereoPtr;
extern ADDFUNC*                 AddLofiWordsMonoPtr;
extern ADDFUNC*                 AddLofiWordsStereoPtr;
extern ADDFUNC*                 AddLofiByteMonoBPtr;
extern ADDFUNC*                 AddLofiByteStereoBPtr;
extern ADDFUNC*                 AddLofiBytesMonoBPtr;
extern ADDFUNC*                 AddLofiBytesStereoBPtr;
extern ADDFUNC*                 AddLofiWordMonoBPtr;
extern ADDFUNC*                 AddLofiWordStereoBPtr;
extern ADDFUNC*                 AddLofiWordsMonoBPtr;
extern ADDFUNC*                 AddLofiWordsStereoBPtr;

struct AHIBase*
initRoutine( struct AHIBase*  device,
             APTR             seglist,
             struct ExecBase* sysbase );

BPTR
DevExpunge( struct AHIBase* device );

#endif /* ahi_header_h */
