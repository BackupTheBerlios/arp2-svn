/*
  rdesktop: A Remote Desktop Protocol client.
  Clipboard handling - Amiga
  Copyright (C) Martin Blom 2004-2005
   
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

static BYTE                    amiga_clip_hooksignal = -1;
static struct ClipboardHandle* amiga_clip_handle     = NULL;
static struct IFFHandle*       amiga_clip_iffhandle  = NULL;
static LONG                    amiga_clip_announce   = 0;
static LONG                    amiga_clip_clipid     = 0;
static ULONG                   amiga_clip_format     = CF_TEXT;

static ULONG amiga_clip_hookfunc(struct Hook*        hook,
				 APTR                object,
				 struct ClipHookMsg* msg) {
  if (msg->chm_Type == 0) {
    // Ask main processing loop to call amiga_clip_handle_signals()
    amiga_clip_announce = msg->chm_ClipID;
    Signal((struct Task*) hook->h_Data, 1UL << amiga_clip_hooksignal);
  }

  return 0;
}

static struct Hook amiga_clip_hook = {
  { NULL, NULL },
  HookEntry,
  amiga_clip_hookfunc,
  NULL
};


static BOOL amiga_clip_active_post() {
  if (amiga_clip_clipid != 0) {
    // We may have an outstanding CBD_POST. Check it.

    amiga_clip_handle->cbh_Req.io_Command = CBD_CURRENTWRITEID;
    DoIO((struct IORequest*) &amiga_clip_handle->cbh_Req);
 
    if (amiga_clip_handle->cbh_Req.io_ClipID <= amiga_clip_clipid) {
      return TRUE;
    }
  }

  return FALSE;
}

BOOL
amiga_clip_init(void) {
  amiga_clip_hooksignal = AllocSignal(-1);
  
  if (amiga_clip_hooksignal == -1) {
    return FALSE;
  }

  amiga_clip_iffhandle = AllocIFF();

  if (amiga_clip_iffhandle == NULL) {
    return FALSE;
  }
  
  amiga_clip_handle = OpenClipboard(PRIMARY_CLIP);

  if (amiga_clip_handle == NULL) {
    return FALSE;
  }

  amiga_clip_iffhandle->iff_Stream = (ULONG) amiga_clip_handle;
  InitIFFasClip(amiga_clip_iffhandle);
      
  amiga_clip_signals = (//(1UL << amiga_clip_handle->cbh_CBport.mp_SigBit) |
			(1UL << amiga_clip_handle->cbh_SatisfyPort.mp_SigBit) |
			(1UL << amiga_clip_hooksignal));
      
  // Add clipboard hook
  amiga_clip_hook.h_Data = FindTask(NULL);
  
  amiga_clip_handle->cbh_Req.io_Command = CBD_CHANGEHOOK;
  amiga_clip_handle->cbh_Req.io_Data    = (char*) &amiga_clip_hook;
  amiga_clip_handle->cbh_Req.io_Length  = 1;
  DoIO((struct IORequest*) &amiga_clip_handle->cbh_Req);
  
  return TRUE;
}

BOOL
amiga_clip_shutdown(void) {
  if (amiga_clip_active_post()) {
    static BOOL once = TRUE;

    if (once) {
      cliprdr_send_data_request(amiga_clip_format);
      once = FALSE;
    }

    // Don't shutdown yet
    return FALSE;
  }
  else {
    // It's ok to shutdown
    return TRUE;
  }
}

void
amiga_clip_deinit(void) {
  if (amiga_clip_active_post()) {
    // We have an outstanding CBD_POST! This happens when we
    // disconnect using Windows and the Amiga side.  All we can do
    // is to write empty data to the clipboard. :-(
    ui_clip_handle_data("", 0);
  }
  
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

  FreeSignal(amiga_clip_hooksignal);
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
    // TODO: In rare cases, the Windows host will never reply to this
    // request. In that case, the Amiga clipboard will hang until some
    // other application pastes.
    cliprdr_send_data_request(amiga_clip_format);
  }
}


void
ui_clip_format_announce(uint8 * data, uint32 length)
{
  if ((length % 36) != 0) {
    // What's this???
    return;
  }

  while (length != 0) {
    uint32 format = ((data[3] << 24) |
		     (data[2] << 16) |
		     (data[1] <<  8) |
		     (data[0]));
    
    if (format == CF_TEXT) {
      amiga_clip_handle->cbh_Req.io_Command = CBD_POST;
      amiga_clip_handle->cbh_Req.io_Data    =
	(char*) &amiga_clip_handle->cbh_SatisfyPort;
      amiga_clip_handle->cbh_Req.io_ClipID  = 0;

      if (DoIO((struct IORequest*) &amiga_clip_handle->cbh_Req) == 0) {
	amiga_clip_format = format;
	amiga_clip_clipid = amiga_clip_handle->cbh_Req.io_ClipID;
      }
      else {
	amiga_clip_clipid = 0;
      }

      break;
    }
    
    length -= 36;
    data += 36;
  }
  
}

void ui_clip_handle_data(uint8 * data, uint32 length)
{
  if (amiga_clip_format == CF_TEXT) {
    if (OpenIFF(amiga_clip_iffhandle, IFFF_WRITE) == 0) {

      amiga_clip_handle->cbh_Req.io_ClipID = amiga_clip_clipid;
      amiga_clip_clipid = 0;
      
      if (PushChunk(amiga_clip_iffhandle, ID_FTXT, ID_FORM,
		    IFFSIZE_UNKNOWN) == 0) {
	if (PushChunk(amiga_clip_iffhandle, 0, ID_CHRS,
		      IFFSIZE_UNKNOWN) == 0) {
	  int len;
	  int i;

	  for (i = 0, len = 0; i < length && data[i] != 0; ++i) {
	    if (data[i] != '\r') {
	      data[len] = data[i];
	      ++len;
	    }
	  }

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

static char* extract_collection(struct CollectionItem* ci, char* str) {
  int   i;
  char* chrs = ci->ci_Data;

  if (ci->ci_Next != NULL) {
    str = extract_collection(ci->ci_Next, str);
  }
  
  for (i = 0; i < ci->ci_Size; ++i) {
    if (chrs[i] == '\n') {
      *str++ = '\r';
    }

    *str++ = chrs[i];
  }

  *str = 0;
  return str;
}

void ui_clip_request_data(uint32 format)
{
  char* str = NULL;
  int	len = 1; // Always reserve space for NUL byte

  if (format == CF_TEXT) {
    if (!amiga_clip_active_post() && // Avoid deadlocks
	OpenIFF(amiga_clip_iffhandle, IFFF_READ) == 0) {
      if (CollectionChunk(amiga_clip_iffhandle, ID_FTXT, ID_CHRS) == 0 &&
	  StopOnExit(amiga_clip_iffhandle,ID_FTXT,ID_FORM) == 0) {
	while (TRUE) {
	  LONG error = ParseIFF(amiga_clip_iffhandle, IFFPARSE_SCAN);

	  if (error == IFFERR_EOF) {
	    break;
	  }
	  else if (error == IFFERR_EOC) {
	    struct CollectionItem* ci;
	    struct CollectionItem* first_ci;

 	    ci = first_ci = FindCollection(amiga_clip_iffhandle,
					   ID_FTXT, ID_CHRS);

	    while (ci != NULL) {
	      int   i;
	      char* chrs = ci->ci_Data;

	      len += ci->ci_Size;

	      for (i = 0; i < ci->ci_Size; ++i) {
		if (chrs[i] == '\n') {
		  ++len;
		}
	      }

	      ci = ci->ci_Next;
	    }

	    str = malloc(len);

	    if (str != NULL) {
	      extract_collection(first_ci, str);
	    }
 	  }
	  else {
	    // Something is wrong!
	    break;
	  }
	}
      }
      
      CloseIFF(amiga_clip_iffhandle);
    }

    if (str != NULL) {
      cliprdr_send_data(str, len);
      free(str);
    }
    else {
      cliprdr_send_data("", 0);
    }

  }
}

void ui_clip_sync(void)
{
  // TODO: Check format of all clipboard.device data here
  cliprdr_send_text_format_announce();
}
