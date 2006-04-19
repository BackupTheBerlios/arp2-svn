/*
  rdesktop: A Remote Desktop Protocol client.
  User interface services - Amiga
  Copyright (C) Martin Blom 2001-2005
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

#include "config.h"

#include "rdesktop.h"

#undef BOOL
#define Bool int

#include <devices/timer.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#define GADID_ICONIFY   2000
#define GADID_STICKY    2001
#define GADID_ZOOM      2002

extern char g_title[];

extern BOOL             amiga_quit;
extern BOOL             amiga_is_os4;
extern ULONG            amiga_bpp;
extern struct Screen*   amiga_screen;
extern struct Window*   amiga_window;
extern struct DrawInfo* amiga_draw_info;

static struct Window*      amiga_window2          = NULL;
static struct timerequest* amiga_timerreq         = NULL;
static struct MsgPort*     amiga_timerport        = NULL;

static BOOL             amiga_timerdevice_opened  = FALSE;
static BOOL             amiga_cbar_sticky         = FALSE;
BOOL                    amiga_cbar_visible        = TRUE;

static struct Image*    amiga_cbar_depth_image    = NULL;
static struct Image*    amiga_cbar_zoom_image     = NULL;
static struct Image*    amiga_cbar_iconify_image  = NULL;
static struct Image*    amiga_cbar_drag_image     = NULL;
static struct Image*    amiga_cbar_sticky_image   = NULL;
static struct Gadget*   amiga_cbar_depth_gadget   = NULL;
static struct Gadget*   amiga_cbar_zoom_gadget    = NULL;
static struct Gadget*   amiga_cbar_iconify_gadget = NULL;
static struct Gadget*   amiga_cbar_drag_gadget    = NULL;
static struct Gadget*   amiga_cbar_sticky_gadget  = NULL;


ULONG
amiga_cbar_get_signal_mask() {
  ULONG mask = 0;

  if( amiga_window2 != NULL )
  {
    mask |= ( 1UL << amiga_window2->UserPort->mp_SigBit );
    if (amiga_timerdevice_opened)
    {
      mask |= (1 << amiga_timerport->mp_SigBit);
    }
  }
  
  return mask;
}


BOOL
amiga_cbar_open(struct TagItem const* common_window_tags)
{
  if (!amiga_is_os4 || amiga_bpp > 8)
  {
    amiga_timerport = CreateMsgPort();
    if (amiga_timerport)
    {
      amiga_timerreq = (struct timerequest *)CreateIORequest(amiga_timerport, sizeof(struct timerequest));
      if (amiga_timerreq)
      {
	if (0 == OpenDevice("timer.device", UNIT_VBLANK, &amiga_timerreq->tr_node, 0))
	{
	  uint32 size = (amiga_window->WScreen->Flags & SCREENHIRES ? SYSISIZE_MEDRES : SYSISIZE_LOWRES);
	  uint32 height = amiga_window->WScreen->Font->ta_YSize + amiga_window->WScreen->WBorTop + 1;

	  int pos = 0;
	  int cnt = 0;
	  struct Gadget* prev = NULL;

	  struct TagItem const common_image_tags[] = 
	    {
	      { SYSIA_Size,     size                    },
	      { SYSIA_DrawInfo, (ULONG) amiga_draw_info },
	      { IA_Height,      height                  },
#if defined (__amigaos4__)
	      {IA_InBorder,    TRUE                     },
#endif
	      {TAG_DONE,       0                        }
	    };

	  struct TagItem const common_gadget_tags[] = 
	    {
	      { GA_RelVerify, TRUE              },
	      { GA_TopBorder, TRUE              },
#if defined (__amigaos4__)
	      { GA_Titlebar,  TRUE              },
#endif
	      {TAG_DONE,      0                 }
	    };

	  amiga_timerdevice_opened           = TRUE;
	  amiga_timerreq->tr_time.tv_secs    = 5;
	  amiga_timerreq->tr_time.tv_micro   = 0;
	  amiga_timerreq->tr_node.io_Command = TR_ADDREQUEST;
	  SendIO(&amiga_timerreq->tr_node);

	  // Create connection bar images for the titlebar gadgets
	  amiga_cbar_depth_image = (struct Image *)NewObject( NULL, "sysiclass",
							    SYSIA_Which, DEPTHIMAGE,
							    TAG_MORE, (ULONG) common_image_tags );


	  amiga_cbar_zoom_image = (struct Image *)NewObject( NULL, "sysiclass",
							   SYSIA_Which, ZOOMIMAGE,
							   TAG_MORE, (ULONG) common_image_tags );

#ifdef ICONIFYIMAGE
	  amiga_cbar_iconify_image = (struct Image *)NewObject( NULL, "sysiclass",
							      SYSIA_Which, ICONIFYIMAGE,
							      TAG_MORE, (ULONG) common_image_tags );
#endif

#ifdef __amigaos4__
	  amiga_cbar_drag_image = (struct Image *)NewObject( NULL, "sysiclass",
							   SYSIA_Which, TBFRAMEIMAGE,
							   SYSIA_Label, g_title,
							   TAG_MORE, (ULONG) common_image_tags );
#endif

#if defined(PADLOCKIMAGE)
	  amiga_cbar_sticky_image = (struct Image *)NewObject( NULL, "sysiclass",
							     SYSIA_Which, PADLOCKIMAGE,
							     TAG_MORE, (ULONG) common_image_tags );
#elif defined(LOCKIMAGE)
	  amiga_cbar_sticky_image = (struct Image *)NewObject( NULL, "sysiclass",
							     SYSIA_Which, LOCKIMAGE,
							     TAG_MORE, (ULONG) common_image_tags );
#endif

	  // Now create the gadgets
	  if (amiga_cbar_depth_image)
	  {
	    pos -= amiga_cbar_depth_image->Width;
	    
	    amiga_cbar_depth_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								GA_Image, (ULONG) amiga_cbar_depth_image,
								GA_SysGType, GTYP_SDEPTH,
								amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								GA_Next, (ULONG) prev,
								TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cbar_depth_gadget) {
	      ++cnt;
	      prev = amiga_cbar_depth_gadget;
	    }
	  }

	  if (amiga_cbar_zoom_image)
	  {
	    pos -= amiga_cbar_zoom_image->Width;

	    amiga_cbar_zoom_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
							       GA_Image, (ULONG) amiga_cbar_zoom_image,
							       GA_ID, GADID_ZOOM,
							       amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
							       GA_Next, (ULONG) prev,
							       TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cbar_zoom_gadget) {
	      ++cnt;
	      prev = amiga_cbar_zoom_gadget;
	    }
	  }

	  if (amiga_cbar_iconify_image)
	  {
	    pos -= amiga_cbar_iconify_image->Width;

	    amiga_cbar_iconify_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								  GA_Image, (ULONG) amiga_cbar_iconify_image,
								  GA_ID, GADID_ICONIFY,
								  amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								  GA_Next, (ULONG) prev,
								  TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cbar_iconify_gadget) {
	      ++cnt;
	      prev = amiga_cbar_iconify_gadget;
	    }
	  }


	  if (amiga_cbar_drag_image)
	  {
	    amiga_cbar_drag_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
							       GA_Image, (ULONG) amiga_cbar_drag_image,
							       GA_SysGType, GTYP_SDRAGGING,
							       GA_Next, (ULONG) prev,
							       TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cbar_drag_gadget) {
	      ++cnt;
	      prev = amiga_cbar_drag_gadget;
	    }
	  }

	  if (amiga_cbar_sticky_image)
	  {
	    pos -= amiga_cbar_sticky_image->Width;

	    amiga_cbar_sticky_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								 GA_Image, (ULONG) amiga_cbar_sticky_image,
								 GA_ID, GADID_STICKY,
								 amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								 GA_Next, (ULONG) prev,
								 TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cbar_sticky_gadget) {
	      ++cnt;
	      prev = amiga_cbar_sticky_gadget;
	    }
	  }

	  if (prev)
	  {
	    struct TagItem const extra_tags[] = 
	      {
#if defined (__amigaos4__)
		{ WA_ToolBox,  TRUE },
		{ WA_StayTop,  TRUE },
#endif
		{TAG_DONE,     0    }
	      };

	    amiga_window2 = OpenWindowTags( NULL,
					    WA_Left,           amiga_screen->Width / 4,
					    WA_Top,            0,
					    WA_Width,          amiga_screen->Width / 2,
					    WA_Height,         height,
					    WA_CustomScreen,   (ULONG) amiga_screen,
					    WA_SmartRefresh,   TRUE,
					    WA_WindowName,     (ULONG) g_title,
					    WA_Title,          (ULONG) g_title,
					    WA_Activate,       FALSE,
					    WA_CloseGadget,    TRUE,
					    WA_Gadgets,        (ULONG) prev,
					    WA_IDCMP,          (IDCMP_CLOSEWINDOW |
								IDCMP_GADGETUP |
								IDCMP_ACTIVEWINDOW |
								IDCMP_SIZEVERIFY),
					    TAG_MORE,          (ULONG) extra_tags);


	    if (amiga_window2)
	    {
#ifdef __amigaos4__
	      ChangeWindowBox(amiga_window2, g_width / 2 - (amiga_cbar_depth_gadget->LeftEdge + amiga_cbar_depth_gadget->Width) / 2, 0, amiga_cbar_depth_gadget->LeftEdge + amiga_cbar_depth_gadget->Width, amiga_window2->Height);
#else
/*           RemoveGList(amiga_window2, prev, cnt); */
/*           AddGList(amiga_window2, prev, 0, cnt, NULL); */
/*           RefreshGList(prev, amiga_window2, NULL, -1); */
#endif
	      amiga_cbar_visible = TRUE;
	    }
	  }
	}
      }
    }
  }

  return amiga_window2 != NULL;
}

void
amiga_cbar_close()
{
  if (amiga_draw_info)
  {
    FreeScreenDrawInfo(amiga_window->WScreen, amiga_draw_info);
    amiga_draw_info = NULL;
  }

  // Remove gadgets from window and free them
  if (amiga_cbar_depth_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cbar_depth_gadget);
    }
    DisposeObject((Object *)amiga_cbar_depth_gadget);
    amiga_cbar_depth_gadget = NULL;
  }

  if (amiga_cbar_iconify_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cbar_iconify_gadget);
    }
    DisposeObject((Object *)amiga_cbar_iconify_gadget);
    amiga_cbar_iconify_gadget = NULL;
  }

  if (amiga_cbar_sticky_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cbar_sticky_gadget);
    }
    DisposeObject((Object *)amiga_cbar_sticky_gadget);
    amiga_cbar_sticky_gadget = NULL;
  }

  if (amiga_cbar_drag_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cbar_drag_gadget);
    }
    DisposeObject((Object *)amiga_cbar_drag_gadget);
    amiga_cbar_drag_gadget = NULL;
  }

  // Dispose images
  DisposeObject((Object *)amiga_cbar_depth_image);
  amiga_cbar_depth_image = NULL;

  DisposeObject((Object *)amiga_cbar_zoom_image);
  amiga_cbar_zoom_image = NULL;

  DisposeObject((Object *)amiga_cbar_iconify_image);
  amiga_cbar_iconify_image = NULL;

  DisposeObject((Object *)amiga_cbar_sticky_image);
  amiga_cbar_sticky_image = NULL;

  DisposeObject((Object *)amiga_cbar_drag_image);
  amiga_cbar_drag_image = NULL;

  // Close window and timer.device
  if( amiga_window2 != NULL )
  {
    CloseWindow( amiga_window2 );
    amiga_window2 = NULL;
  }

  if (amiga_timerdevice_opened)
  {
    AbortIO(&amiga_timerreq->tr_node);
    WaitIO(&amiga_timerreq->tr_node);
    CloseDevice(&amiga_timerreq->tr_node);
    amiga_timerdevice_opened = FALSE;
  }

  DeleteIORequest(&amiga_timerreq->tr_node);
  amiga_timerreq = NULL;

  DeleteMsgPort(amiga_timerport);
  amiga_timerport = NULL;
}

void
amiga_cbar_show(void) 
{
  if (amiga_window2 != NULL)
  {
    amiga_cbar_visible = TRUE;

#if defined (__amigaos4__)
    int i;

    for (i = 0 ; i < amiga_window2->Height ; i++)
    {
      MoveWindow(amiga_window2, 0, 1);
      Delay(1);
    }
#elif defined (__MORPHOS__)
    ShowWindow( amiga_window2 );
    ActivateWindow( amiga_window );
#endif
  }
}


void
amiga_cbar_hide(void) 
{
  if (amiga_window2 != NULL) 
  {
#if defined (__amigaos4__)
    int i;

    for (i = 0 ; i < amiga_window2->Height ; i++)
    {
      MoveWindow(amiga_window2, 0, -1);
      Delay(1);
    }
#elif defined (__MORPHOS__)
    HideWindow( amiga_window2 );
#endif
  }

  amiga_cbar_visible = FALSE;
}


void
amiga_cbar_set_mouse(int x, int y) 
{
  if( amiga_window2 && !amiga_cbar_visible && 
      y <= 0 && x >= amiga_window2->LeftEdge && x < amiga_window2->LeftEdge + amiga_window2->Width)
  {
    amiga_cbar_show();
  }
}


void 
amiga_cbar_handle_events(ULONG mask)
{
  if( amiga_window2 != NULL &&
      ( mask & ( 1UL << amiga_window2->UserPort->mp_SigBit ) ) )
  {
    struct IntuiMessage* msg;

    while( ( msg = (struct IntuiMessage*) 
	     GetMsg( amiga_window2->UserPort ) ) != NULL )
    {
      switch( msg->Class )
      {
	case IDCMP_CLOSEWINDOW:
	  amiga_quit = TRUE;
	  break;

	case IDCMP_GADGETUP:
	{
	  struct Gadget* g = (struct Gadget*) msg->IAddress;

#ifdef __amigaos4__
	  if( g->GadgetID == GADID_ICONIFY &&
	      amiga_app_icon == NULL )
	  {
	    amiga_icon->do_Type = 0;

	    amiga_app_icon = AddAppIconA( 0, 0, g_title,
					  amiga_wb_port, 0, amiga_icon, NULL );

	    if( amiga_app_icon != NULL )
	    {
	      MoveScreen( amiga_screen, 0, amiga_screen->Height);
	      HideWindow( amiga_window );
	      HideWindow( amiga_window2 );
	    }
	  }
#endif

	  if( g->GadgetID == GADID_STICKY )
	  {
	    amiga_cbar_sticky = !amiga_cbar_sticky;
	    if (amiga_cbar_visible)
	    {
	      if (amiga_cbar_sticky)
	      {
		DrawImageState(amiga_window2->RPort, amiga_cbar_sticky_image, amiga_cbar_sticky_gadget->LeftEdge, amiga_cbar_sticky_gadget->TopEdge, IDS_INACTIVESELECTED, amiga_draw_info);
	      } else {
		amiga_cbar_hide();
	      }
	    }
	  }
	  break;
	}
      }

      ReplyMsg( (struct Message*) msg );
    }
  }

  if( amiga_timerdevice_opened &&
      ( mask & ( 1UL << amiga_timerport->mp_SigBit ) ) )
  {
    struct Message *msg = GetMsg(amiga_timerport);

    if (msg)
    {
      amiga_timerreq->tr_time.tv_secs    = 5;
      amiga_timerreq->tr_time.tv_micro   = 0;
      amiga_timerreq->tr_node.io_Command = TR_ADDREQUEST;
      SendIO(&amiga_timerreq->tr_node);

      if (!amiga_cbar_sticky)
      {
	if (amiga_cbar_visible
	    && (amiga_window->MouseY > amiga_window2->Height
		|| amiga_window->MouseX < amiga_window2->LeftEdge
		|| amiga_window->MouseX >= amiga_window2->LeftEdge + amiga_window2->Width))
	{
	  amiga_cbar_hide();
	} else if (!amiga_cbar_visible
		   && amiga_window->MouseY <= 0
		   && amiga_window->MouseX >= amiga_window2->LeftEdge
		   && amiga_window->MouseX < amiga_window2->LeftEdge + amiga_window2->Width)
	{
	  amiga_cbar_show();
	}
      }
    }
  }
}

