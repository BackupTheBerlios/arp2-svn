/*
  rdesktop: A Remote Desktop Protocol client.
  Clipboard handling - Amiga
  Copyright (C) Martin Blom 2001-2004
  Copyright (C) Matthew Chapman 1999-2000
   
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
#include <utility/hooks.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include "amiga_clipboard.h"

static struct MsgPort*   amiga_clip_port = NULL;
static struct IOClipReq* amiga_clip_req  = NULL;

static ULONG amiga_clip_hookfunc(struct Hook*        hook,
				 APTR                object,
				 struct ClipHookMsg* msg) {
  printf("Got cliphookmsg %d: Command %d, ID %d\n",
	 msg->chm_Type, msg->chm_ChangeCmd, msg->chm_ClipID);
  if (msg->chm_Type == 0) {
    // Ask main processing loop to call cliprdr_send_text_format_announce()
    Signal((struct Task*) hook->h_Data, SIGBREAKF_CTRL_F);
  }
}

static struct Hook amiga_clip_hook = {
  { NULL, NULL },
  HookEntry,
  amiga_clip_hookfunc,
  NULL
};


BOOL
amiga_clip_init(void) {
  amiga_clip_port = CreateMsgPort();

  if (amiga_clip_port != NULL) {
    amiga_clip_req = (struct IOClipReq*) CreateIORequest(amiga_clip_port,
							 sizeof *amiga_clip_req);

    if (OpenDevice("clipboard.device", PRIMARY_CLIP,
		   (struct IORequest*) amiga_clip_req, 0) == 0) {

      // Add clipboard hook
      amiga_clip_hook.h_Data = FindTask(NULL);

      amiga_clip_req->io_Data    = (char*) &amiga_clip_hook;
      amiga_clip_req->io_Length  = 1;
      amiga_clip_req->io_Command = CBD_CHANGEHOOK;
      DoIO((struct IORequest*) amiga_clip_req);

      return TRUE;
    }
  }

  return FALSE;
}

void
amiga_clip_deinit(void) {
  if (amiga_clip_req != NULL) {
    if (amiga_clip_hook.h_Data != NULL) {

      // Remove clipboard hook
      amiga_clip_hook.h_Data = NULL;

      amiga_clip_req->io_Data    = (char*) &amiga_clip_hook;
      amiga_clip_req->io_Length  = 0;
      amiga_clip_req->io_Command = CBD_CHANGEHOOK;
      DoIO((struct IORequest*) amiga_clip_req);
    }
    
    CloseDevice((struct IORequest*) amiga_clip_req);
    DeleteIORequest((struct IORequest*) amiga_clip_req);
  }

  DeleteMsgPort(amiga_clip_port);
}


void
ui_clip_format_announce(uint8 * data, uint32 length)
{
  int i;
  printf( "ui_clip_format_announce (%d bytes)\n", length);
}

void ui_clip_handle_data(uint8 * data, uint32 length)
{
  printf( "ui_clip_handle_data (%d bytes) '%s'\n", length, data );
}

void ui_clip_request_data(uint32 format)
{
  printf( "ui_clip_request_data (format %x)\n", format );

  if (format == CF_TEXT) {
    cliprdr_send_data("Banan", 6);
  }
}

void ui_clip_sync(void)
{
  printf( "ui_clip_sync\n" );

  cliprdr_send_text_format_announce();
}
