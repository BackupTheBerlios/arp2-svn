/*
   rdesktop: A Remote Desktop Protocol client.
   User interface services - Amiga
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

#include <sys/time.h>
#include "rdesktop.h"
#include <errno.h>
#include <stdlib.h>

/* Fix includes */
#define BOOL Bool
struct VSPrite;

#include <ix.h>

#include <cybergraphics/cybergraphics.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/modeid.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <intuition/pointerclass.h>
#include <libraries/asl.h>
#include <proto/asl.h>
#include <proto/cybergraphics.h>
#include <proto/exec.h>
#undef GetOutlinePen // I get an annoying warning if this is defined
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>

#undef BOOL

static struct Library*       AslBase       = NULL;
static struct GfxBase*       GfxBase       = NULL;
static struct IntuitionBase* IntuitionBase = NULL;
struct Library*              LayersBase    = NULL;
static struct Library*       CyberGfxBase  = NULL;

extern int  bpp;
extern int  width;
extern int  height;
extern BOOL sendmotion;
extern BOOL fullscreen;
extern int  desktopsize_percent;

static USHORT         amiga_solid_pattern = ~0;

static struct Screen* amiga_screen         = NULL;
static UBYTE*         amiga_backup         = NULL;
static struct Window* amiga_window         = NULL;
static struct Region* amiga_old_region     = NULL;
static ULONG          amiga_cursor_colors[ 3 * 3 ];
static LONG           amiga_pens[ 256 ];
static HCOLOURMAP     amiga_last_colmap    = NULL;
static UWORD          amiga_last_qualifier = 0;

struct Glyph
{
  int           BytesPerRow;
  unsigned char Data[0];
};

struct Cursor
{
  uint8*        Planes;
  struct BitMap Bitmap;
  Object*       Pointer;
};


static void
amiga_remap_pens( UBYTE* from, UBYTE* to, size_t length )
{
  size_t i;

  for( i = 0; i < length; ++i )
  {
    *to = amiga_pens[ *from ];
    ++from;
    ++to;
  }
}


LONG
RemappedWriteChunkyPixels(struct RastPort *rp,LONG xstart,LONG ystart,
   LONG xstop,LONG ystop,UBYTE *array,LONG bytesperrow)
{
   struct RastPort temprp;
   UBYTE *temparray;
   LONG width,height;
   LONG modulo;
   LONG y;
   LONG pixelswritten;

   pixelswritten = 0;
   width = xstop - xstart + 1;
   height = ystop - ystart + 1;

   if(width > 0 && height > 0)
   {
      modulo = (width + 15) & ~15;

      temparray = AllocVec(modulo * height,MEMF_ANY);

      if( temparray != NULL )
      {
         InitRastPort(&temprp);

         temprp.BitMap = AllocBitMap(width,1,8,NULL,rp->BitMap);

         if( temprp.BitMap != NULL)
         {
            if(modulo == bytesperrow)
               amiga_remap_pens(array,temparray,modulo * height);
            else
            {
               for(y = 0 ; y < height ; y++)
                  amiga_remap_pens(&array[bytesperrow * y],&temparray[modulo * y],width);
            }

            pixelswritten = WritePixelArray8(rp,xstart,ystart,
                                             xstop,ystop,
                                             temparray,&temprp);

            WaitBlit();
            FreeBitMap(temprp.BitMap);
         }

         FreeVec(temparray);
      }
   }

   return(pixelswritten);
}


LONG
SafeWriteChunkyPixels(struct RastPort *rp,LONG xstart,LONG ystart,
   LONG xstop,LONG ystop,UBYTE *array,LONG bytesperrow)
{
   struct RastPort temprp;
   UBYTE *temparray;
   LONG width,height;
   LONG modulo;
   LONG y;
   LONG pixelswritten;

   pixelswritten = 0;
   width = xstop - xstart + 1;
   height = ystop - ystart + 1;

   if(width > 0 && height > 0)
   {
      modulo = (width + 15) & ~15;

      temparray = AllocVec(modulo * height,MEMF_ANY);

      if( temparray != NULL )
      {
         InitRastPort(&temprp);

         temprp.BitMap = AllocBitMap(width,1,8,NULL,rp->BitMap);

         if( temprp.BitMap != NULL)
         {
            if(modulo == bytesperrow)
               CopyMem(array,temparray,modulo * height);
            else
            {
               for(y = 0 ; y < height ; y++)
                  CopyMem(&array[bytesperrow * y],&temparray[modulo * y],width);
            }

            pixelswritten = WritePixelArray8(rp,xstart,ystart,
                                             xstop,ystop,
                                             temparray,&temprp);

            WaitBlit();
            FreeBitMap(temprp.BitMap);
         }

         FreeVec(temparray);
      }
   }

   return(pixelswritten);
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

    temprp.BitMap = AllocBitMap(cx,1,8,NULL,amiga_window->RPort->BitMap);

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


static int
amiga_translate_key( int code )
{
  switch( code )
  {
    case 0x45:    // esc
      return 0x01;

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
      
    case 0x41:    // backspace
      return 0x0e;

    case 0x42:    // tab
      return 0x0f;

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

    case 0x44:    // return
      return 0x1c;

    case 0x63:    // ctrl
      return 0x1d;

    case 0x62:    // caps
      return 0x3a;
      
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

    case 0x60:    // lshift
      return 0x2a;

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

    case 0x61:    // rshift
      return 0x36;

    case 0x64:    // lalt
      return 0x38;

    case 0x66:    // lamiga
      return 0x5b | 0x80;             // Map to Windows left GUI key

    case 0x40:    // space
      return 0x39;
      
    case 0x67:    // ramiga
      return 0x5c | 0x80;             // Map to Windows right GUI key
      
    case 0x65:    // ralt
      return 0x38 | 0x80;

    case 0x46:    // del
      return 0x53 | 0x80;
      
    case 0x5f:    // help
      return 0x5d | 0x80;             // Map to Windows App/Menu key
      
    case 0x4c:    // up arrow
      return 0x48 | 0x80;

    case 0x4d:    // down arrow
      return 0x50 | 0x80;

    case 0x4e:    // right arrow
      return 0x4d | 0x80;

    case 0x4f:    // left arrow
      return 0x4b | 0x80;

    case 0x5a:    // kp [ (NumL)
      return 0x45;                    // Map to Num Lock key

    case 0x5b:    // kp ] (ScrL)
      return 0x46;                    // Map to Scroll Lock key

    case 0x5c:    // kp /
      return 0x35 | 0x80;;

    case 0x5d:    // kp *
      return 0x37;
      
    case 0x3d:    // kp 7
    case 0x3e:    // kp 8
    case 0x3f:    // kp 9
      return code + 10;

    case 0x4a:    // kp -
      return 0x4a;
      
    case 0x2d:    // kp 4
    case 0x2e:    // kp 5
    case 0x2f:    // kp 6
      return code + 30;

    case 0x5e:    // kp +
      return 0x4e;

    case 0x1d:    // kp 1
    case 0x1e:    // kp 2
    case 0x1f:    // kp 3
      return code + 50;

    case 0x43:    // kp enter
      return 0x1c | 0x80;
      
    case 0x0f:    // kp 0
      return 0x52;
      
    case 0x3c:    // kp .
      return 0x53;

    default:
      return 0;
  }
}


void 
init_keycodes(int id)
{
  printf( "Initializing keycodes for %d\n", id );
}


BOOL
ui_create_window(char *title)
{
  struct Screen*              pub = NULL;
  struct ScreenModeRequester* req = NULL;
  ULONG                       id  = DEFAULT_MONITOR_ID;
  struct DimensionInfo        di;
  ULONG                       common_window_tags[] =
  {
    WA_ReportMouse,    sendmotion ? TRUE : FALSE,
    WA_Activate,       TRUE,
    WA_RMBTrap,        TRUE,
    WA_ScreenTitle,    (ULONG) title,
    WA_WindowName,     (ULONG) title,
    WA_IDCMP,          IDCMP_CLOSEWINDOW |
                       IDCMP_ACTIVEWINDOW |
                       IDCMP_INACTIVEWINDOW |
                       IDCMP_NEWSIZE |
                       IDCMP_SIZEVERIFY |
                       IDCMP_RAWKEY |
                       IDCMP_MOUSEBUTTONS |
                       (sendmotion ? IDCMP_MOUSEMOVE : 0),
    TAG_DONE
  };

  AslBase = OpenLibrary( AslName, 39 );
  
  if( AslBase == NULL )
  {
    error( "ui_create_window: Unable to open '%s'.\n", AslName );
    return False;
  }

  GfxBase = (struct GfxBase*) OpenLibrary( GRAPHICSNAME, 39 );
  
  if( GfxBase == NULL )
  {
    error( "ui_create_window: Unable to open '%s'.\n", GRAPHICSNAME );
    return False;
  }

  IntuitionBase = (struct IntuitionBase*) OpenLibrary( "intuition.library", 39 );
  
  if( IntuitionBase == NULL )
  {
    error( "ui_create_window: Unable to open '%s'.\n", "intuition.library" );
    return False;
  }

  LayersBase = OpenLibrary( "layers.library", 39 );
  
  if( LayersBase == NULL )
  {
    error( "ui_create_window: Unable to open '%s'.\n", "layers.library" );
    return False;
  }

  CyberGfxBase = OpenLibrary( CYBERGFXNAME, 40 );

  // Don't care if we're unable to open it
  
  memset( amiga_pens, -1, sizeof( amiga_pens ) );

  pub = LockPubScreen( NULL );

  if( pub == NULL )
  {
    error( "ui_create_window: Unable to lock default public screen." );
    return False;
  }


  if( width == -1 )
  {
    width = pub->Width;
  }

  if( height == -1 )
  {
    height = pub->Height;
  }

  if( bpp == -1 )
  {
    bpp = pub->RastPort.BitMap->Depth;
  }

  id = GetVPModeID( &pub->ViewPort );


  if( desktopsize_percent )
  {
    width  = width  * desktopsize_percent / 100;
    height = height * desktopsize_percent / 100;
  }

  width = ( width + 3 ) & ~3;

  if( fullscreen )
  {
    // We don't need it anymore now
    UnlockPubScreen( NULL, pub );

    req = AllocAslRequestTags( ASL_ScreenModeRequest,  
                               ASLSM_TitleText,            (ULONG) title,
                               ASLSM_MinWidth,             640,
                               ASLSM_MinHeight,            400,
                               ASLSM_MaxDepth,             8,
                               ASLSM_InitialDisplayID,     id,
                               ASLSM_InitialDisplayWidth,  width,
                               ASLSM_InitialDisplayHeight, height,
                               ASLSM_InitialDisplayDepth,  bpp,
                               ASLSM_DoWidth,              TRUE,
                               ASLSM_DoHeight,             TRUE,
                               ASLSM_DoDepth,              TRUE,
                               TAG_DONE );

    if( req == NULL )
    {
      error( "ui_create_window: Unable to create screen mode requester.\n" );
      return False;
    }

    if( ! AslRequestTags( req, TAG_DONE ) )
    {
      error( "ui_create_window: Requester canceled.\n" );
      return False;
    }
  
    if( GetDisplayInfoData( NULL, (APTR) &di, sizeof( di ), 
                            DTAG_DIMS, req->sm_DisplayID ) == 0 )
    {
      error( "ui_create_window: Unable to get dimensions of screen.\n" );
      return False;
    }

    id        = req->sm_DisplayID;
    width     = req->sm_DisplayWidth;
    height    = req->sm_DisplayHeight;
    bpp       = req->sm_DisplayDepth;

    FreeAslRequest( req );
    req = NULL;

    amiga_screen = OpenScreenTags( NULL, 
                                   SA_Left,        di.TxtOScan.MinX,
                                   SA_Top,         di.TxtOScan.MinY,
                                   SA_Width,       width,
                                   SA_Height,      height,
                                   SA_Depth,       bpp,
                                   SA_Overscan,    OSCAN_TEXT,
                                   SA_Title,       (ULONG) title,
                                   SA_ShowTitle,   FALSE,
                                   SA_Quiet,       TRUE,
                                   SA_Type,        CUSTOMSCREEN,
                                   SA_DisplayID,   id,
                                   SA_Interleaved, TRUE,
                                   SA_AutoScroll,  TRUE,
                                   SA_MinimizeISG, TRUE,
                                   SA_SharePens,   TRUE,
                                   TAG_DONE );

    if( amiga_screen == NULL )
    {
      error( "ui_create_window: Unable to open screen.\n" );
      return False;
    }

    amiga_window = OpenWindowTags( NULL,
                                   WA_Left,           0,
                                   WA_Top,            0,
                                   WA_Width,          amiga_screen->Width,
                                   WA_Height,         amiga_screen->Height,
                                   WA_CustomScreen,   (ULONG) amiga_screen,
                                   WA_Backdrop,       TRUE,
                                   WA_Borderless,     TRUE,
                                   WA_NoCareRefresh,  TRUE,
                                   WA_SimpleRefresh,  TRUE,
				   WA_BusyPointer,    TRUE,
				   WA_PointerDelay,   TRUE,
                                   TAG_MORE,          (ULONG) common_window_tags,
                                   TAG_DONE  /* Is TAG_MORE jsr or jmp? */ );
  }
  else
  {
    struct IntuiText it =
    {
      0, 0,
      0,
      0, 0,
      pub->Font, title
    };

    WORD zoom[] = { ~0, ~0, 0, pub->WBorTop + pub->Font->ta_YSize + 1 };
    
    zoom[ 2 ] = IntuiTextLength( &it ) + 128;
    
    if( zoom[ 2 ] > pub->Width )
    {
      zoom[ 2 ] = pub->Width;
    }

    amiga_window = OpenWindowTags( NULL,
                                   WA_Title,          (ULONG) title,
                                   WA_DragBar,        TRUE,
                                   WA_DepthGadget,    TRUE,
                                   WA_CloseGadget,    TRUE,
                                   WA_InnerWidth,     width,
                                   WA_InnerHeight,    height,
                                   WA_AutoAdjust,     TRUE,
                                   WA_PubScreen,      (ULONG) pub,
                                   WA_Zoom,           (ULONG) zoom,
                                   WA_SmartRefresh,   TRUE,
                                   TAG_MORE,          (ULONG) common_window_tags,
                                   TAG_DONE  /* Is TAG_MORE jsr or jmp? */ );
  

    // We don't need it anymore now
    UnlockPubScreen( NULL, pub );
  }

  if( amiga_window == NULL )
  {
    error( "ui_create_window: Unable to open window.\n" );
    return False;
  }

  GetRGB32( amiga_window->WScreen->ViewPort.ColorMap,
            16 + 1, 3, amiga_cursor_colors );

  // Update width, height and depth to actual dimensions

  width  = ( amiga_window->Width - 
             amiga_window->BorderLeft - 
             amiga_window->BorderRight ) & ~3;

  height = amiga_window->Height -
           amiga_window->BorderTop -
           amiga_window->BorderBottom;

  bpp    = amiga_window->RPort->BitMap->Depth;

  return True;
}

void
ui_destroy_window()
{
  if( amiga_backup != NULL )
  {
    xfree( amiga_backup );
  }

  if( amiga_window != NULL )
  {
    int i;

    for( i = 0; i < sizeof( amiga_pens ) / sizeof( amiga_pens[ 0 ] ); ++i )
    {
      if( amiga_pens[ i ] != -1 )
      {
        ReleasePen( amiga_window->WScreen->ViewPort.ColorMap, amiga_pens[ i ] );
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

    CloseWindow( amiga_window );
  }


  if( amiga_screen != NULL )
  {
    CloseScreen( amiga_screen );
  }

  CloseLibrary( CyberGfxBase );

  CloseLibrary( AslBase );
  CloseLibrary( (struct Library*) GfxBase );
  CloseLibrary( (struct Library*) IntuitionBase );
  CloseLibrary( LayersBase );
}


BOOL
ui_select(int rdp_socket)
{
  int n = rdp_socket+1;
  fd_set rfds;
  BOOL   quit = FALSE;

  while( !quit )
  {
    int   res;
    ULONG mask;

    FD_ZERO(&rfds);
    FD_SET(rdp_socket, &rfds);

    mask = 1L << amiga_window->UserPort->mp_SigBit;
    res  = ix_select(n, &rfds, NULL, NULL, NULL, &mask );

    if( res == -1 )
    {
      if( mask == ( 1UL << amiga_window->UserPort->mp_SigBit ) )
      {
	struct IntuiMessage* msg;

	while( ( msg = (struct IntuiMessage*) 
		 GetMsg( amiga_window->UserPort ) ) != NULL )
	{
	  uint32 ev_time;

	  ev_time = time(NULL);

	  switch( msg->Class )
	  {
	    case IDCMP_CLOSEWINDOW:
	      quit = TRUE;
	      break;

            case IDCMP_ACTIVEWINDOW:
              // Make sure the cursor colors are correct

//              GetRGB32( amiga_window->WScreen->ViewPort.ColorMap,
//                        16 + 1, 3, amiga_cursor_colors );

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 1,
                        0x00000000, 0x00000000, 0x00000000 );

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 3,
                        0xffffffff, 0xffffffff, 0xffffffff );
              break;

            case IDCMP_INACTIVEWINDOW:
              // Restore cursor colors

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 1,
                        amiga_cursor_colors[ 0 ],
                        amiga_cursor_colors[ 1 ],
                        amiga_cursor_colors[ 2 ] );

              SetRGB32( &amiga_window->WScreen->ViewPort, 16 + 3,
                        amiga_cursor_colors[ 6 ],
                        amiga_cursor_colors[ 7 ],
                        amiga_cursor_colors[ 8 ] );
              break;

            case IDCMP_NEWSIZE:
              if( ( amiga_window->Flags & WFLG_ZOOMED ) == 0 )
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

                bytesperrow = ( (width + 15) & ~15 ) * bytesperpixel;

                if( amiga_backup != NULL )
                {
                  amiga_write_video_memory( amiga_backup, bytesperrow,
                                            amiga_window->BorderLeft,
                                            amiga_window->BorderTop,
                                            width, height,
                                            bytesperpixel != 1 );

                  xfree( amiga_backup );
                  amiga_backup = NULL;
                }
              }
              break;

            case IDCMP_SIZEVERIFY:
              if( ( amiga_window->Flags & WFLG_ZOOMED ) == 0 )
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

                bytesperrow = ( (width + 15) & ~15 ) * bytesperpixel;

                if( amiga_backup == NULL )
                {
                  amiga_backup = xmalloc( bytesperrow * height );
                }

                if( amiga_backup != NULL )
                {
                  amiga_read_video_memory( amiga_backup, bytesperrow,
                                           amiga_window->BorderLeft,
                                           amiga_window->BorderTop,
                                           width, height,
                                           bytesperpixel != 1 );
                }
              }
              
              break;
            
	    case IDCMP_RAWKEY:
	    {
	      int scancode;
	      int flag;

              if( msg->Code & 0x80 )
              {
                flag = KBD_FLAG_UP;
              }
              else
              {
                flag = KBD_FLAG_DOWN;
              }

	      amiga_last_qualifier = msg->Qualifier;
	      scancode = amiga_translate_key( msg->Code & ~0x80 );

              if( scancode == 0 )
                break;
                
              if( scancode == 0x3a )
              {
                // Handle CAPS LOCK
                
		rdp_send_input( ev_time, RDP_INPUT_SCANCODE, 0,
				0x3a, 0);

                flag = KBD_FLAG_UP;
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
	      break;
            }
          }

	  ReplyMsg( (struct Message*) msg );
	}
      }
      else
      {
	error("select: %s\n", strerror(errno));
	return False;
      }
    }
    else if( res != 0 )
    {
      if( amiga_window->MouseX == 0 && amiga_window->MouseY == 0 )
      {
        return False;
      }

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


HBITMAP
ui_create_bitmap(int width, int height, uint8 *data)
{
  struct RastPort* rp;
  struct BitMap*   bmp;

  rp  = xmalloc( sizeof( *rp ) );
  
  if( rp != NULL )
  {
    InitRastPort( rp );
    
    bmp = AllocBitMap( width, height, 8, 0, amiga_window->RPort->BitMap );
    
    if( bmp != NULL )
    {
      rp->BitMap = bmp;
      
      RemappedWriteChunkyPixels( rp, 0, 0, width - 1, height - 1, data, width );
    }
    else
    {
      xfree( rp );
      rp = NULL;
    }
  }

  DEBUG("Amiga: Created bitmap %08x (%dx%d)\n", rp, width, heigth );
  
  return (HBITMAP) rp;
}

void
ui_paint_bitmap(int x, int y, int cx, int cy,
		int width, int height, uint8 *data)
{
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

#if 0
  RemappedWriteChunkyPixels( amiga_window->RPort, 
                             x, y, x + cx - 1, y + cy - 1, 
                             data, width );
#endif
}

void
ui_destroy_bitmap(HBITMAP bmp)
{
  struct RastPort* rp;
  
  rp = (struct RastPort*) bmp;

  WaitBlit();
  FreeBitMap( rp->BitMap );
  xfree( rp );

  DEBUG("Amiga: Destroyed bitmap %08x\n", rp );
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
#if 1
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
#endif  

  DEBUG("Amiga: Created glyph %08x (%dx%d)\n", glyph, width, heigth );
  return (HGLYPH) glyph;
}


void
ui_destroy_glyph(HGLYPH glyph)
{
  xfree( glyph );
  DEBUG("Amiga: Destroyed glyph %08x\n", glyph );
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
      }

      InitBitMap( &c->Bitmap, 2, width, height );

      c->Bitmap.Planes[0] = c->Planes;
      c->Bitmap.Planes[1] = c->Planes + wordsperrow * 2 * height;

      c->Pointer = NewObject( NULL, POINTERCLASS,
                              POINTERA_BitMap,    (ULONG) &c->Bitmap,
                              POINTERA_XOffset,   -x,
                              POINTERA_YOffset,   -y,
                              POINTERA_WordWidth, wordsperrow,
                              POINTERA_XResolution, POINTERXRESN_SCREENRES,
                              POINTERA_YResolution, POINTERXRESN_SCREENRES,
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
    SetWindowPointer( amiga_window,
                      WA_Pointer, (ULONG) c->Pointer,
                      TAG_DONE );
  }
}


void
ui_destroy_cursor(HCURSOR cursor)
{
  struct Cursor* c = (struct Cursor*) cursor;

  if( c->Pointer != NULL )
  {
    DisposeObject(c->Pointer);
  }

  xfree( c->Planes );

  xfree( c );
}



HCOLOURMAP
ui_create_colourmap(COLOURMAP *colours)
{
  ULONG* map;

  /* Allocate space for WORD, WORD, ncolours RGB ULONG, ULONG */

  map = xmalloc( 2 + 2 + colours->ncolours * 3 * 4 + 4 );
#if 0  
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
#endif
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

  amiga_last_colmap = map;
#if 0
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

    buffer = xmalloc( width );

    if( buffer != NULL )
    {
      InitRastPort(&temprp);

      temprp.BitMap = AllocBitMap(width,1,8,NULL,amiga_window->RPort->BitMap);

      if( temprp.BitMap != NULL)
      {
        int x;
        int y;

        for( y = amiga_window->BorderTop;
             y < height + amiga_window->BorderTop;
             ++y )
        {
          ReadPixelLine8( amiga_window->RPort,
                          amiga_window->BorderLeft, y,
                          width,
                          buffer, &temprp);

          for( x = 0; x < width; ++ x )
          {
            buffer[ x ] = remap[ buffer[ x ] ];
          }

          WritePixelLine8( amiga_window->RPort,
                           amiga_window->BorderLeft, y,
                           width,
                           buffer, &temprp);
        }

        WaitBlit();
        FreeBitMap(temprp.BitMap);
      }
      
      xfree( buffer );
    }
  }
#endif
}


void
ui_set_clip(int x, int y, int cx, int cy)
{
  struct Region* r;

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  if( amiga_old_region != NULL )
  {
    ui_reset_clip();
  }

  r = NewRegion();
  
  if( r != NULL )
  {
    struct Rectangle rect;
    
    rect.MinX = x;
    rect.MaxX = x + cx - 1;
    rect.MinY = y;
    rect.MaxY = y + cy - 1;
    
    OrRectRegion( r, &rect );
    
    LockLayer( 0, amiga_window->WLayer );
    amiga_old_region = InstallClipRegion( amiga_window->WLayer, r );
    UnlockLayer( amiga_window->WLayer );
  }
}


void
ui_reset_clip()
{
  struct Region* r;

  LockLayer( 0, amiga_window->WLayer );
  r = InstallClipRegion( amiga_window->WLayer, amiga_old_region );
  UnlockLayer( amiga_window->WLayer );

  if( r != NULL )
  {
    DisposeRegion( r );
  }

  amiga_old_region = NULL;
}


void
ui_bell()
{
  DisplayBeep( amiga_window->WScreen );
}


uint16 ui_get_toggle_keys()
{
  uint16 keys = 0;

  if (amiga_last_qualifier & IEQUALIFIER_CAPSLOCK )
  {
    keys |= KBD_FLAG_CAPITAL;
  }

  return keys;
}

void
ui_destblt(uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy)
{
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;
#if 0
  ClipBlit( amiga_window->RPort, x, y,
            amiga_window->RPort, x, y,
            cx, cy,
            opcode << 4 );
#endif
}


void
ui_patblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* brush */ BRUSH *brush, int bgcolour, int fgcolour)
{
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;
#if 0
  switch (brush->style)
  {
	  case 0:	/* Solid */
      ClipBlit( amiga_window->RPort, x, y, // Not really used
                amiga_window->RPort, x, y,
                cx, cy,
                ( opcode ^ 3 ) << 4   /* B is always 1 */ );
  		break;

 	  case 3:	/* Pattern */
		{
      struct RastPort rp;

      InitRastPort( &rp );
    
      rp.BitMap = AllocBitMap( cx, cy, 8, 0, amiga_window->RPort->BitMap );
    
      if( rp.BitMap != NULL )
      {
        int h;
        int v;

        for( v = 0; v < cy; ++v )
        {
          UBYTE mask;
        
          mask = brush->pattern[ ( v + brush->yorigin ) & 7 ];

          for( h = 0; h < cx; ++h )
          {
            if( mask & ( 1 << ( ( h + brush->xorigin ) & 7 ) ) )
            {
              SetAPen( &rp, amiga_pens[ fgcolour ] );
            }
            else
            {
              SetAPen( &rp, amiga_pens[ bgcolour ] );
            }
    
            WritePixel( &rp, h, v );
          }
        }

    	  BltBitMapRastPort( rp.BitMap, 0, 0,
		                       amiga_window->RPort, x, y,
		                       cx, cy,
		                       opcode << 4 );

        WaitBlit();
        FreeBitMap( rp.BitMap );
      }

//      printf( "Did %d on (%d,%d)-(%d,%d) using brush %08x, fg=%d bg=%d (%03x, %03x)\n",
//              opcode, x, y, x+cx-1, y+cy-1, brush, fgcolour, bgcolour,
//              GetRGB4(amiga_window->WScreen->ViewPort.ColorMap, amiga_pens[fgcolour]),
//              GetRGB4(amiga_window->WScreen->ViewPort.ColorMap, amiga_pens[bgcolour])
//            );

      break;
    }

    default:
      unimpl("brush %d\n", brush->style);
  }
#endif
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
#if 0
  ClipBlit( amiga_window->RPort, srcx, srcy,
            amiga_window->RPort, x, y,
            cx, cy,
            opcode << 4 );
#endif
}


void
ui_memblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, HCOLOURMAP map, int srcx, int srcy)
{
  /* FIXME: use the map!!! */
  if( map != NULL && map != amiga_last_colmap )
    ui_set_colourmap( map );
  
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;
#if 0
  ClipBlit( (struct RastPort*) src, srcx, srcy,
            amiga_window->RPort, x, y,
            cx, cy,
            opcode << 4 );
#endif
}

void
ui_triblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, HCOLOURMAP map, int srcx, int srcy,
	  /* brush */ BRUSH *brush, int bgcolour, int fgcolour)
{
          /* FIXME: use the map!!!*/
          if( map != NULL && map != amiga_last_colmap )
	          ui_set_colourmap( map );
  
	/* This is potentially difficult to do in general. Until someone
	   comes up with a more efficient way of doing it I am using cases. */
#if 0
	switch (opcode)
	{
		case 0x69:	/* PDSxxn */
			ui_memblt(ROP2_XOR, x, y, cx, cy, src, 0, srcx, srcy);
			ui_patblt(ROP2_NXOR, x, y, cx, cy,
				  brush, bgcolour, fgcolour);
			break;

		case 0xb8:	/* PSDPxax */
			ui_patblt(ROP2_XOR, x, y, cx, cy,
				  brush, bgcolour, fgcolour);
			ui_memblt(ROP2_AND, x, y, cx, cy, src, 0, srcx, srcy);
			ui_patblt(ROP2_XOR, x, y, cx, cy,
				  brush, bgcolour, fgcolour);
			break;

		case 0xc0:	/* PSa */
			ui_memblt(ROP2_COPY, x, y, cx, cy, src, 0, srcx, srcy);
			ui_patblt(ROP2_AND, x, y, cx, cy, brush, bgcolour,
				  fgcolour);
			break;

		default:
			unimpl("triblt 0x%x\n", opcode);
			ui_memblt(ROP2_COPY, x, y, cx, cy, src, 0, srcx, srcy);
	}
#endif
//  printf( "Did %d on (%d,%d)-(%d,%d) from bitmap %08x: %d, %d using brush %08x, fg=%d bg=%d\n", 
//          opcode, x, y, x+cx-1, y+cy-1, src, srcx, srcy, brush, fgcolour, bgcolour );
}

void
ui_line(uint8 opcode,
	/* dest */ int startx, int starty, int endx, int endy,
	/* pen */ PEN *pen)
{
  startx += amiga_window->BorderLeft;
  starty += amiga_window->BorderTop;

  endx   += amiga_window->BorderLeft;
  endy   += amiga_window->BorderTop;

  // TODO: Use opcode, style and width
#if 0
  SetAPen( amiga_window->RPort, amiga_pens[ pen->colour ] );
  SetDrMd( amiga_window->RPort, JAM1 );
  Move( amiga_window->RPort, startx, starty );
  Draw( amiga_window->RPort, endx, endy );
#endif
}

void
ui_rect(
	       /* dest */ int x, int y, int cx, int cy,
	       /* brush */ int colour)
{
  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;
#if 0
  SetAPen( amiga_window->RPort, amiga_pens[ colour ] );
  SetDrMd( amiga_window->RPort, JAM1 );
  RectFill( amiga_window->RPort, x, y, x + cx - 1, y + cy - 1 );
#endif
}

void
amiga_draw_glyph(int mixmode,
		 /* dest */ int x, int y, int cx, int cy,
		 /* src */ HGLYPH glyph, int srcx, int srcy, int bgcolour,
		 int fgcolour)
{
  struct Glyph* g = (struct Glyph*) glyph;
#if 0
  SetDrMd( amiga_window->RPort, JAM1 );

  if( mixmode != MIX_TRANSPARENT )
  {
    SetAPen( amiga_window->RPort, amiga_pens[ bgcolour ] );
    RectFill( amiga_window->RPort, x, y, x + cx - 1, y + cy - 1 );
  }

  // TODO: Use srcx, srcy

  SetAPen( amiga_window->RPort, amiga_pens[ fgcolour ] );
  SetAfPt( amiga_window->RPort, &amiga_solid_pattern, 0 );
  BltPattern( amiga_window->RPort, g->Data, 
              x, y, x + cx - 1 , y + cy - 1, 
              g->BytesPerRow );
#endif
}


#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
    {\
      xyoffset = ttext[++idx];\
      if ((xyoffset & 0x80))\
        {\
          if (flags & 0x04) /* vertical text */\
            y += ttext[++idx] | (ttext[++idx] << 8);\
          else\
            x += ttext[++idx] | (ttext[++idx] << 8);\
        }\
      else\
        {\
          if (flags & 0x04) /* vertical text */\
            y += xyoffset;\
          else\
            x += xyoffset;\
        }\
    }\
  if (glyph != NULL)\
    {\
/*    if (flags & 0x04) // vertical text\
       ui_draw_glyph (mixmode, x + (short) glyph->baseline,\
         y + (short) glyph->offset,\
         glyph->width, glyph->height,\
         glyph->pixmap, 0, 0, bgcolour, fgcolour,\
         (HBITMAP) pixmap);\
       else\
*/\
      amiga_draw_glyph (mixmode, x + (short) glyph->offset,\
                     y + (short) glyph->baseline,\
                     glyph->width, glyph->height,\
                     glyph->pixmap, 0, 0, bgcolour, fgcolour);\
      if (flags & TEXT2_IMPLICIT_X)\
        x += glyph->width;\
    }\
}

void
ui_draw_text (uint8 font, uint8 flags, int mixmode, int x, int y,
	      int clipx, int clipy, int clipcx, int clipcy, int boxx,
	      int boxy, int boxcx, int boxcy, int bgcolour,
	      int fgcolour, uint8 * text, uint8 length)
{
  FONTGLYPH* glyph;
  int i,j, xyoffset;
  DATABLOB* entry;

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;

  boxx += amiga_window->BorderLeft;
  boxy += amiga_window->BorderTop;

  clipx += amiga_window->BorderLeft;
  clipy += amiga_window->BorderTop;
#if 0
  SetAPen( amiga_window->RPort, amiga_pens[ bgcolour ] );
  SetDrMd( amiga_window->RPort, JAM1 );

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

  /* Paint text, character by character */

  for(i=0;i<length;)
    {
      switch (text[i])
        {
          case 0xff:
            if (i+2<length)
              cache_put_text (text[i+1], text, text[i+2]);
            else
              {
                error("this shouldn't be happening\n");
                break;
              }
            /* this will move pointer from start to first character after FF command */
            length-=i+3;
            text=&(text[i+3]);
            i=0;
            break;

          case 0xfe:
            entry = cache_get_text (text[i+1]);
            if (entry!=NULL)
              {
                if ((((uint8 *)(entry->data))[1] == 0 ) && (!(flags & TEXT2_IMPLICIT_X)))
                  {
                    if (flags & 0x04) /* vertical text */
                      y += text[i+2];
                    else
                      x += text[i+2];
                  }
                if (i+2<length) 
                  i+=3;
                else
                  i+=2;
                length-=i;
                /* this will move pointer from start to first character after FE command */
                text=&(text[i]);
                i=0;
                for(j = 0; j < entry->size; j++)
                  DO_GLYPH( ((uint8 *)(entry->data)) , j);
              }
            break;

          default:
            DO_GLYPH(text,i);
            i++;
            break;
        }
    }
#endif
}


void
ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
  UBYTE* buffer        = NULL;
  int    bytesperpixel = 1;
  int    bytesperrow   = 0;

  x += amiga_window->BorderLeft;
  y += amiga_window->BorderTop;
#if 0
  if( CyberGfxBase != NULL &&
      GetCyberMapAttr( amiga_window->RPort->BitMap,
                       CYBRMATTR_ISCYBERGFX ) &&
      GetCyberMapAttr( amiga_window->RPort->BitMap, 
                       CYBRMATTR_PIXFMT ) != PIXFMT_LUT8 )
  {
    bytesperpixel = 4;
  }

  bytesperrow = ( (cx + 15) & ~15 ) * bytesperpixel;

  buffer = xmalloc( bytesperrow * cy );

  if( buffer != NULL )
  {
    amiga_read_video_memory( buffer, bytesperrow,
                             x, y, cx, cy,
                             bytesperpixel != 1 );

    offset *= bytesperpixel;
    cache_put_desktop( offset, cx, cy, bytesperrow, bytesperpixel, buffer );
    xfree( buffer );
  }
#endif
}


void
ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
  UBYTE* buffer;
  int    bytesperpixel = 1;
#if 0
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
#endif
}
