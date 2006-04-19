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

#ifdef ETI_Iconify
# define GADID_ICONIFY ETI_Iconify
#else
# define GADID_ICONIFY 1000
#endif
#define GADID_STICKY   1001
#define GADID_ZOOM     1002

#include "rdesktop.h"

#undef BOOL
#define Bool int

#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#if defined(__amigaos4__)
# include <unistd.h>
# include <time.h>
# include <proto/bsdsocket.h>
#elif defined(__libnix__)
#  include <libnix.h>
#elif defined(__ixemul__)
# include <ix.h>
#else
# error I need OS4, ixemul or libnix!
#endif

#include <cybergraphx/cybergraphics.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#ifdef HAVE_DEVICES_NEWMOUSE_H
# include <devices/newmouse.h>
#endif
#ifdef HAVE_DEVICES_RAWKEYCODES_H
# include <devices/rawkeycodes.h>
#endif
#include <devices/timer.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/modeid.h>
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/pointerclass.h>
#include <libraries/asl.h>
#ifdef HAVE_NEWMOUSE_H
# include <newmouse.h>
#endif
#include <workbench/workbench.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/cybergraphics.h>
#include <proto/exec.h>
#undef GetOutlinePen // I get an annoying warning if this is defined
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/wb.h>

#ifdef __MORPHOS__
# include <intuition/extensions.h>
#endif

#include "amiga_clipboard.h"
#include "amiga_support.h"

#define TRACEFUNC printf("%s()\n", __func__);

extern int  g_width;
extern int  g_height;
extern Bool g_sendmotion;
extern Bool g_fullscreen;
extern char g_title[];
extern int  g_server_bpp;
extern Bool g_bitmap_cache_persist_enable;
extern int  a_slowmouse;

#ifdef WITH_RDPSND
extern int amiga_audio_signal;
#endif

int    amiga_left           = 0;
int    amiga_top            = 0;
STRPTR amiga_pubscreen_name = NULL;
ULONG  amiga_screen_id      = DEFAULT_MONITOR_ID;
struct DiskObject* amiga_icon = NULL;

struct MsgPort* amiga_wb_port  = NULL;
struct AppIcon* amiga_app_icon = NULL;

BOOL           amiga_quit              = FALSE;

#ifdef __amigaos4__
BOOL           amiga_is_os4             = TRUE;
#else
BOOL           amiga_is_morphos         = FALSE;
BOOL           amiga_is_amithlon        = FALSE;
BOOL           amiga_is_os4             = FALSE;
#endif

UWORD          amiga_last_qualifier     = 0;
BOOL           amiga_numlock            = TRUE;  // default state is on
BOOL           amiga_scrolllock         = FALSE; // default state is off
BOOL           amiga_capslock           = 0xbad; // -> sync on first key

HCURSOR        amiga_last_cursor        = NULL;
HCURSOR        amiga_null_cursor        = NULL;

BOOL           amiga_broken_cursor      = FALSE;
BOOL           amiga_broken_blitter     = FALSE;

struct Screen* amiga_pubscreen          = NULL;
ULONG          amiga_bpp                = 8;
struct Screen* amiga_screen             = NULL;
UBYTE*         amiga_backup             = NULL;
struct Window* amiga_window             = NULL;
ULONG          amiga_cursor_colors[ 3 * 3 ];
LONG           amiga_pens[ 256 ];
APTR           amiga_tmp_buffer         = NULL;
ULONG          amiga_tmp_buffer_length  = 0;
struct BitMap* amiga_tmp_bitmap         = NULL;
BOOL           amiga_clipping           = FALSE;

/* For the connection bar */
struct Window* amiga_window2            = NULL;
struct timerequest *amiga_timerreq      = NULL;
struct MsgPort *amiga_timerport         = NULL;
uint32 amiga_timerdevice_opened         = FALSE;
BOOL amiga_connection_bar_sticky        = FALSE;
BOOL amiga_connection_bar_visible       = TRUE;

static struct DrawInfo *amiga_draw_info        = NULL;

static struct Image    *amiga_cb_depth_image   = NULL;
static struct Image    *amiga_cb_zoom_image    = NULL;
static struct Image    *amiga_cb_iconify_image = NULL;
static struct Image    *amiga_cb_drag_image    = NULL;
static struct Image    *amiga_cb_sticky_image  = NULL;

static struct Gadget   *amiga_cb_depth_gadget  = NULL;
static struct Gadget   *amiga_cb_zoom_gadget   = NULL;
static struct Gadget   *amiga_cb_iconify_gadget= NULL;
static struct Gadget   *amiga_cb_drag_gadget   = NULL;
static struct Gadget   *amiga_cb_sticky_gadget = NULL;

#ifdef __amigaos4__
static struct Image    *amiga_iconify_image     = NULL;
static struct Gadget   *amiga_iconify_gadget    = NULL;
#endif

struct Glyph
{
    int           BytesPerRow;
    unsigned char Data[0];
};

struct Cursor
{
    uint8*        Planes;
    struct BitMap BitMap;
    Object*       Pointer;
};

#ifdef __libnix__
// We handle it ourselves
void __chkabort(void) {}
#endif



static BOOL
amiga_connection_bar_open(struct TagItem const* common_window_tags)
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
	  amiga_cb_depth_image = (struct Image *)NewObject( NULL, "sysiclass",
							    SYSIA_Which, DEPTHIMAGE,
							    TAG_MORE, (ULONG) common_image_tags );


	  amiga_cb_zoom_image = (struct Image *)NewObject( NULL, "sysiclass",
							   SYSIA_Which, ZOOMIMAGE,
							   TAG_MORE, (ULONG) common_image_tags );

#ifdef ICONIFYIMAGE
	  amiga_cb_iconify_image = (struct Image *)NewObject( NULL, "sysiclass",
							      SYSIA_Which, ICONIFYIMAGE,
							      TAG_MORE, (ULONG) common_image_tags );
#endif

#ifdef __amigaos4__
	  amiga_cb_drag_image = (struct Image *)NewObject( NULL, "sysiclass",
							   SYSIA_Which, TBFRAMEIMAGE,
							   SYSIA_Label, g_title,
							   TAG_MORE, (ULONG) common_image_tags );
#endif

#if defined(PADLOCKIMAGE)
	  amiga_cb_sticky_image = (struct Image *)NewObject( NULL, "sysiclass",
							     SYSIA_Which, PADLOCKIMAGE,
							     TAG_MORE, (ULONG) common_image_tags );
#elif defined(LOCKIMAGE)
	  amiga_cb_sticky_image = (struct Image *)NewObject( NULL, "sysiclass",
							     SYSIA_Which, LOCKIMAGE,
							     TAG_MORE, (ULONG) common_image_tags );
#endif

	  // Now create the gadgets
	  if (amiga_cb_depth_image)
	  {
	    pos -= amiga_cb_depth_image->Width;
	    
	    amiga_cb_depth_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								GA_Image, (ULONG) amiga_cb_depth_image,
								GA_SysGType, GTYP_SDEPTH,
								amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								GA_Next, (ULONG) prev,
								TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_depth_gadget) {
	      ++cnt;
	      prev = amiga_cb_depth_gadget;
	    }
	  }

	  if (amiga_cb_zoom_image)
	  {
	    pos -= amiga_cb_zoom_image->Width;

	    amiga_cb_zoom_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
							       GA_Image, (ULONG) amiga_cb_zoom_image,
							       GA_ID, GADID_ZOOM,
							       amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
							       GA_Next, (ULONG) prev,
							       TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_zoom_gadget) {
	      ++cnt;
	      prev = amiga_cb_zoom_gadget;
	    }
	  }

	  if (amiga_cb_iconify_image)
	  {
	    pos -= amiga_cb_iconify_image->Width;

	    amiga_cb_iconify_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								  GA_Image, (ULONG) amiga_cb_iconify_image,
								  GA_ID, GADID_ICONIFY,
								  amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								  GA_Next, (ULONG) prev,
								  TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_iconify_gadget) {
	      ++cnt;
	      prev = amiga_cb_iconify_gadget;
	    }
	  }


	  if (amiga_cb_drag_image)
	  {
	    amiga_cb_drag_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
							       GA_Image, (ULONG) amiga_cb_drag_image,
							       GA_SysGType, GTYP_SDRAGGING,
							       GA_Next, (ULONG) prev,
							       TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_drag_gadget) {
	      ++cnt;
	      prev = amiga_cb_drag_gadget;
	    }
	  }

	  src += data_width * 3;
	}
	
	break;
      }

      default:
	error( "amiga_write_pixels: Illegal server bitplane depth.\n" );
	break;
    }

    WritePixelArray( argb_data, 0, 0, width * 4, rp, x, y, width, height, RECTFMT_ARGB );
  }
}


static void
amiga_read_video_memory( void* buffer, int xmod,
                         int x, int y, int cx, int cy,
                         BOOL truecolor )
{
  if( ! truecolor )
  {
    struct RastPort temprp;

    InitRastPort(&temprp);

    temprp.BitMap = AllocBitMap( cx, 1, amiga_bpp,
				 BMF_MINPLANES, amiga_window->RPort->BitMap );

    if( temprp.BitMap != NULL)
    {
      ReadPixelArray8( amiga_window->RPort,
                       x, y,
                       x + cx -1, y + cy - 1,
                       buffer, &temprp);
      WaitBlit();
      FreeBitMap(temprp.BitMap);
    }
  }  
  else
  {
    ReadPixelArray( buffer, 0, 0, xmod,
                    amiga_window->RPort, 
                    x, y,
                    cx, cy,
                    RECTFMT_ARGB );
  }
}

static void
amiga_write_video_memory( void* buffer, int xmod,
                          int x, int y, int cx, int cy,
                          BOOL truecolor )
{
  if( ! truecolor )
  {
    SafeWriteChunkyPixels( amiga_window->RPort,
                           x, y,
                           x + cx -1, y + cy - 1,
                           buffer, xmod );
  }
  else
  {
    WritePixelArray( buffer, 0, 0, xmod,
                     amiga_window->RPort, 
                     x, y,
                     cx, cy,
                     RECTFMT_ARGB );
  }
}


static void
amiga_backup_window( void )
{
  int bytesperpixel = 1;
  int bytesperrow   = 0;

  if( CyberGfxBase != NULL &&
      GetCyberMapAttr( amiga_window->RPort->BitMap,
		       CYBRMATTR_ISCYBERGFX ) &&
      GetCyberMapAttr( amiga_window->RPort->BitMap, 
		       CYBRMATTR_PIXFMT ) != PIXFMT_LUT8 )
  {
    bytesperpixel = 4;
  }

  bytesperrow = ( (g_width + 15) & ~15 ) * bytesperpixel;

  if( amiga_backup == NULL )
  {
    amiga_backup = xmalloc( bytesperrow * g_height );
  }

  if( amiga_backup != NULL )
  {
    amiga_read_video_memory( amiga_backup, bytesperrow,
			     amiga_window->BorderLeft,
			     amiga_window->BorderTop,
			     g_width, g_height,
			     bytesperpixel != 1 );
  }
}  


static void
amiga_restore_window( void )
{
  int bytesperpixel = 1;
  int bytesperrow   = 0;

  if( CyberGfxBase != NULL &&
      GetCyberMapAttr( amiga_window->RPort->BitMap,
		       CYBRMATTR_ISCYBERGFX ) &&
      GetCyberMapAttr( amiga_window->RPort->BitMap, 
		       CYBRMATTR_PIXFMT ) != PIXFMT_LUT8 )
  {
    bytesperpixel = 4;
  }

  bytesperrow = ( (g_width + 15) & ~15 ) * bytesperpixel;

  if( amiga_backup != NULL )
  {
    amiga_write_video_memory( amiga_backup, bytesperrow,
			      amiga_window->BorderLeft,
			      amiga_window->BorderTop,
			      g_width, g_height,
			      bytesperpixel != 1 );

    WaitBlit();
    xfree( amiga_backup );
    amiga_backup = NULL;
  }
}


static int
amiga_translate_key( int code, ULONG qualifier, BOOL* numlock )
{
  // Good URLs:
  // http://panda.cs.ndsu.nodak.edu/~achapwes/PICmicro/keyboard/scancodes1.html
  // http://library.n0i.net/linux-unix/programming/ke-scan/scancodes-1.html

  // OS4 handles NumLock internally, so we have to undo it.
  if (amiga_is_os4 && (qualifier & IEQUALIFIER_NUMERICPAD)) {
    int new_code = code;
    
    switch (code) {
      case 0x70: new_code = 0x3d; break;     //  7    
      case 0x4c: new_code = 0x3e; break;     //  8    
      case 0x48: new_code = 0x3f; break;     //  9    
      case 0x4f: new_code = 0x2d; break;     //  4    
//    case nada: new_code = 0x2e; break;     //  5 (no code sent?)
      case 0x4e: new_code = 0x2f; break;     //  6    
      case 0x71: new_code = 0x1d; break;     //  1    
      case 0x4d: new_code = 0x1e; break;     //  2    
      case 0x49: new_code = 0x1f; break;     //  3    
      case 0x47: new_code = 0x0f; break;     //  0
      case 0x46: new_code = 0x3c; break;     //  ,
    }

    if (code != new_code) {
      // The numeric keypad sent cursor keys -> Undo and clear numlock
      code = new_code;
      *numlock = FALSE;
    }
    else {
      switch (code) {
	case 0x3d:     //  7    
	case 0x3e:     //  8    
	case 0x3f:     //  9    
	case 0x2d:     //  4    
	case 0x2e:     //  5
	case 0x2f:     //  6    
	case 0x1d:     //  1    
	case 0x1e:     //  2    
	case 0x1f:     //  3    
	case 0x0f:     //  0
	case 0x3c:     //  ,
	  // The numeric keypad sent numbers -> Set numlock
	  *numlock = TRUE;
	  break;
      }
    }
  }

  
  switch( code )
  {
    case 0x00:    // tilde
      return 0x29;

    case 0x01:    // 1
    case 0x02:    // 2
    case 0x03:    // 3
    case 0x04:    // 4
    case 0x05:    // 5
    case 0x06:    // 6
    case 0x07:    // 7
    case 0x08:    // 8
    case 0x09:    // 9
    case 0x0a:    // 0
    case 0x0b:    // -
    case 0x0c:    // =
      return code + 1;

    case 0x0d:    // backslash
      return 0x2b;             // Map to backslash (doesn't exist
                               // on PC keyboards!)

    case 0x0e:    // unused
      return 0;
      
    case 0x0f:    // kp 0
      return 0x52;
      
    case 0x10:    // q
    case 0x11:    // w
    case 0x12:    // e
    case 0x13:    // r
    case 0x14:    // t
    case 0x15:    // y
    case 0x16:    // u
    case 0x17:    // i
    case 0x18:    // o
    case 0x19:    // p
    case 0x1a:    // [
    case 0x1b:    // ]
      return code;

    case 0x1c:    // unused
      return 0;
      
    case 0x1d:    // kp 1
    case 0x1e:    // kp 2
    case 0x1f:    // kp 3
      return code + 50;

    case 0x20:    // a
    case 0x21:    // s
    case 0x22:    // d
    case 0x23:    // f
    case 0x24:    // g
    case 0x25:    // h
    case 0x26:    // j
    case 0x27:    // k
    case 0x28:    // l
    case 0x29:    // ;
    case 0x2a:    // '
      return code - 2;

    case 0x2b:    // (national ')
      return 0x2b;

    case 0x2c:    // unused
      return 0;
      
    case 0x2d:    // kp 4
    case 0x2e:    // kp 5
    case 0x2f:    // kp 6
      return code + 30;
      
    case 0x30:    // (national <)
      return 0x56;

    case 0x31:    // z
    case 0x32:    // x
    case 0x33:    // c
    case 0x34:    // v
    case 0x35:    // b
    case 0x36:    // n
    case 0x37:    // m
    case 0x38:    // ,
    case 0x39:    // .
    case 0x3a:    // /
      return code - 5;

    case 0x3b:    // unused
      return 0;
      
    case 0x3c:    // kp .
      return 0x53;

    case 0x3d:    // kp 7
    case 0x3e:    // kp 8
    case 0x3f:    // kp 9
      return code + 10;

    case 0x40:    // space
      return 0x39;
      
    case 0x41:    // backspace
      return 0x0e;

    case 0x42:    // tab
      return 0x0f;

    case 0x43:    // kp enter
      return 0x1c | 0x80;
      
    case 0x44:    // return
      return 0x1c;

    case 0x45:    // esc
      return 0x01;

    case 0x46:    // del
      return 0x53 | 0x80;

    case 0x47:    // MOS/OS4 insert
      return 0x52 | 0x80;

    case 0x48:    // MOS/OS4 page up
      return 0x49 | 0x80;

    case 0x49:    // MOS/OS4 page down
      return 0x51 | 0x80;
      
    case 0x4a:    // kp -
      return 0x4a;

    case 0x4b:    // MOS/OS4 f11
      return 0x57;
      
    case 0x4c:    // up arrow
      return 0x48 | 0x80;

    case 0x4d:    // down arrow
      return 0x50 | 0x80;

    case 0x4e:    // right arrow
      return 0x4d | 0x80;

    case 0x4f:    // left arrow
      return 0x4b | 0x80;

    case 0x50:    // f1
    case 0x51:    // f2
    case 0x52:    // f3
    case 0x53:    // f4
    case 0x54:    // f5
    case 0x55:    // f6
    case 0x56:    // f7
    case 0x57:    // f8
    case 0x58:    // f9
    case 0x59:    // f10
      return code - 21;

    case 0x5a:    // kp [ (NumL)
      return 0x45;                    // Map to Num Lock key

    case 0x5b:    // kp ] (ScrL)
      return 0x46;                    // Map to Scroll Lock key

    case 0x5c:    // kp /
	return 0x35 | 0x80;

    case 0x5d:    // kp *
      if (qualifier & IEQUALIFIER_CONTROL) {
	return 0x37 | 0x80;	      // Map to Print Screen
      }
      else {
	return 0x37;
      }
      
    case 0x5e:    // kp +
      return 0x4e;

    case 0x5f:    // help
      if (amiga_is_os4) {
	return 0x46;		      // Map to Scroll Lock key
      }
      else {
	return 0x5d | 0x80;           // Map to Windows App/Menu key
      }
      
    case 0x60:    // lshift
      return 0x2a;

    case 0x61:    // rshift
      return 0x36;

    case 0x62:    // caps
      return 0x3a;
      
    case 0x63:    // ctrl
      return 0x1d;                    // (0x1d | 0x80 is right ctrl)

    case 0x64:    // lalt
      return 0x38;

    case 0x65:    // ralt
      return 0x38 | 0x80;

    case 0x66:    // lamiga
      return 0x5b | 0x80;             // Map to Windows left GUI key

    case 0x67:    // ramiga
      return 0x5c | 0x80;             // Map to Windows right GUI key

    case 0x68:    // unused mousebutton
    case 0x69:    // unused mousebutton
    case 0x6a:    // unused mousebutton
      break;

    case 0x6b:	  // OS4 Menu key/MOS scroll lock
      if (amiga_is_os4) {
	return 0x5d | 0x80;
      }
      else {
	return 0x46;
      }

    case 0x6c:    // MOS print screen
      if (!amiga_is_os4) {
	return 0x37 | 0x80;
      }
      else {
	break;
      }
      
    case 0x6d:    // OS4 print screen/MOS num lock
      if (amiga_is_os4) {
	return 0x37 | 0x80;
      }
      else {
	return 0x45;
      }

    case 0x6e:    // MOS/OS4 pause
      if (qualifier & IEQUALIFIER_CONTROL) {
	return 0x46 | 0x80;	      // Map to Break
      }
      else {
	break;
      }
      
    case 0x6f:    // MOS/AOS4 f12
      return 0x58;
      
    case 0x70:    // MOS/OS4 home
      return 0x47 | 0x80;

    case 0x71:    // MOS/OS4 end
      return 0x4f | 0x80;

    case 0x72:    // MOS/CDTV stop
      return  0x24 | 0x80;
      
    case 0x73:    // MOS/CDTV play
      return 0x22 | 0x80;
      
    case 0x74:    // MOS/CDTV prev
      return 0x10 | 0x80;
      
    case 0x75:    // MOS/CDTV next
      return 0x19 | 0x80;
      
    case 0x76:    // MOS/CDTV rew
      return 0x2e | 0x80;             // Map to volume down
      
    case 0x77:    // MOS/CDTV ff
      return 0x30 | 0x80;             // Map to volume up

    case 0x79:    // OS4 num lock
      return 0x45;

  }

  return 0;
}


static BOOL
amiga_connection_bar_open(struct TagItem const* common_window_tags)
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
	  amiga_cb_depth_image = (struct Image *)NewObject( NULL, "sysiclass",
							    SYSIA_Which, DEPTHIMAGE,
							    TAG_MORE, (ULONG) common_image_tags );


	  amiga_cb_zoom_image = (struct Image *)NewObject( NULL, "sysiclass",
							   SYSIA_Which, ZOOMIMAGE,
							   TAG_MORE, (ULONG) common_image_tags );

#ifdef ICONIFYIMAGE
	  amiga_cb_iconify_image = (struct Image *)NewObject( NULL, "sysiclass",
							      SYSIA_Which, ICONIFYIMAGE,
							      TAG_MORE, (ULONG) common_image_tags );
#endif

#ifdef __amigaos4__
	  amiga_cb_drag_image = (struct Image *)NewObject( NULL, "sysiclass",
							   SYSIA_Which, TBFRAMEIMAGE,
							   SYSIA_Label, g_title,
							   TAG_MORE, (ULONG) common_image_tags );
#endif

#if defined(PADLOCKIMAGE)
	  amiga_cb_sticky_image = (struct Image *)NewObject( NULL, "sysiclass",
							     SYSIA_Which, PADLOCKIMAGE,
							     TAG_MORE, (ULONG) common_image_tags );
#elif defined(LOCKIMAGE)
	  amiga_cb_sticky_image = (struct Image *)NewObject( NULL, "sysiclass",
							     SYSIA_Which, LOCKIMAGE,
							     TAG_MORE, (ULONG) common_image_tags );
#endif

	  // Now create the gadgets
	  if (amiga_cb_depth_image)
	  {
	    pos -= amiga_cb_depth_image->Width;
	    
	    amiga_cb_depth_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								GA_Image, (ULONG) amiga_cb_depth_image,
								GA_SysGType, GTYP_SDEPTH,
								amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								GA_Next, (ULONG) prev,
								TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_depth_gadget) {
	      ++cnt;
	      prev = amiga_cb_depth_gadget;
	    }
	  }

	  if (amiga_cb_zoom_image)
	  {
	    pos -= amiga_cb_zoom_image->Width;

	    amiga_cb_zoom_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
							       GA_Image, (ULONG) amiga_cb_zoom_image,
							       GA_ID, GADID_ZOOM,
							       amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
							       GA_Next, (ULONG) prev,
							       TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_zoom_gadget) {
	      ++cnt;
	      prev = amiga_cb_zoom_gadget;
	    }
	  }

	  if (amiga_cb_iconify_image)
	  {
	    pos -= amiga_cb_iconify_image->Width;

	    amiga_cb_iconify_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								  GA_Image, (ULONG) amiga_cb_iconify_image,
								  GA_ID, GADID_ICONIFY,
								  amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								  GA_Next, (ULONG) prev,
								  TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_iconify_gadget) {
	      ++cnt;
	      prev = amiga_cb_iconify_gadget;
	    }
	  }


	  if (amiga_cb_drag_image)
	  {
	    amiga_cb_drag_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
							       GA_Image, (ULONG) amiga_cb_drag_image,
							       GA_SysGType, GTYP_SDRAGGING,
							       GA_Next, (ULONG) prev,
							       TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_drag_gadget) {
	      ++cnt;
	      prev = amiga_cb_drag_gadget;
	    }
	  }

	  if (amiga_cb_sticky_image)
	  {
	    pos -= amiga_cb_sticky_image->Width;

	    amiga_cb_sticky_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
								 GA_Image, (ULONG) amiga_cb_sticky_image,
								 GA_ID, GADID_STICKY,
								 amiga_is_os4 ? TAG_IGNORE : GA_RelRight, pos,
								 GA_Next, (ULONG) prev,
								 TAG_MORE, (ULONG) common_gadget_tags );
	    if (amiga_cb_sticky_gadget) {
	      ++cnt;
	      prev = amiga_cb_sticky_gadget;
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
	      ChangeWindowBox(amiga_window2, g_width / 2 - (amiga_cb_depth_gadget->LeftEdge + amiga_cb_depth_gadget->Width) / 2, 0, amiga_cb_depth_gadget->LeftEdge + amiga_cb_depth_gadget->Width, amiga_window2->Height);
#else
/*           RemoveGList(amiga_window2, prev, cnt); */
/*           AddGList(amiga_window2, prev, 0, cnt, NULL); */
/*           RefreshGList(prev, amiga_window2, NULL, -1); */
#endif
	      amiga_connection_bar_visible = TRUE;
	    }
	  }
	}
      }
    }
  }

  return amiga_window2 != NULL;
}

static void
amiga_connection_bar_close()
{
  if (amiga_draw_info)
  {
    FreeScreenDrawInfo(amiga_window->WScreen, amiga_draw_info);
    amiga_draw_info = NULL;
  }

  // Remove gadgets from window and free them
  if (amiga_cb_depth_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cb_depth_gadget);
    }
    DisposeObject((Object *)amiga_cb_depth_gadget);
    amiga_cb_depth_gadget = NULL;
  }

  if (amiga_cb_iconify_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cb_iconify_gadget);
    }
    DisposeObject((Object *)amiga_cb_iconify_gadget);
    amiga_cb_iconify_gadget = NULL;
  }

  if (amiga_cb_sticky_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cb_sticky_gadget);
    }
    DisposeObject((Object *)amiga_cb_sticky_gadget);
    amiga_cb_sticky_gadget = NULL;
  }

  if (amiga_cb_drag_gadget)
  {
    if (amiga_window2)
    {
      RemoveGadget(amiga_window2, amiga_cb_drag_gadget);
    }
    DisposeObject((Object *)amiga_cb_drag_gadget);
    amiga_cb_drag_gadget = NULL;
  }

  // Dispose images
  DisposeObject((Object *)amiga_cb_depth_image);
  amiga_cb_depth_image = NULL;

  DisposeObject((Object *)amiga_cb_zoom_image);
  amiga_cb_zoom_image = NULL;

  DisposeObject((Object *)amiga_cb_iconify_image);
  amiga_cb_iconify_image = NULL;

  DisposeObject((Object *)amiga_cb_sticky_image);
  amiga_cb_sticky_image = NULL;

  DisposeObject((Object *)amiga_cb_drag_image);
  amiga_cb_drag_image = NULL;

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

static void
amiga_connection_bar_show(void) 
{
  if (amiga_window2 != NULL)
  {
    amiga_connection_bar_visible = TRUE;

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


static void
amiga_connection_bar_hide(void) 
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

  amiga_connection_bar_visible = FALSE;
}


static void
amiga_connection_bar_set_mouse(int x, int y) 
{
  if( amiga_window2 && !amiga_connection_bar_visible && 
      y <= 0 && x >= amiga_window2->LeftEdge && x < amiga_window2->LeftEdge + amiga_window2->Width)
  {
    amiga_connection_bar_show();
  }
}


static void 
amiga_connection_bar_handle_events(ULONG mask)
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
	    amiga_connection_bar_sticky = !amiga_connection_bar_sticky;
	    if (amiga_connection_bar_visible)
	    {
	      if (amiga_connection_bar_sticky)
	      {
		DrawImageState(amiga_window2->RPort, amiga_cb_sticky_image, amiga_cb_sticky_gadget->LeftEdge, amiga_cb_sticky_gadget->TopEdge, IDS_INACTIVESELECTED, amiga_draw_info);
	      } else {
		amiga_connection_bar_hide();
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

      if (!amiga_connection_bar_sticky)
      {
	if (amiga_connection_bar_visible
	    && (amiga_window->MouseY > amiga_window2->Height
		|| amiga_window->MouseX < amiga_window2->LeftEdge
		|| amiga_window->MouseX >= amiga_window2->LeftEdge + amiga_window2->Width))
	{
	  amiga_connection_bar_hide();
	} else if (!amiga_connection_bar_visible
		   && amiga_window->MouseY <= 0
		   && amiga_window->MouseX >= amiga_window2->LeftEdge
		   && amiga_window->MouseX < amiga_window2->LeftEdge + amiga_window2->Width)
	{
	  amiga_connection_bar_show();
	}
      }
    }
  }
}


Bool
ui_init(void)
{
  struct ScreenModeRequester* req = NULL;
  
  uint8 null_pointer_mask[2] = { 0xff, 0xff };
  uint8 null_pointer_data[16*1*3];

  bzero( null_pointer_data, sizeof( null_pointer_data ) );
  
  memset( amiga_pens, -1, sizeof( amiga_pens ) );

  amiga_pubscreen = LockPubScreen( amiga_pubscreen_name );

  if( amiga_pubscreen == NULL )
  {
    error( "ui_init: Unable to lock public screen.\n" );
    return False;
  }

  if( CyberGfxBase != NULL &&
      GetCyberMapAttr( amiga_pubscreen->RastPort.BitMap, CYBRMATTR_ISCYBERGFX ) )
  {
    amiga_bpp = GetCyberMapAttr( amiga_pubscreen->RastPort.BitMap, CYBRMATTR_DEPTH );
  }
  else
  {
    amiga_bpp = amiga_pubscreen->RastPort.BitMap->Depth;
  }

  if( g_server_bpp != 0 )
  {
    amiga_bpp = g_server_bpp;
  }
  
  if( g_fullscreen )
  {
    if( amiga_screen_id == DEFAULT_MONITOR_ID)
    {
      struct DimensionInfo di;
      int depth  = amiga_bpp;
      int width  = 640;
      int height = 480;

      amiga_screen_id = GetVPModeID( &amiga_pubscreen->ViewPort );

      if( CyberGfxBase != NULL &&
	  IsCyberModeID( amiga_screen_id ) )
      {
	depth = GetCyberIDAttr( CYBRIDATTR_DEPTH, amiga_screen_id );
	width = GetCyberIDAttr( CYBRIDATTR_WIDTH, amiga_screen_id );
	height = GetCyberIDAttr( CYBRIDATTR_HEIGHT, amiga_screen_id );
      }
    
      req = AllocAslRequestTags( ASL_ScreenModeRequest,  
				 ASLSM_TitleText,            (ULONG) g_title,
				 ASLSM_MinWidth,             640,
				 ASLSM_MinHeight,            480,
				 ASLSM_MaxDepth,             24,
				 ASLSM_InitialDisplayID,     amiga_screen_id,
				 ASLSM_InitialDisplayWidth,  width,
				 ASLSM_InitialDisplayHeight, height,
				 ASLSM_InitialDisplayDepth,  depth,
				 ASLSM_DoWidth,              TRUE,
				 ASLSM_DoHeight,             TRUE,
				 ASLSM_DoDepth,              TRUE,
				 TAG_DONE );

      if( req == NULL )
      {
	error( "ui_init: Unable to create screen mode requester.\n" );
	return False;
      }

      if( ! AslRequestTags( req, TAG_DONE ) )
      {
	return False;
      }
  
      if( GetDisplayInfoData( NULL, (APTR) &di, sizeof( di ), 
			      DTAG_DIMS, req->sm_DisplayID ) == 0 )
      {
	error( "ui_init: Unable to get dimensions of screen.\n" );
	return False;
      }

      amiga_screen_id = req->sm_DisplayID;
      g_width         = req->sm_DisplayWidth;
      g_height        = req->sm_DisplayHeight;
      amiga_bpp       = req->sm_DisplayDepth;

      FreeAslRequest( req );
    }
    else
    {
      if( CyberGfxBase != NULL &&
	  IsCyberModeID( amiga_screen_id ) )
      {
	amiga_bpp = GetCyberIDAttr( CYBRIDATTR_DEPTH, amiga_screen_id );

	if( g_width <= 0 )
	{
	  g_width = GetCyberIDAttr( CYBRIDATTR_WIDTH, amiga_screen_id );
	}

	if( g_height <= 0 )
	{
	  g_height = GetCyberIDAttr( CYBRIDATTR_HEIGHT, amiga_screen_id );
	}
      }
      else
      {
	if( g_server_bpp != 0 )
	{
	  amiga_bpp = g_server_bpp;
	}
	else
	{
	  amiga_bpp = 8;
	}
      }
    }

    g_server_bpp = 0;
  }

  if( g_width == 0 )
  {
    // Default 
    g_width = amiga_pubscreen->Width * 80 / 100;
  }
  else if( g_width < 0 )
  {
    g_width  = amiga_pubscreen->Width * (-g_width) / 100;
  }

  if( g_height == 0 )
  {
    // Default 
    g_height = amiga_pubscreen->Height * 80 / 100;
  }
  else if( g_height < 0 )
  {
    g_height = amiga_pubscreen->Height * (-g_height) / 100;
  }

  if( g_fullscreen )
  {
    // We don't need it anymore
    UnlockPubScreen( NULL, amiga_pubscreen );
    amiga_pubscreen = NULL;
  }

  // The bpp stuff is a bit confusing, but here is a summary:
  //
  // * If a mode requester is used, amiga_bpp is set by that requester
  // * If a screen mode ID is specified, the depth is taken from the
  //   ID if it's a Cybergraphics id. Else, the depth is taken from
  //   the command line (default is 8 bpp but it may be less in the
  //   amiga version of rdesktop).
  // * If window mode, amiga_bpp is taken from the public screen.
  //
  // Now, set g_server_bpp to something appropriate.
  
  switch( amiga_bpp )
  {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
      g_server_bpp = 8;
      break;

    case 15:
      g_server_bpp = 15;
      break;
	
    case 16:
      g_server_bpp = 16;
      break;

    case 24:
    case 32:
      g_server_bpp = 24;
      break;

    default:
      error( "ui_init: Illegal screen depth.\n" );
      return False;
  }

#ifndef __amigaos4__
  // Check for working minterm handling

  if( amiga_bpp > 8 )
  {
    struct Library* working = OpenLibrary( "Picasso96API.library", 0 );

    if( working == NULL )
    {
      amiga_broken_blitter = TRUE;
    }
    else
    {
      CloseLibrary( working );
    }
  }

  // Check for broken cursor handling

  if (OpenResource("amithlon.resource") ||
      OpenResource("umilator.resource")) {
    amiga_is_amithlon = TRUE;
  }

  if (SysBase->LibNode.lib_Version >= 50) {
    if (FindResident("MorphOS")) {
      amiga_is_morphos = TRUE;
    }
    else {
      amiga_is_os4 = TRUE;
    }
  }

  if (amiga_is_amithlon) {
    amiga_broken_cursor = TRUE;
  }
#endif /* __amigaos4__ */
  
  g_width = g_width & ~3;

  /* create invisible 1x1 cursor to be used as null cursor */
  amiga_null_cursor = ui_create_cursor( 0, 0, 16, 1,
					null_pointer_mask, null_pointer_data );

  // Try to enable clipboard handling (just ignore failures)
  if (amiga_clip_init()) {
    cliprdr_init();
  }

  amiga_wb_port = CreateMsgPort();

  if( amiga_wb_port == NULL )
  {
    error("ui_init: Unable to create AppIcon message port\n" );
  }
  
  // OK, we're game
  return True;
}


void
ui_deinit(void)
{
  struct Message* msg;

  amiga_clip_deinit();

  if( amiga_wb_port != NULL )
  {
    while( ( msg = GetMsg( amiga_wb_port ) ) != NULL )
    {
      ReplyMsg( msg );
    }

    DeleteMsgPort( amiga_wb_port );
  }
  
  UnlockPubScreen( NULL, amiga_pubscreen );
  amiga_pubscreen = 0;
  
  ui_destroy_cursor( amiga_null_cursor );
  amiga_null_cursor = 0;

  FreeVec( amiga_tmp_buffer );
}


Bool
ui_create_window(void)
{
  ULONG common_window_tags[] =
    {
      WA_ReportMouse,    g_sendmotion ? TRUE : FALSE,
      WA_Activate,       TRUE,
      WA_RMBTrap,        TRUE,
      WA_ScreenTitle,    (ULONG) g_title,
      WA_WindowName,     (ULONG) g_title,
#ifdef __MORPHOS__
      WA_ExtraGadget_Iconify, amiga_icon != NULL,
#endif
      WA_IDCMP,          (IDCMP_CLOSEWINDOW |
			  IDCMP_GADGETUP |
			  IDCMP_ACTIVEWINDOW |
			  IDCMP_INACTIVEWINDOW |
			  IDCMP_NEWSIZE |
			  IDCMP_SIZEVERIFY |
			  IDCMP_RAWKEY |
			  IDCMP_MOUSEBUTTONS |
#ifdef __amigaos4__
			  IDCMP_EXTENDEDMOUSE |
#endif
			  (g_sendmotion ? IDCMP_MOUSEMOVE : (a_slowmouse ? IDCMP_INTUITICKS : 0))),
      TAG_DONE
    };
  

  if( g_fullscreen )
  {
    struct TagItem const screen_tags[] =
      {
	{ SA_Width,		g_width			},
	{ SA_Height,		g_height		},
	{ SA_Depth,		amiga_bpp		},
	{ SA_Title,		(ULONG) g_title		},
	{ SA_ShowTitle,		FALSE			},
	{ SA_Quiet,		TRUE			},
	{ SA_Type,		CUSTOMSCREEN		},
	{ SA_DisplayID,		amiga_screen_id		},
	{ SA_Interleaved,	TRUE			},
	{ SA_AutoScroll,	TRUE			},
	{ SA_MinimizeISG,	TRUE			},
	{ SA_SharePens,		TRUE			},
	{ SA_LikeWorkbench,	amiga_bpp > 8		},
#ifdef __amigaos4__
	{ SA_OffScreenDragging, amiga_bpp > 8		},
#endif
	{ TAG_DONE,		0			},
      };

    amiga_screen = OpenScreenTagList( NULL, screen_tags );

    if( amiga_screen == NULL )
    {
      error( "ui_create_window: Unable to open screen.\n" );
      return False;
    }
    else 
    {
      struct TagItem const window_tags[] = 
	{
	  { WA_Left,           0			},
	  { WA_Top,            0			},
	  { WA_Width,          amiga_screen->Width	},
	  { WA_Height,         amiga_screen->Height	},
	  { WA_CustomScreen,   (ULONG) amiga_screen	},
	  { WA_Borderless,     TRUE			},
	  { WA_SmartRefresh,   TRUE			},
	  { WA_WindowName,     (ULONG) g_title		},
	  { WA_Backdrop,       TRUE			},
	  { TAG_MORE,          (ULONG) common_window_tags },
	};

      amiga_window = OpenWindowTagList( NULL, window_tags );

      if( amiga_window == NULL )
      {
	error( "ui_create_window: Unable to open window on screen.\n" );
	CloseScreen( amiga_screen );
	return False;
      }

      amiga_draw_info = GetScreenDrawInfo(amiga_window->WScreen);
    }

    amiga_connection_bar_open((struct TagItem const *) common_window_tags);
  }
  else
  {
    struct IntuiText it =
      {
	0, 0,
	0,
	0, 0,
	amiga_pubscreen->Font, g_title,
	NULL
      };

    // NOTE!! amiga_pubscreen is still valid!
    
    WORD zoom[] = { ~0, ~0, 0,
		    amiga_pubscreen->WBorTop + amiga_pubscreen->Font->ta_YSize + 1 };
    
    zoom[ 2 ] = IntuiTextLength( &it ) + 128;
    
    if( zoom[ 2 ] > amiga_pubscreen->Width )
    {
      zoom[ 2 ] = amiga_pubscreen->Width;
    }

    amiga_window = OpenWindowTags( NULL,
                                   WA_Title,          (ULONG) g_title,
                                   WA_DragBar,        TRUE,
                                   WA_DepthGadget,    TRUE,
                                   WA_CloseGadget,    TRUE,
                                   WA_Left,           amiga_left,
                                   WA_Top,            amiga_top,
                                   WA_InnerWidth,     g_width,
                                   WA_InnerHeight,    g_height,
                                   WA_AutoAdjust,     FALSE,
                                   WA_PubScreen,      (ULONG) amiga_pubscreen,
                                   WA_Zoom,           (ULONG) zoom,
                                   WA_SmartRefresh,   TRUE,
                                   TAG_MORE,          (ULONG) common_window_tags );

    // We don't need it anymore
    UnlockPubScreen( NULL, amiga_pubscreen );
    amiga_pubscreen = 0;

    if( amiga_window == NULL )
    {
      error( "ui_create_window: Unable to open window.\n" );
      return False;
    }

    amiga_draw_info = GetScreenDrawInfo(amiga_window->WScreen);

#ifdef __amigaos4__
    if (amiga_window && amiga_icon)
    {
       uint32 size = (amiga_window->WScreen->Flags & SCREENHIRES ? SYSISIZE_MEDRES : SYSISIZE_LOWRES);
       uint32 height = amiga_window->WScreen->Font->ta_YSize + amiga_window->WScreen->WBorTop + 1;

       amiga_iconify_image = (struct Image *)NewObject( NULL, "sysiclass",
                                                       SYSIA_Size, size,
                                                       SYSIA_DrawInfo, amiga_draw_info,
                                                       SYSIA_Which, ICONIFYIMAGE,
                                                       IA_Height, height,
                                                       TAG_DONE );
       if (amiga_iconify_image)
       {
          amiga_iconify_gadget = (struct Gadget *)NewObject( NULL, "buttongclass",
                                                            GA_Image, amiga_iconify_image,
                                                            GA_Titlebar, TRUE,
                                                            GA_RelRight, 0,
                                                            GA_ID, GADID_ICONIFY,
                                                            GA_RelVerify, TRUE,
                                                            TAG_DONE );
       }

       if (amiga_iconify_gadget)
       {
          AddGList(amiga_window, amiga_iconify_gadget, 0, 1, NULL);
          RefreshGList(amiga_iconify_gadget, amiga_window, NULL, 1);
       }
    }
#endif
  }


  GetRGB32( amiga_window->WScreen->ViewPort.ColorMap,
            16 + 1, 3, amiga_cursor_colors );

  return True;
}

void
ui_destroy_window()
{
  ui_reset_clip();
  
  if( amiga_app_icon != NULL )
  {
    RemoveAppIcon( amiga_app_icon );
    amiga_app_icon = NULL;
  }

  amiga_connection_bar_close();

  if( amiga_backup != NULL )
  {
    WaitBlit();
    xfree( amiga_backup );
    amiga_backup = NULL;
  }

  if( amiga_window != NULL )
  {
    unsigned int i;

    for( i = 0; i < sizeof( amiga_pens ) / sizeof( amiga_pens[ 0 ] ); ++i )
    {
      if( amiga_pens[ i ] != -1 )
      {
        ReleasePen( amiga_window->WScreen->ViewPort.ColorMap, amiga_pens[ i ] );
	amiga_pens[ i ] = -1;
      }
    }

    // Restore cursor colors

    SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 1,
              amiga_cursor_colors[ 0 ],
              amiga_cursor_colors[ 1 ],
              amiga_cursor_colors[ 2 ] );

    SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 3,
              amiga_cursor_colors[ 6 ],
              amiga_cursor_colors[ 7 ],
              amiga_cursor_colors[ 8 ] );

#ifdef __amigaos4__
   if (amiga_iconify_gadget)
   {
     RemoveGadget(amiga_window, amiga_iconify_gadget);
     DisposeObject((Object *)amiga_iconify_gadget);
     amiga_iconify_gadget = NULL;
   }

   if (amiga_iconify_image)
   {
     DisposeObject((Object *)amiga_iconify_image);
     amiga_iconify_image = NULL;
   }
#endif

    CloseWindow( amiga_window );
    amiga_window = NULL;
  }

  if( amiga_screen != NULL )
  {
    CloseScreen( amiga_screen );
    amiga_screen = NULL;
  }

  if (amiga_tmp_bitmap)
  {
    WaitBlit();
    FreeBitMap( amiga_tmp_bitmap );
    amiga_tmp_bitmap = NULL;
  }
}


Bool
ui_select(int rdp_socket)
{
  int n = rdp_socket;
  fd_set rfds, wfds;
  struct timeval tv;
  Bool s_timeout = False;

  while(TRUE)
  {
    int   res;
    ULONG mask;

    if (amiga_quit) {
      if (amiga_clip_shutdown()) {
	break;
      }
    }

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_SET(rdp_socket, &rfds);

    /* default timeout */
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    /* add redirection handles */
    rdpdr_add_fds(&n, &rfds, &wfds, &tv, &s_timeout);

    n++;

    mask = (1UL << amiga_wb_port->mp_SigBit) | amiga_clip_signals;

#if !defined(__ixemul__)
      mask |= SIGBREAKF_CTRL_C;
#endif

    if( amiga_window != NULL )
    {
      mask |= ( 1UL << amiga_window->UserPort->mp_SigBit );
    }

    if( amiga_window2 != NULL )
    {
      mask |= ( 1UL << amiga_window2->UserPort->mp_SigBit );
      if (amiga_timerdevice_opened)
      {
         mask |= (1 << amiga_timerport->mp_SigBit);
      }
    }
    
#ifdef WITH_RDPSND
    if( amiga_audio_signal != -1 )
    {
      mask |= ( 1UL << amiga_audio_signal );
    }
#endif

#if defined(__amigaos4__)
    res  = WaitSelect(n, &rfds, &wfds, NULL, &tv, &mask );
#elif defined(__libnix__)
    res  = lx_select(n, &rfds, &wfds, NULL, &tv, &mask );
#elif defined(__ixemul__)
    res  = ix_select(n, &rfds, &wfds, NULL, &tv, &mask );
#else
# error "Don't know how to wait for signals!"
#endif

    if( res == -1 && mask == 0 )
    {
      error("select: %s\n", strerror(errno));
      return False;
    }

#if !defined(__ixemul__)
    if (mask & SIGBREAKF_CTRL_C) {
      SetSignal(0,SIGBREAKF_CTRL_C);
      amiga_quit = TRUE;
    }
#endif

    if (mask & (amiga_clip_signals)) {
      amiga_clip_handle_signals();
    }
    
      if( mask & ( 1UL << amiga_wb_port->mp_SigBit ) )
      {
	struct AppMessage* msg;

	while( ( msg = (struct AppMessage*) 
		 GetMsg( amiga_wb_port ) ) != NULL )
	{
	  if( msg->am_NumArgs == 0 )
	  {
	    if( amiga_app_icon != NULL )
	    {
	      RemoveAppIcon( amiga_app_icon );
	      amiga_app_icon = NULL;
#if defined(__MORPHOS__)
	      ShowWindow( amiga_window );
#elif defined(__amigaos4__)
	      ShowWindow( amiga_window, WINDOW_FRONTMOST );
	      if (amiga_window2)
	      {
                 ShowWindow( amiga_window2, WINDOW_FRONTMOST );
                 MoveScreen( amiga_screen, 0, -amiga_screen->Height);
	      }
	      ActivateWindow( amiga_window );
	      if (amiga_screen)
	      {
	         ScreenToFront(amiga_screen);
	      }
#endif
	    }
	  }

	  ReplyMsg( (struct Message*) msg );
	}
      }

#ifdef WITH_RDPSND
      if( amiga_audio_signal != -1 &&
	  ( mask & ( 1UL << amiga_audio_signal ) ) )
      {
	wave_out_play();
      }
#endif
 
      if( amiga_window != NULL &&
	  ( mask & ( 1UL << amiga_window->UserPort->mp_SigBit ) ) )
      {
	struct IntuiMessage* msg;
	struct InputEvent ie;
	char   q_buffer[8];

	ie.ie_Class = IECLASS_RAWKEY;
	ie.ie_SubClass = 0;

	while( ( msg = (struct IntuiMessage*) 
		 GetMsg( amiga_window->UserPort ) ) != NULL )
	{
	  uint32 ev_time;

	  ev_time = time(NULL);

	  amiga_last_qualifier = msg->Qualifier;

	  switch( msg->Class )
	  {
	    case IDCMP_CLOSEWINDOW:
	      amiga_quit = TRUE;
	      break;

            case IDCMP_ACTIVEWINDOW:
#ifdef __amigaos4__
              if (AWCODE_INTERIM == msg->Code)
              {
                 break;
              }
#endif
              // Make sure the cursor colors are correct

//              GetRGB32( amiga_window->WScreen->ViewPort.ColorMap,
//                        16 + 1, 3, amiga_cursor_colors );

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 1,
                        0x00000000, 0x00000000, 0x00000000 );

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 3,
                        0xffffffff, 0xffffffff, 0xffffffff );

	      if( amiga_last_cursor != NULL )
	      {
		ui_set_cursor( amiga_last_cursor );
	      }
	      
              break;

            case IDCMP_INACTIVEWINDOW:
#ifdef __amigaos4__
              if (AWCODE_INTERIM == msg->Code)
              {
                 break;
              }
#endif
              // Restore cursor colors

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 1,
                        amiga_cursor_colors[ 0 ],
                        amiga_cursor_colors[ 1 ],
                        amiga_cursor_colors[ 2 ] );

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 3,
                        amiga_cursor_colors[ 6 ],
                        amiga_cursor_colors[ 7 ],
                        amiga_cursor_colors[ 8 ] );

	      ui_set_null_cursor();
              break;

            case IDCMP_NEWSIZE:
              if( ( amiga_window->Flags & WFLG_ZOOMED ) == 0 )
              {
		amiga_restore_window();
              }
              break;

            case IDCMP_SIZEVERIFY:
              if( ( amiga_window->Flags & WFLG_ZOOMED ) == 0 )
              {
		amiga_backup_window();
              }
              break;

	    case IDCMP_GADGETUP:
	    {
	      struct Gadget* g = (struct Gadget*) msg->IAddress;

#if defined(__MORPHOS__) || defined(__amigaos4__)
	      if( g->GadgetID == GADID_ICONIFY &&
		  amiga_app_icon == NULL )
	      {
		amiga_icon->do_Type = 0;

		amiga_app_icon = AddAppIconA( 0, 0, g_title,
					      amiga_wb_port, 0, amiga_icon, NULL );

		if( amiga_app_icon != NULL )
		{
		  HideWindow( amiga_window );
		}
	      }
#endif
	      break;
	    }
            
#ifdef __amigaos4__
	    case IDCMP_EXTENDEDMOUSE:
	    {
	       struct IntuiWheelData *wd = (struct IntuiWheelData *)msg->IAddress;

	       if (wd && wd->WheelY)
	       {
		  rdp_send_input( ev_time, RDP_INPUT_MOUSE,
				  wd->WheelY < 0 ? MOUSE_FLAG_BUTTON4 | MOUSE_FLAG_DOWN : MOUSE_FLAG_BUTTON5 | MOUSE_FLAG_DOWN,
                                  msg->MouseX - amiga_window->BorderLeft,
                                  msg->MouseY - amiga_window->BorderTop );
	       }

	       break;
	    }
#endif

	    case IDCMP_RAWKEY:
	    {
	      int  scancode;
	      int  flag;
	      BOOL numlock = amiga_numlock;
	      BOOL capslock;

#if defined(HAVE_NEWMOUSE_H) || defined(HAVE_DEVICES_NEWMOUSE_H)
	      int button = 0;

	      switch (msg->Code & ~0x80) {
#if defined(HAVE_DEVICES_RAWKEYCODES_H)
		case RAWKEY_NM_WHEEL_UP:
#elif defined(HAVE_NEWMOUSE_H)
		case NM_WHEEL_UP:
#endif
		  button = MOUSE_FLAG_BUTTON4 | MOUSE_FLAG_DOWN;
		  break;

#if defined(HAVE_DEVICES_RAWKEYCODES_H)
		case RAWKEY_NM_WHEEL_DOWN:
#elif defined(HAVE_NEWMOUSE_H)
		case NM_WHEEL_DOWN:
#endif
		  button = MOUSE_FLAG_BUTTON5 | MOUSE_FLAG_DOWN;
		  break;
	      }

	      if (button != 0) {
		rdp_send_input( ev_time, RDP_INPUT_MOUSE,
				button,
                                msg->MouseX - amiga_window->BorderLeft,
                                msg->MouseY - amiga_window->BorderTop );
		break;
	      }
#endif


	      // RAmiga-Q quits.
	      ie.ie_Code = msg->Code;
	      ie.ie_Qualifier = msg->Qualifier;
	      /* recover dead key codes & qualifiers */
	      ie.ie_EventAddress = (APTR*) *((ULONG*) msg->IAddress);

	      q_buffer[0] = 0;
	      if( MapRawKey(&ie, q_buffer, sizeof (q_buffer), NULL) == 1 &&
		  tolower(q_buffer[0]) == 'q' &&
		  (msg->Qualifier & (IEQUALIFIER_LSHIFT |
				     IEQUALIFIER_RSHIFT |
				     IEQUALIFIER_CAPSLOCK |
				     IEQUALIFIER_CONTROL |
				     IEQUALIFIER_LALT |
				     IEQUALIFIER_RALT |
				     IEQUALIFIER_LCOMMAND |
				     IEQUALIFIER_RCOMMAND |
				     IEQUALIFIER_NUMERICPAD))
		  == IEQUALIFIER_RCOMMAND) {
		amiga_quit = TRUE;
		break;
	      }
	      
              if( msg->Code & 0x80 )
              {
                flag = KBD_FLAG_UP;
              }
              else
              {
                flag = KBD_FLAG_DOWN;
              }

	      scancode = amiga_translate_key( msg->Code & ~0x80,
					      msg->Qualifier,
					      &numlock );

              if( scancode == 0 )
                break;

	      // Handle NUM LOCK
	      if (scancode == 0x45) {
		if (amiga_is_os4) {
		  numlock = (flag == KBD_FLAG_DOWN);
		}
		else if (flag & KBD_FLAG_UP) {
		  numlock = !numlock;
		}
	      }

	      // Handle CAPS LOCK
	      capslock = (msg->Qualifier & IEQUALIFIER_CAPSLOCK) != 0;

	      // Handle SCROLL LOCK
	      if (scancode == 0x46 && (flag & KBD_FLAG_UP)) {
		amiga_scrolllock = !amiga_scrolllock;
	      }

	      // Sync NumLock/CapsLock
	      if (numlock != amiga_numlock ||
		  capslock != amiga_capslock) {
		amiga_numlock = numlock;
		amiga_capslock = capslock;

		rdp_send_input(0, RDP_INPUT_SYNCHRONIZE, 0,
			       ui_get_numlock_state(read_keyboard_state()), 0);
	      }

	      // These are syncronized; don't send codes
              if (scancode == 0x3a || scancode == 0x45) {
		break;
	      }
	      	      
              if( scancode & 0x80 )
              {
                flag     |= KBD_FLAG_EXT;
                scancode &= ~0x80;
              }

	      rdp_send_input( ev_time, RDP_INPUT_SCANCODE, flag,
			      scancode, 0);

	      break;
	    }

	    case IDCMP_MOUSEBUTTONS:
	    {
	      int button = 0;
    	        
	      switch( msg->Code )
	      {
		case SELECTDOWN:
		  button = MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN;
		  break;

		case MENUDOWN:
		  button = MOUSE_FLAG_BUTTON2 | MOUSE_FLAG_DOWN;
		  break;

		case MIDDLEDOWN:
		  button = MOUSE_FLAG_BUTTON3 | MOUSE_FLAG_DOWN;
		  break;

		case SELECTUP:
		  button = MOUSE_FLAG_BUTTON1;
		  break;

		case MENUUP:
		  button = MOUSE_FLAG_BUTTON2;
		  break;

		case MIDDLEUP:
		  button = MOUSE_FLAG_BUTTON3;
		  break;
    	            
                default:
                  error( "ui_select: Unknown button code: %d\n", msg->Code );
                  break;
	      }

              if( button != 0 )
              {
		rdp_send_input( ev_time, RDP_INPUT_MOUSE,
				button,
                                msg->MouseX - amiga_window->BorderLeft,
                                msg->MouseY - amiga_window->BorderTop );
              }

	      break;
	    }
	          
	    case IDCMP_MOUSEMOVE:
	    {
              rdp_send_input( ev_time, RDP_INPUT_MOUSE,
			      MOUSE_FLAG_MOVE,
                              msg->MouseX - amiga_window->BorderLeft,
                              msg->MouseY - amiga_window->BorderTop );
	      amiga_connection_bar_set_mouse( msg->MouseX, msg->MouseY );
	      break;
            }

	    case IDCMP_INTUITICKS:
	    {
	      static int ticks = 0;
	      static int oldX = 10000000;
	      static int oldY = 10000000;

	      if (++ticks >= 10)
	      {
	        ticks = 0;
	        if (oldX != msg->MouseX - amiga_window->BorderLeft
	         || oldY != msg->MouseY - amiga_window->BorderTop)
	        {
	          oldX = msg->MouseX - amiga_window->BorderLeft;
	          oldY = msg->MouseY - amiga_window->BorderTop;

                  rdp_send_input( ev_time, RDP_INPUT_MOUSE, 
			          MOUSE_FLAG_MOVE, oldX, oldY);
                }
	      }

	      amiga_connection_bar_set_mouse( msg->MouseX, msg->MouseY );
	    }
	    break;

	    default:
	      error( "Unexpected IDCMP: %d\n", (int) msg->Class );
	      break;
          }

	  ReplyMsg( (struct Message*) msg );
	}
      }

      amiga_connection_bar_handle_events(mask);

      if( res > 0 )
      {
	rdpdr_check_fds(&rfds, &wfds, (Bool) False);

	if (FD_ISSET(rdp_socket, &rfds))
	{
	  return True;
	}
      }
  }
	
  return False;
}


void
ui_move_pointer(int x, int y)
{
  // TODO: Move pointer
}


unsigned int
read_keyboard_state()
{
  return amiga_last_qualifier;
}


uint16
ui_get_numlock_state(unsigned int amiga_qualifiers)
{
  uint16 state = 0;

  if (amiga_numlock) {
    state |= KBD_FLAG_NUMLOCK;
  }

  if (amiga_qualifiers & IEQUALIFIER_CAPSLOCK) {
    state |= KBD_FLAG_CAPITAL;
  }
  
  if (amiga_scrolllock) {
    state |= KBD_FLAG_SCROLL;
  }

  return state;
}


HBITMAP
ui_create_bitmap(int width, int height, uint8 *data)
{
  struct RastPort* rp;
  struct BitMap*   bmp;

  rp  = xmalloc( sizeof( *rp ) );
  
  if( rp != NULL )
  {
    InitRastPort( rp );
    
    bmp = AllocBitMap( width, height, amiga_bpp,
		       BMF_MINPLANES, amiga_window->RPort->BitMap );
    
    if( bmp != NULL )
    {
      rp->BitMap = bmp;

      amiga_write_pixels( rp, 0, 0, width, height, data, width );
    }
    else
    {
      xfree( rp );
      rp = NULL;
    }
  }

  return (HBITMAP) rp;
}

void
ui_paint_bitmap(int x, int y, int cx, int cy,
		int width, int height, uint8 *data)
{
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  amiga_write_pixels( amiga_window->RPort, x, y, cx, cy, data, width );
}

void
ui_destroy_bitmap(HBITMAP bmp)
{
  struct RastPort* rp = (struct RastPort*) bmp;

  if( rp != NULL )
  {
    WaitBlit();
    FreeBitMap( rp->BitMap );
    xfree( rp );
  }
}


HGLYPH
ui_create_glyph(int width, int height, uint8 *data)
{
  int           bytesperrow = ( width + 7 ) / 8;
  int           wordsperrow = ( width + 15 ) / 16;
  int           x;
  int           y;
  uint8*        p;
  struct Glyph* glyph;

  glyph = (struct Glyph*) xmalloc( sizeof( struct Glyph) + wordsperrow * 2 * height );

  glyph->BytesPerRow = wordsperrow * 2;

  p = glyph->Data;

  for( y = 0; y < height; ++y )
  {
    for( x = 0; x < bytesperrow; ++x )
    {
      *p = *data;
      ++p;
      ++data;
    }
    
    if( bytesperrow & 1 )
    {
      *p = 0;
      ++p;
    }
  }
  
  return (HGLYPH) glyph;
}


void
ui_destroy_glyph(HGLYPH glyph)
{
  WaitBlit();
  xfree( glyph );
}


HCURSOR
ui_create_cursor(unsigned int x, unsigned int y, int width,
		 int height, uint8 *andmask, uint8 *data)
{
  struct Cursor* c;

  c = (struct Cursor*) xmalloc( sizeof( *c ) );
  
  if( c != NULL )
  {
    int bytesperrow = ( width + 7 ) / 8;
    int wordsperrow = ( width + 15 ) / 16;

    c->Planes = xmalloc( wordsperrow * 2 * height * 2 );
    
    if( c->Planes != NULL )
    {
      int cx;
      int cy;

      uint8* p1 = c->Planes;
      uint8* p2 = c->Planes + wordsperrow * 2 * height;

      andmask += bytesperrow * height;
      data    += width * height * 3;

      for( cy = height - 1; cy >= 0; --cy )
      {
        uint8* col;

        andmask -= bytesperrow;
        data    -= width * 3;

        col = data;

        for( cx = 0; cx < bytesperrow; ++cx )
        {
          int    b;

          *p1 = ~andmask[ cx ];
          *p2 = 0;

          for( b = 128; b != 0; b >>= 1 )
          {
            if( col[ 0 ] >= 0x80 || col[ 1 ] >= 0x80 || col[ 2 ] >= 0x80 )
            {
              // The text cursor has only plane 2 set! Weired ...

              if( *p1 & b )
              {
                *p2 |= b;
              }
              else
              {
                *p1 |= b;
              }
            }

            col += 3;
          }

/* #define CCOL(a,b) CCOL2((a)*2+(b)) */
/* #define CCOL2(a) (a)==0 ? " " : (a)==1 ? "+" : (a)==2 ? "=" : "*" */
/* 	  printf("%02x.%02x ", *p1, *p2); */
/* 	  printf( "%s", CCOL(*p1 & 0x80, *p2 & 0x80) ); */
/* 	  printf( "%s", CCOL(*p1 & 0x40, *p2 & 0x40) ); */
/* 	  printf( "%s", CCOL(*p1 & 0x20, *p2 & 0x20) ); */
/* 	  printf( "%s", CCOL(*p1 & 0x10, *p2 & 0x10) ); */
/* 	  printf( "%s", CCOL(*p1 & 0x08, *p2 & 0x08) ); */
/* 	  printf( "%s", CCOL(*p1 & 0x04, *p2 & 0x04) ); */
/* 	  printf( "%s", CCOL(*p1 & 0x02, *p2 & 0x02) ); */
/* 	  printf( "%s", CCOL(*p1 & 0x01, *p2 & 0x01) ); */
          ++p1;
          ++p2;
        }

        if( bytesperrow & 1 )
        {
          *p1 = 0;
          *p2 = 0;
          ++p1;
          ++p2;
        }
/* 	printf("\n"); */
      }

      InitBitMap( &c->BitMap, 2, width, height );

      c->BitMap.Planes[0] = c->Planes;
      c->BitMap.Planes[1] = c->Planes + wordsperrow * 2 * height;

      c->Pointer = NewObject( NULL, POINTERCLASS,
                              POINTERA_BitMap,    (ULONG) &c->BitMap,
                              POINTERA_XOffset,   -x,
                              POINTERA_YOffset,   -y,
                              POINTERA_WordWidth, wordsperrow,
                              POINTERA_XResolution, POINTERXRESN_SCREENRES,
                              POINTERA_YResolution, POINTERYRESN_SCREENRESASPECT,
                              TAG_DONE );
      
      if( c->Pointer != NULL )
      {
        // Alles gut!
      }
      else
      {
        ui_destroy_cursor( c );
        c = NULL;
      }
    }
    else
    {
      ui_destroy_cursor( c );
      c = NULL;
    }
  }

  return (HCURSOR) c;
}


void
ui_set_cursor(HCURSOR cursor)
{
  struct Cursor* c = (struct Cursor*) cursor;

  if( c->Pointer != NULL )
  {
    if (!amiga_broken_cursor) {
      SetWindowPointer( amiga_window,
			WA_Pointer, (ULONG) c->Pointer,
			TAG_DONE );
    }
    amiga_last_cursor = cursor;
  }
}


void
ui_destroy_cursor(HCURSOR cursor)
{
  struct Cursor* c = (struct Cursor*) cursor;

  if( cursor == amiga_last_cursor )
  {
    amiga_last_cursor = NULL;
  }
  
  if( c != NULL )
  {
    if( c->Pointer != NULL )
    {
      DisposeObject(c->Pointer);
    }

    WaitBlit();
    xfree( c->Planes );

    xfree( c );
  }
}

void
ui_set_null_cursor(void)
{
  HCURSOR tmp = amiga_last_cursor;

  ui_set_cursor( amiga_null_cursor );
  amiga_last_cursor = tmp;
}


HCOLOURMAP
ui_create_colourmap(COLOURMAP *colours)
{
  ULONG* map;

  /* Allocate space for WORD, WORD, ncolours RGB ULONG, ULONG */

  map = xmalloc( 2 + 2 + colours->ncolours * 3 * 4 + 4 );
  
  if( map != NULL )
  {
    int c, i;

    i = 0;

    map[ i ] = ( colours->ncolours << 16 ) | 0;
    ++i;
    
    for( c = 0; c < colours->ncolours; ++c )
    {
      uint8 r, g, b;

      r = colours->colours[ c ].red;
      g = colours->colours[ c ].green;
      b = colours->colours[ c ].blue;

      map[ i ] = ( r << 24 ) | ( r << 16 ) | ( r << 8 ) | r;
      ++i;
      map[ i ] = ( g << 24 ) | ( g << 16 ) | ( g << 8 ) | g;
      ++i;
      map[ i ] = ( b << 24 ) | ( b << 16 ) | ( b << 8 ) | b;
      ++i;
    }
    
    map[ i ] = 0;
  }

  return (HCOLOURMAP) map;
}


void
ui_destroy_colourmap(HCOLOURMAP map)
{
  xfree( map );
}

void
ui_set_colourmap(HCOLOURMAP map)
{
  LONG* ptr = (LONG*) map;
  int   n   = *(WORD*) ptr;
  int   i;
  UBYTE remap[ 256 ];

  ++ptr;

  if( n > 256 )
  {
    error( "ui_set_colourmap: Unexpected size: %d\n", n );
    return;
  }

  for( i = 0; i < n; ++i )
  {
    if( amiga_pens[ i ] != -1 )
    {
      ReleasePen( amiga_window->WScreen->ViewPort.ColorMap, amiga_pens[ i ] );
    }
  }

  for( i = 0; i < n; ++i )
  {
    int old_pen = amiga_pens[ i ];

    amiga_pens[ i ] = ObtainBestPen( amiga_window->WScreen->ViewPort.ColorMap,
                                     ptr[ 0 ], ptr[ 1 ], ptr[ 2 ],
                                     OBP_Precision, PRECISION_EXACT,
                                     TAG_DONE );

    if( old_pen != -1 )
    {
      remap[ old_pen ] = amiga_pens[ i ];
    }

    ptr += 3;
  }
  
  if( CyberGfxBase == NULL ||
      GetCyberMapAttr( amiga_window->RPort->BitMap,
                       CYBRMATTR_ISCYBERGFX ) == FALSE ||
      GetCyberMapAttr( amiga_window->RPort->BitMap,
                       CYBRMATTR_PIXFMT ) == PIXFMT_LUT8 )
  {
    struct RastPort temprp;
    UBYTE*          buffer;

    // Pen for LUT may have changed: Fix screen!

    buffer = amiga_get_tmp_buffer( g_width );

    if( buffer != NULL )
    {
      InitRastPort(&temprp);

      temprp.BitMap = AllocBitMap( g_width, 1, amiga_bpp,
				   BMF_MINPLANES, amiga_window->RPort->BitMap );

      if( temprp.BitMap != NULL)
      {
        int x;
        int y;

        for( y = amiga_window->BorderTop;
             y < g_height + amiga_window->BorderTop;
             ++y )
        {
          ReadPixelLine8( amiga_window->RPort,
                          amiga_window->BorderLeft, y,
                          g_width,
                          buffer, &temprp);

          for( x = 0; x < g_width; ++ x )
          {
            buffer[ x ] = remap[ buffer[ x ] ];
          }

          WritePixelLine8( amiga_window->RPort,
                           amiga_window->BorderLeft, y,
                           g_width,
                           buffer, &temprp);
        }

        WaitBlit();
        FreeBitMap(temprp.BitMap);
      }
    }
  }
}


void
ui_set_clip(int x, int y, int cx, int cy)
{
  struct Region* r;
  
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  r = NewRegion();
  
  if( r != NULL )
  {
    struct Rectangle rect;
   
    rect.MinX = x;
    rect.MaxX = x + cx - 1;
    rect.MinY = y;
    rect.MaxY = y + cy - 1;
    
    OrRectRegion( r, &rect );
    
    r = InstallClipRegion( amiga_window->WLayer, r );

    if( r != NULL )
    {
      DisposeRegion( r );
    }
    
  }

  amiga_clipping = TRUE;
}


void
ui_reset_clip()
{
  struct Region* r;

  r = InstallClipRegion( amiga_window->WLayer, NULL );
  
  if( r != NULL )
  {
    DisposeRegion( r );
  }

  amiga_clipping = FALSE;
}


void
ui_bell()
{
  DisplayBeep( amiga_window->WScreen );
}


void
ui_destblt(uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy)
{
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  amiga_blt_rastport( amiga_window->RPort, x, y,
		      amiga_window->RPort, x, y,
		      cx, cy,
		      opcode << 4 );
}


static uint8 hatch_patterns[] = {
#if 0
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,	/* 0 - bsHorizontal */
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,	/* 1 - bsVertical */
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,	/* 2 - bsFDiagonal */
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,	/* 3 - bsBDiagonal */
	0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08,	/* 4 - bsCross */
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81	/* 5 - bsDiagCross */
#else
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,	/* 0 - bsHorizontal */
	0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7,	/* 1 - bsVertical */ 
	0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe,	/* 2 - bsFDiagonal */
	0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f,	/* 3 - bsBDiagonal */
	0xf7, 0xf7, 0xf7, 0x00, 0xf7, 0xf7, 0xf7, 0xf7,	/* 4 - bsCross */    
	0x7e, 0xbd, 0xdb, 0xe7, 0xe7, 0xdb, 0xbd, 0x7e,	/* 5 - bsDiagCross */
#endif
};

void
ui_patblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* brush */ BRUSH *brush, int bgcolour, int fgcolour)
{
  uint8* pattern = brush->pattern;
    
  // TODO: This function is totally fucked up and shout be rewritten
  
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  switch (brush->style)
  {
    case 0:	/* Solid */
      amiga_blt_rastport( amiga_window->RPort, x, y, // Not really used
			  amiga_window->RPort, x, y,
			  cx, cy,
			  ( opcode ^ 3 ) << 4   /* B is always 1 */ );
      break;

    case 2:     /* Hatch */
      pattern = hatch_patterns + brush->pattern[0] * 8;

    case 3:	/* Pattern */
    {
      struct RastPort rp;

      InitRastPort( &rp );
    
      rp.BitMap = amiga_get_tmp_bitmap( cx, cy );
    
      if( rp.BitMap != NULL )
      {
	int h;
	int v;
	LONG bgpen = amiga_obtain_pen( bgcolour );
	LONG fgpen = amiga_obtain_pen( fgcolour );

	for( v = 0; v < cy; ++v )
	{
	  UBYTE mask;
        
	  mask = pattern[ ( v + brush->yorigin ) & 7 ];

	  for( h = 0; h < cx; ++h )
	  {
	    if( ( mask & ( 1 << ( ( h + brush->xorigin ) & 7 ) ) ) == 0 )
	    {
	      amiga_set_abpen_drmd( &rp, fgpen, 0, JAM1 );
	    }
	    else
	    {
	      amiga_set_abpen_drmd( &rp, bgpen, 0, JAM1 );
	    }
    
	    WritePixel( &rp, h, v );
	  }
	}

	amiga_release_pen( bgpen );
	amiga_release_pen( fgpen );

	WorkingBltBitMapRastPort( rp.BitMap, 0, 0,
				  amiga_window->RPort, x, y,
				  cx, cy,
				  opcode << 4 );
      }

      break;
    }

    default:
      unimpl("brush %d\n", brush->style);
  }

}


void
ui_screenblt(uint8 opcode,
	     /* dest */ int x, int y, int cx, int cy,
	     /* src */ int srcx, int srcy)
{
  srcx += amiga_window->BorderLeft;
  srcy += amiga_window->BorderTop;

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  amiga_blt_rastport( amiga_window->RPort, srcx, srcy,
		      amiga_window->RPort, x, y,
		      cx, cy,
		      opcode << 4 );
}


void
ui_memblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, int srcx, int srcy)
{
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  amiga_blt_rastport( (struct RastPort*) src, srcx, srcy,
		      amiga_window->RPort, x, y,
		      cx, cy,
		      opcode << 4 );
}

void
ui_triblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, int srcx, int srcy,
	  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
  /* This is potentially difficult to do in general. Until someone
     comes up with a more efficient way of doing it I am using cases. */

  switch (opcode)
  {
    case 0x69:	/* PDSxxn */
      ui_memblt(ROP2_XOR, x, y, cx, cy, src, srcx, srcy);
      ui_patblt(ROP2_NXOR, x, y, cx, cy, brush, bgcolour, fgcolour);
      break;

    case 0xb8:	/* PSDPxax */
      ui_patblt(ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
      ui_memblt(ROP2_AND, x, y, cx, cy, src, srcx, srcy);
      ui_patblt(ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
      break;

    case 0xc0:	/* PSa */
      ui_memblt(ROP2_COPY, x, y, cx, cy, src, srcx, srcy);
      ui_patblt(ROP2_AND, x, y, cx, cy, brush, bgcolour, fgcolour);
      break;

    default:
      unimpl("triblt 0x%x\n", opcode);
      ui_memblt(ROP2_COPY, x, y, cx, cy, src, srcx, srcy);
  }
}

void
ui_line(uint8 opcode,
	/* dest */ int startx, int starty, int endx, int endy,
	/* pen */ PEN *pen)
{
  LONG amiga_pen = amiga_obtain_pen( pen->colour );

  startx += amiga_window->BorderLeft;
  starty += amiga_window->BorderTop;

  endx   += amiga_window->BorderLeft;
  endy   += amiga_window->BorderTop;

  // TODO: Use opcode, style and width

  amiga_set_abpen_drmd( amiga_window->RPort, amiga_pen, 0, JAM1 );
  Move( amiga_window->RPort, startx, starty );
  Draw( amiga_window->RPort, endx, endy );

  amiga_release_pen( amiga_pen );
}

void
ui_rect(
  /* dest */ int x, int y, int cx, int cy,
  /* brush */ int colour)
{
  LONG pen = amiga_obtain_pen( colour );

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;
  
  amiga_set_abpen_drmd( amiga_window->RPort, pen, 0, JAM1 );
  RectFill( amiga_window->RPort, x, y, x + cx - 1, y + cy - 1 );

  amiga_release_pen( pen );
}

void
ui_draw_glyph(int mixmode,
	      /* dest */ int x, int y, int cx, int cy,
	      /* src */ HGLYPH glyph, int srcx, int srcy, int bgcolour,
	      int fgcolour)
{
  struct Glyph* g = (struct Glyph*) glyph;
  LONG bgpen = amiga_obtain_pen( bgcolour );
  LONG fgpen = amiga_obtain_pen( fgcolour );

  amiga_set_abpen_drmd( amiga_window->RPort, fgpen, bgpen,
		mixmode == MIX_TRANSPARENT ? JAM1 : JAM2 );
  BltTemplate( g->Data + g->BytesPerRow * srcy, srcx, g->BytesPerRow,
	       amiga_window->RPort, x, y, cx, cy );
  
  amiga_release_pen( bgpen );
  amiga_release_pen( fgpen );
}


#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
  {\
    xyoffset = ttext[++idx];\
    if ((xyoffset & 0x80))\
    {\
      if (flags & TEXT2_VERTICAL)\
        y += ttext[idx+1] | (ttext[idx+2] << 8);\
      else\
        x += ttext[idx+1] | (ttext[idx+2] << 8);\
      idx += 2;\
    }\
    else\
    {\
      if (flags & TEXT2_VERTICAL)\
        y += xyoffset;\
      else\
        x += xyoffset;\
    }\
  }\
  if (glyph != NULL)\
  {\
    ui_draw_glyph (mixmode, x + (short) glyph->offset,\
                   y + (short) glyph->baseline,\
                   glyph->width, glyph->height,\
                   glyph->pixmap, 0, 0, bgcolour, fgcolour);\
    if (flags & TEXT2_IMPLICIT_X)\
      x += glyph->width;\
  }\
}

void
ui_draw_text (uint8 font, uint8 flags, uint8 opcode, int mixmode, int x, int y,
	      int clipx, int clipy, int clipcx, int clipcy, int boxx,
	      int boxy, int boxcx, int boxcy, BRUSH * brush, int bgcolour,
	      int fgcolour, uint8 * text, uint8 length)
{
  FONTGLYPH* glyph;
  int i,j, xyoffset;
  DATABLOB* entry;
  LONG bgpen = 0;

  bgpen = amiga_obtain_pen( bgcolour );

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  /* Sometimes, the boxcx value is something really large, like
     32691. This makes XCopyArea fail with Xvnc. The code below
     is a quick fix. */
  if (boxx + boxcx > g_width)
  {
    boxcx = g_width - boxx;
  }
  
  boxx += amiga_window->BorderLeft;
  boxy += amiga_window->BorderTop;

  clipx += amiga_window->BorderLeft;
  clipy += amiga_window->BorderTop;

  amiga_set_abpen_drmd( amiga_window->RPort, bgpen, 0, JAM1 );

  if (boxcx > 1)
  {
    RectFill( amiga_window->RPort,
	      boxx, boxy, boxx + boxcx - 1, boxy + boxcy - 1 );
  }
  else if (mixmode == MIX_OPAQUE)
  {
    RectFill( amiga_window->RPort,
	      clipx, clipy, clipx + clipcx - 1, clipy + clipcy - 1 );
  }

  amiga_release_pen( bgpen );
  
  /* Paint text, character by character */
  for (i = 0; i < length;)
  {
    switch (text[i])
    {
      case 0xff:
	if (i + 2 < length)
	  cache_put_text(text[i + 1], text, text[i + 2]);
	else
	{
	  error("this shouldn't be happening\n");
	  return;
	}
	/* this will move pointer from start to first character after FF command */
	length -= i + 3;
	text = &(text[i + 3]);
	i = 0;
	break;

      case 0xfe:
	entry = cache_get_text(text[i + 1]);
	if (entry != NULL)
	{
	  if ((((uint8 *) (entry->data))[1] ==
	       0) && (!(flags & TEXT2_IMPLICIT_X)))
	  {
	    if (flags & TEXT2_VERTICAL)
	      y += text[i + 2];
	    else
	      x += text[i + 2];
	  }
	  for (j = 0; j < entry->size; j++)
	    DO_GLYPH(((uint8 *) (entry->data)), j);
	}
	if (i + 2 < length)
	  i += 3;
	else
	  i += 2;
	length -= i;
	/* this will move pointer from start to first character after FE command */
	text = &(text[i]);
	i = 0;
	break;

      default:
	DO_GLYPH(text, i);
	i++;
	break;
    }
  }
}


void
ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
  UBYTE* buffer        = NULL;
  int    bytesperpixel = 1;
  int    bytesperrow   = 0;

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  if( CyberGfxBase != NULL &&
      GetCyberMapAttr( amiga_window->RPort->BitMap,
                       CYBRMATTR_ISCYBERGFX ) &&
      GetCyberMapAttr( amiga_window->RPort->BitMap, 
                       CYBRMATTR_PIXFMT ) != PIXFMT_LUT8 )
  {
    bytesperpixel = 4;
  }

  bytesperrow = ( (cx + 15) & ~15 ) * bytesperpixel;

  buffer = amiga_get_tmp_buffer( bytesperrow * cy );

  if( buffer != NULL )
  {
    amiga_read_video_memory( buffer, bytesperrow,
                             x, y, cx, cy,
                             bytesperpixel != 1 );

    offset *= bytesperpixel;
    cache_put_desktop( offset, cx, cy, bytesperrow, bytesperpixel, buffer );
  }
}


void
ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
  UBYTE* buffer;
  int    bytesperpixel = 1;

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  if( CyberGfxBase != NULL &&
      GetCyberMapAttr( amiga_window->RPort->BitMap,
                       CYBRMATTR_ISCYBERGFX ) &&
      GetCyberMapAttr( amiga_window->RPort->BitMap, 
                       CYBRMATTR_PIXFMT ) != PIXFMT_LUT8 )
  {
    bytesperpixel = 4;
  }

  offset *= bytesperpixel;
  buffer = cache_get_desktop( offset, cx, cy, bytesperpixel );

  if( buffer != NULL )
  {
    amiga_write_video_memory( buffer, cx * bytesperpixel,
                              x, y, cx, cy,
                              bytesperpixel != 1 );
  }
}

void
 ui_resize_window(void)
{
}

void
ui_begin_update(void)
{
}

void
ui_end_update(void)
{
}

void 
ui_polygon(uint8 opcode, uint8 fillmode, POINT * point, int npoints,
	   BRUSH * brush, int bgcolour, int fgcolour)
{
}

void 
ui_polyline(uint8 opcode, POINT * points, int npoints, PEN * pen)
{
}

void 
ui_ellipse(uint8 opcode, uint8 fillmode,
	   int x, int y, int cx, int cy,
	   BRUSH * brush, int bgcolour, int fgcolour)
{
}

#include <fcntl.h>
#include <sys/stat.h> // get S_IRWXU etc

/* Create the bitmap cache directory */
Bool
rd_pstcache_mkdir(void)
{
  if ((mkdir("PROGDIR:cache", S_IRWXU) == -1) && errno != EEXIST)
  {
    perror("PROGDIR:cache");
    return False;
  }

  return True;
}

#define MAX_CELL_SIZE		0x1000	/* pixels */
extern int g_pstcache_Bpp;
/* open a file in the .rdesktop directory */
int
rd_open_file(char *filename)
{
  if (!strncmp(filename, "cache/", 6))
  {
    char fn[256];
    BPTR fd;

    sprintf(fn, "PROGDIR:%s", filename);
    fd = Open(fn, MODE_OLDFILE);
    if (!fd)
    {
      fd = Open(fn, MODE_NEWFILE);
      if (fd)
      {
        int idx;
        void *tmp = calloc(1, g_pstcache_Bpp * MAX_CELL_SIZE + sizeof(CELLHEADER));

        if (!tmp)
        {
          Close(fd);
          DeleteFile(fn);
          perror(fn);
          return -1;
        }

        for (idx = 0; idx < BMPCACHE2_NUM_PSTCELLS; idx++) 
        {
          int len = Write(fd, tmp, g_pstcache_Bpp * MAX_CELL_SIZE + sizeof(CELLHEADER));

          if (len != (int)(g_pstcache_Bpp * MAX_CELL_SIZE + sizeof(CELLHEADER)))
          {
            free(tmp);
            Close(fd);
            DeleteFile(fn);
            return -1;
          }
        }

        free(tmp);

        idx = Seek(fd, 0, OFFSET_BEGINNING);
        if (-1 == idx)
        {
          Close(fd);
          DeleteFile(fn);
          perror(fn);
          return -1;
        }
      }
    }

    if (fd == 0)
    {
      perror(fn);
      return -1;
    }

    return fd;
  } else {
    char *home;
    char fn[256];
    int fd;

    home = "ENVARC:";
    sprintf(fn, "%sRDesktop/%s", home, filename);
    fd = open(fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1)
      perror(fn);
    return fd;
  }
}

/* close file */
void
rd_close_file(int fd)
{
  if (fd > 0x400)
  {
    Close(fd);
  } else {
    close(fd);
  }
}

/* read from file*/
int
rd_read_file(int fd, void *ptr, int len)
{
  if (fd > 0x400)
  {
    return Read(fd, ptr, len);
  } else {
    return read(fd, ptr, len);
  }
}

/* write to file */
int
rd_write_file(int fd, void *ptr, int len)
{
  if (fd > 0x400)
  {
    return Write(fd, ptr, len);
  } else {
    return write(fd, ptr, len);
  }
}

/* move file pointer */
int
rd_lseek_file(int fd, int offset)
{
  if (fd > 0x400)
  {
    return Seek(fd, offset, OFFSET_BEGINNING);
  } else {
    return lseek(fd, offset, SEEK_SET);
  }
}

/* do a write lock on a file */
Bool
rd_lock_file(int fd, int start, int len)
{
  if (fd > 0x400)
  {
    int success = ChangeMode(CHANGE_FH, fd, EXCLUSIVE_LOCK);
    if (0 == success)
    {
      return False;
    }
    return True;
  } else {
    return False;
  }
}
