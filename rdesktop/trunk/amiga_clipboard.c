/*
  rdesktop: A Remote Desktop Protocol client.
  Clipboard handling - Amiga
  Copyright (C) Martin Blom 2004
   
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
   
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "rdesktop.h"

#undef BOOL
#define Bool int

#include <devices/clipboard.h>
#include <libraries/iffparse.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/iffparse.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include "amiga_clipboard.h"

#define ID_FTXT MAKE_ID('F', 'T', 'X', 'T')
#define ID_CHRS MAKE_ID('C', 'H', 'R', 'S')

ULONG amiga_clip_signals = 0;

static struct ClipboardHandle* amiga_clip_handle    = NULL;
static struct IFFHandle*       amiga_clip_iffhandle = NULL;
static LONG                    amiga_clip_announce  = 0;
static LONG                    amiga_clip_clipid    = 0;
static ULONG                   amiga_clip_format    = CF_TEXT;

static ULONG amiga_clip_hookfunc(struct Hook*        hook,
				 APTR                object,
				 struct ClipHookMsg* msg) {
  if (msg->chm_Type == 0) {
    // Ask main processing loop to call amiga_clip_handle_signals()
    amiga_clip_announce = msg->chm_ClipID;
    Signal((struct Task*) hook->h_Data,
	   1UL << amiga_clip_handle->cbh_SatisfyPort.mp_SigBit);
  }

  return 0;
}

static struct Hook amiga_clip_hook = {
  { NULL, NULL },
  HookEntry,
  amiga_clip_hookfunc,
  NULL
};


BOOL
amiga_clip_init(void) {
  amiga_clip_iffhandle = AllocIFF();

  if (amiga_clip_iffhandle != NULL) {
    amiga_clip_handle = OpenClipboard(PRIMARY_CLIP);

    if (amiga_clip_handle != NULL) {
      amiga_clip_iffhandle->iff_Stream = (ULONG) amiga_clip_handle;
      InitIFFasClip(amiga_clip_iffhandle);
      
      amiga_clip_signals = ((1UL << amiga_clip_handle->cbh_CBport.mp_SigBit) |
			    (1UL << amiga_clip_handle->cbh_SatisfyPort.mp_SigBit));
      
      // Add clipboard hook
      amiga_clip_hook.h_Data = FindTask(NULL);

      amiga_clip_handle->cbh_Req.io_Command = CBD_CHANGEHOOK;
      amiga_clip_handle->cbh_Req.io_Data    = (char*) &amiga_clip_hook;
      amiga_clip_handle->cbh_Req.io_Length  = 1;
      DoIO((struct IORequest*) &amiga_clip_handle->cbh_Req);

      return TRUE;
    }
  }

  return FALSE;
}

void
amiga_clip_deinit(void) {
  if (amiga_clip_iffhandle != NULL) {
    FreeIFF(amiga_clip_iffhandle);
  }

  if (amiga_clip_handle != NULL) {
    // Remove clipboard hook
    amiga_clip_hook.h_Data = NULL;

    amiga_clip_handle->cbh_Req.io_Command = CBD_CHANGEHOOK;
    amiga_clip_handle->cbh_Req.io_Data    = (char*) &amiga_clip_hook;
    amiga_clip_handle->cbh_Req.io_Length  = 0;
    DoIO((struct IORequest*) &amiga_clip_handle->cbh_Req);

    CloseClipboard(amiga_clip_handle);
    amiga_clip_handle  = NULL;
    amiga_clip_signals = 0;
  }
}


void
amiga_clip_handle_signals() {
  struct SatisfyMsg* sm;
  
  if (amiga_clip_announce != 0 && amiga_clip_announce != amiga_clip_clipid) {
    amiga_clip_announce = 0;
    ui_clip_sync();
  }

  sm = (struct SatisfyMsg*) GetMsg(&amiga_clip_handle->cbh_SatisfyPort);
  
  if (sm != NULL) {
    cliprdr_send_data_request(amiga_clip_format);
  }
}


void
ui_clip_format_announce(uint8 * data, uint32 length)
{
  amiga_clip_handle->cbh_Req.io_Command = CBD_POST;
  amiga_clip_handle->cbh_Req.io_Data    = (char*) &amiga_clip_handle->cbh_SatisfyPort;
  amiga_clip_handle->cbh_Req.io_ClipID  = 0;

  if (DoIO((struct IORequest*) &amiga_clip_handle->cbh_Req) == 0) {
    // TODO: Check format
    amiga_clip_format = CF_TEXT;
    amiga_clip_clipid = amiga_clip_handle->cbh_Req.io_ClipID;
  }
  else {
    amiga_clip_clipid = 0;
  }
}

void ui_clip_handle_data(uint8 * data, uint32 length)
{
  if (amiga_clip_format == CF_TEXT) {
    if (OpenIFF(amiga_clip_iffhandle, IFFF_WRITE) == 0) {
      if (PushChunk(amiga_clip_iffhandle, ID_FTXT, ID_FORM,
		    IFFSIZE_UNKNOWN) == 0) {
	if (PushChunk(amiga_clip_iffhandle, 0, ID_CHRS,
		      IFFSIZE_UNKNOWN) == 0) {
	  int len;

	  for (len = 0; len <= length && data[len] != 0; ++len);

	  if (WriteChunkBytes(amiga_clip_iffhandle, data, len) == len) {
	    // Success
	  }

	  PopChunk(amiga_clip_iffhandle);
	}

	PopChunk(amiga_clip_iffhandle);
      }
      
      CloseIFF(amiga_clip_iffhandle);
    }
  }
}

void ui_clip_request_data(uint32 format)
{
  if (format == CF_TEXT) {
    if (OpenIFF(amiga_clip_iffhandle, IFFF_READ) == 0) {
      if (StopChunk(amiga_clip_iffhandle, ID_FTXT, ID_CHRS) == 0) {
	while (TRUE) {
	  struct ContextNode* cn;

	  CollectionChunk!!!
	  
	  LONG error = ParseIFF(amiga_clip_iffhandle);
	  
	  if (error == IFFERR_EOC) {
	    continue;
	  }
	  else if (error != 0) {
	    break;
	  }

	  cn = CurrentChunk(amiga_clip_iffhandle);

	  if (cn != NULL && cn->cn_Type == ID_FTXT && cn->cn_ID == ID_CHRS) {
	    
	  }
      }
      
      CloseIFF(amiga_clip_iffhandle);
    }
    
    cliprdr_send_data("Banan", 6);
  }
}

void ui_clip_sync(void)
{
  // TODO: Check format of all clipboard.device data here
  cliprdr_send_text_format_announce();
}
