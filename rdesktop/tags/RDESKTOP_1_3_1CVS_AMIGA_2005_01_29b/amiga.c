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

#ifdef __amigaos4__
# include <devices/timer.h>
# include <intuition/gadgetclass.h>
# include <intuition/imageclass.h>
# define ETI_Iconify 1000
#endif

#include "rdesktop.h"

#undef BOOL
#define Bool int

#include <sys/time.h>
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
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/modeid.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <intuition/pointerclass.h>
#include <libraries/asl.h>
#ifdef HAVE_NEWMOUSE_H
# include <newmouse.h>
#endif
#include <workbench/workbench.h>
#include <proto/asl.h>
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

#define TRACEFUNC printf("%s()\n", __func__);

extern int  g_width;
extern int  g_height;
extern Bool g_sendmotion;
extern Bool g_fullscreen;
extern char g_title[];
extern int  g_server_bpp;

#ifdef WITH_RDPSND
extern int amiga_audio_signal;
#endif

int    amiga_left           = 0;
int    amiga_top            = 0;
STRPTR amiga_pubscreen_name = NULL;
ULONG  amiga_screen_id      = DEFAULT_MONITOR_ID;
struct DiskObject* amiga_icon = NULL;

static struct MsgPort* amiga_wb_port  = NULL;
static struct AppIcon* amiga_app_icon = NULL;

static BOOL           amiga_is_morphos         = FALSE;
static BOOL           amiga_is_amithlon        = FALSE;
static BOOL           amiga_is_os4             = FALSE;

static UWORD          amiga_last_qualifier     = 0;
static BOOL           amiga_numlock            = TRUE;  // default state is on
static BOOL           amiga_scrolllock         = FALSE; // default state is off
static BOOL           amiga_capslock           = 0xbad; // -> sync on first key

static HCURSOR        amiga_last_cursor        = NULL;
static HCURSOR        amiga_null_cursor        = NULL;

static BOOL           amiga_broken_cursor      = FALSE;
static BOOL           amiga_broken_blitter     = FALSE;

static struct Screen* amiga_pubscreen          = NULL;
static ULONG          amiga_bpp                = 8;
static struct Screen* amiga_screen             = NULL;
static UBYTE*         amiga_backup             = NULL;
static struct Window* amiga_window             = NULL;
static struct Region* amiga_old_region         = NULL;
static ULONG          amiga_cursor_colors[ 3 * 3 ];
static LONG           amiga_pens[ 256 ];
static APTR           amiga_tmp_buffer         = NULL;
static ULONG          amiga_tmp_buffer_length  = 0;
static struct BitMap* amiga_tmp_bitmap         = NULL;
static BOOL           amiga_clipping           = FALSE;

#ifdef __amigaos4__
struct DrawInfo      *amiga_DrInfo             = NULL;
struct Image         *amiga_IconifyImage       = NULL;
struct Gadget        *amiga_IconifyGadget      = NULL;
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

void __chkabort(void) {}

void
amiga_req(char* prefix, char* txt)
{
  struct EasyStruct es =  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) "RDesktop",
    (STRPTR) "%s: %s",
    "OK"
  };
  ULONG args[] = { (ULONG) prefix, (ULONG) txt };
    
  EasyRequestArgs( amiga_window, &es, NULL, args );
}


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


static APTR
amiga_get_tmp_buffer( ULONG length )
{
  if( length > amiga_tmp_buffer_length )
  {
    FreeVec( amiga_tmp_buffer );
    amiga_tmp_buffer = AllocVec( length, MEMF_ANY );

    if( amiga_tmp_buffer == NULL )
    {
      error( "amiga_get_tmp_buffer: Unable to allocate %d bytes.\n", length );
      amiga_tmp_buffer_length = 0;
    }
    else
    {
      amiga_tmp_buffer_length = length;
    }
  }

  return amiga_tmp_buffer;
}


static struct BitMap*
amiga_get_tmp_bitmap( int width, int height )
{
  if( amiga_tmp_bitmap == NULL ||
      GetBitMapAttr( amiga_tmp_bitmap, BMA_WIDTH ) < width ||
      GetBitMapAttr( amiga_tmp_bitmap, BMA_HEIGHT ) < height )
  {
    WaitBlit();
    FreeBitMap( amiga_tmp_bitmap );

    amiga_tmp_bitmap = AllocBitMap( width, height, amiga_bpp,
				    BMF_MINPLANES,
				    amiga_window->RPort->BitMap );

    if( amiga_tmp_bitmap == NULL )
    {
      error( "amiga_get_tmp_bitmap: Unable to allocate a %dx%d bitmap.\n",
	     width, height );
    }
  }

  return amiga_tmp_bitmap;
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

    temparray = amiga_get_tmp_buffer( modulo * height );

    if( temparray != NULL )
    {
      InitRastPort(&temprp);

      temprp.BitMap = AllocBitMap( width, 1, amiga_bpp,
				   BMF_MINPLANES, rp->BitMap );

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

    temparray = amiga_get_tmp_buffer( modulo * height );

    if( temparray != NULL )
    {
      InitRastPort(&temprp);

      temprp.BitMap = AllocBitMap( width, 1, amiga_bpp,
				   BMF_MINPLANES, rp->BitMap );

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
    }
  }

  return(pixelswritten);
}


static LONG
amiga_obtain_pen( ULONG color )
{
  ULONG r, g, b;

  switch( g_server_bpp )
  {
    case 8:
      return amiga_pens[ color ];

    case 15:
      r = ( color & 0x7c00 ) << 17;
      g = ( color & 0x03e0 ) << 22;
      b = ( color & 0x001f ) << 27;
      break;

    case 16:
      r = ( color & 0xf800 ) << 16;
      g = ( color & 0x07e0 ) << 21;
      b = ( color & 0x001f ) << 27;
      break;

    case 24:
      r = ( color & 0x0000ff ) << 24;
      g = ( color & 0x00ff00 ) << 16;
      b = ( color & 0xff0000 ) << 8;
      break;

    default:
      error( "amiga_obtain_pen: Illegal server bitplane depth.\n" );
      return -1;
  }

  return ObtainBestPen( amiga_window->WScreen->ViewPort.ColorMap,
			r, g, b,
			OBP_Precision, PRECISION_EXACT,
			TAG_DONE );
  
  return ObtainPen( amiga_window->WScreen->ViewPort.ColorMap,
		    -1, r, g, b, 0 );
}

static void
amiga_release_pen( LONG pen )
{
  if( g_server_bpp != 8 )
  {
    ReleasePen( amiga_window->WScreen->ViewPort.ColorMap, pen );
  }
}



VOID
SoftClipBlit( struct RastPort *srcRP, LONG xSrc, LONG ySrc,
	      struct RastPort *destRP, LONG xDest, LONG yDest,
	      LONG xSize, LONG ySize, ULONG minterm )
{
  int x, y;

  ULONG bc = minterm & 0x80 ? 0xffffffff : 0;
  ULONG bC = minterm & 0x40 ? 0xffffffff : 0;
  ULONG Bc = minterm & 0x20 ? 0xffffffff : 0;
  ULONG BC = minterm & 0x10 ? 0xffffffff : 0;
  
  ULONG* src;
  ULONG* dst;

  src = amiga_get_tmp_buffer( sizeof( ULONG ) * xSize * ySize * 2 );

  if( src == NULL )
  {
    return;
  }

  dst = src + xSize * ySize;
  
  ReadPixelArray( src, 0, 0, sizeof( ULONG ) * xSize,
		  srcRP, xSrc, ySrc, xSize, ySize, RECTFMT_ARGB );

  ReadPixelArray( dst, 0, 0, sizeof( ULONG ) * xSize,
		  destRP, xDest, yDest, xSize, ySize, RECTFMT_ARGB );

  for( x = 0; x < xSize * ySize; ++x )
  {
    ULONG b = src[ x ];
    ULONG c = dst[ x ];
      
    ULONG d = ( ( bc & ( b & c ) ) |
		( bC & ( b & ~c ) ) |
		( Bc & ( ~b & c ) ) |
		( BC & ( ~b & ~c ) ) );

    dst[ x ] = d;
  }

  WritePixelArray( dst, 0, 0, sizeof( ULONG ) * xSize,
		   destRP, xDest, yDest, xSize, ySize, RECTFMT_ARGB );
}


VOID
SoftBltBitMapRastPort( struct BitMap *srcBitMap, LONG xSrc, LONG ySrc,
		       struct RastPort *destRP, LONG xDest, LONG yDest,
		       LONG xSize, LONG ySize,
		       ULONG minterm )
{
  struct RastPort rp;

  InitRastPort( &rp );
  rp.BitMap = srcBitMap;

  SoftClipBlit( &rp, xSrc, ySrc, destRP, xDest, yDest, xSize, ySize, minterm );
}


VOID
WorkingClipBlit( struct RastPort *srcRP, LONG xSrc, LONG ySrc,
		 struct RastPort *destRP, LONG xDest, LONG yDest,
		 LONG xSize, LONG ySize,
		 ULONG minterm )
{
  // Neither Picasso96 nor CyberGraphX handle minterm 0x00 and 0xff
  // correctly, so use RectFill() for those instead. :-(

  if( minterm == 0 )
  {
    LONG pen = amiga_obtain_pen( 0x00000000 );
  
    SetABPenDrMd( amiga_window->RPort, pen, 0, JAM1 );
    RectFill( amiga_window->RPort,
	      xDest, yDest,
	      xDest + xSize - 1, yDest + ySize - 1 );

    amiga_release_pen( pen );
  }
  else if( minterm == 0xf0 )
  {
    LONG pen = amiga_obtain_pen( 0xffffffff );
  
    SetABPenDrMd( amiga_window->RPort, pen, 0, JAM1 );
    RectFill( amiga_window->RPort,
	      xDest, yDest,
	      xDest + xSize - 1, yDest + ySize - 1 );

    amiga_release_pen( pen );
  }
  else if( amiga_broken_blitter && minterm != 0xc0 )
  {
    // MinTerms do not work with CyberGraphX

    SoftClipBlit( srcRP, xSrc, ySrc,
		  destRP, xDest, yDest,
		  xSize, ySize,
		  minterm );
  }
  else
  {
    ClipBlit( srcRP, xSrc, ySrc,
	      destRP, xDest, yDest,
	      xSize, ySize,
	      minterm );
  }
}


VOID
WorkingBltBitMapRastPort( struct BitMap *srcBitMap, LONG xSrc, LONG ySrc,
			  struct RastPort *destRP, LONG xDest, LONG yDest,
			  LONG xSize, LONG ySize,
			  ULONG minterm )
{
  // Neither Picasso96 nor CyberGraphX handle minterm 0x00 and 0xff
  // correctly, so use RectFill() for those instead. :-(
  
  if( minterm == 0 )
  {

    LONG pen = amiga_obtain_pen( 0x00000000 );
  
    SetABPenDrMd( amiga_window->RPort, pen, 0, JAM1 );
    RectFill( amiga_window->RPort,
	      xDest, yDest,
	      xDest + xSize - 1, yDest + ySize - 1 );

    amiga_release_pen( pen );
  }
  else if( minterm == 0xf0 )
  {
    LONG pen = amiga_obtain_pen( 0xffffffff );
  
    SetABPenDrMd( amiga_window->RPort, pen, 0, JAM1 );
    RectFill( amiga_window->RPort,
	      xDest, yDest,
	      xDest + xSize - 1, yDest + ySize - 1 );

    amiga_release_pen( pen );
  }
  else if( amiga_broken_blitter && minterm != 0xc0 )
  {
    // MinTerms do not work with CyberGraphX
 
    SoftBltBitMapRastPort( srcBitMap, xSrc, ySrc,
			   destRP, xDest, yDest,
			   xSize, ySize,
			   minterm );
  }
  else
  {
    BltBitMapRastPort( srcBitMap, xSrc, ySrc,
		       destRP, xDest, yDest,
		       xSize, ySize,
		       minterm );
  }
}


VOID
amiga_blt_rastport( struct RastPort *srcRP, LONG xSrc, LONG ySrc,
		    struct RastPort *destRP, LONG xDest, LONG yDest,
		    LONG xSize, LONG ySize,
		    ULONG minterm )
{
  // This function works like ClipBlit, with the exception that
  // the installed clip region does not affect the source rastport.
  //
  // If you have a better idea, mail me!

  
  // Use BltBitMapRastPort when possible
    
  if( g_fullscreen )
  {
    WorkingBltBitMapRastPort( srcRP->BitMap, xSrc, ySrc,
			      destRP, xDest, yDest,
			      xSize, ySize,
			      minterm );
  }
  else
  {
    if( ! amiga_clipping )
    {
      WorkingClipBlit( srcRP, xSrc, ySrc,
		       destRP, xDest, yDest,
		       xSize, ySize,
		       minterm );
    }
    else
    {
      struct RastPort temprp;
    
      InitRastPort( &temprp );

      temprp.BitMap = amiga_get_tmp_bitmap( xSize, ySize );

      if( temprp.BitMap != NULL )
      {
	struct Region*  region;
	  
	region = InstallClipRegion( amiga_window->WLayer, NULL );

	ClipBlit( srcRP, xSrc, ySrc,
		  &temprp, 0, 0, xSize, ySize, 0xc0 );

	InstallClipRegion( amiga_window->WLayer, region );
    
	WorkingBltBitMapRastPort( temprp.BitMap, 0, 0,
				  destRP, xDest, yDest,
				  xSize, ySize,
				  minterm );
      }
    }
  }
}







static void
amiga_write_pixels( struct RastPort* rp,
		    int x, int y, int width, int height,
		    uint8* data, int data_width )
{
  if( g_server_bpp == 8 )
  {
    RemappedWriteChunkyPixels( rp, x, y, x + width - 1, y + height - 1,
			       data, data_width );
  }
  else
  {
    ULONG* argb_data = amiga_get_tmp_buffer( width * height * 4 );
    int xx, yy;
    int i = 0;
    
    switch( g_server_bpp )
    {
      case 15:
      {
	UBYTE* src = (UBYTE*) data;
	ULONG* dst = argb_data;

	for( yy = 0; yy < height; ++yy )
	{
	  for( xx = 0; xx < width * 2; xx += 2 )
	  {
	    UWORD color = ( src[ xx + 1 ] << 8 ) | src[ xx + 0 ];

	    argb_data[ i++ ] = ( ( ( color & 0x7c00 ) << 9 ) |
				 ( ( color & 0x03e0 ) << 6 ) |
				 ( ( color & 0x001f ) << 3 ) );
	  }

	  src += data_width * 2;
	}
	
	break;
      }
 
      case 16:
      {
	UBYTE* src = (UBYTE*) data;

	for( yy = 0; yy < height; ++yy )
	{
	  for( xx = 0; xx < width * 2; xx += 2 )
	  {
	    UWORD color = ( src[ xx + 1 ] << 8 ) | src[ xx + 0 ];

	    argb_data[ i++ ] = ( ( ( color & 0xf800 ) << 8 ) |
				 ( ( color & 0x07e0 ) << 5 ) |
				 ( ( color & 0x001f ) << 3 ) );
	  }

	  src += data_width * 2;
	}
	
	break;
      }

      case 24:
      {
	UBYTE* src = (UBYTE*) data;

	for( yy = 0; yy < height; ++yy )
	{
	  for( xx = 0; xx < width * 3; xx += 3 )
	  {
	    argb_data[ i++ ] = ( ( src[ xx + 2] << 16 ) |
				 ( src[ xx + 1 ] << 8 ) |
				 src[ xx + 0 ] );
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
  // Good URL:
  // http://panda.cs.ndsu.nodak.edu/~achapwes/PICmicro/keyboard/scancodes1.html

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
			  (g_sendmotion ? IDCMP_MOUSEMOVE : 0)),
      TAG_DONE
    };
  

  if( g_fullscreen )
  {
    amiga_screen = OpenScreenTags( NULL,
                                   SA_Width,       g_width,
                                   SA_Height,      g_height,
                                   SA_Depth,       amiga_bpp,
                                   SA_Title,       (ULONG) g_title,
                                   SA_ShowTitle,   FALSE,
                                   SA_Quiet,       TRUE,
                                   SA_Type,        CUSTOMSCREEN,
                                   SA_DisplayID,   amiga_screen_id,
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
                                   TAG_MORE,          (ULONG) common_window_tags );
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

#ifdef __amigaos4__
    if (amiga_window && amiga_icon)
    {
       uint32 size = (amiga_window->WScreen->Flags & SCREENHIRES ? SYSISIZE_MEDRES : SYSISIZE_LOWRES);
       uint32 height = amiga_window->WScreen->Font->ta_YSize + amiga_window->WScreen->WBorTop + 1;
       struct Image *idepth, *izoom;

       amiga_DrInfo = GetScreenDrawInfo(amiga_window->WScreen);

       idepth = (struct Image *)NewObject( NULL, "sysiclass",
                                           SYSIA_Size, size,
                                           SYSIA_DrawInfo, amiga_DrInfo,
                                           SYSIA_Which, DEPTHIMAGE,
                                           IA_Height, height,
                                           TAG_DONE );
       izoom = (struct Image *)NewObject( NULL, "sysiclass",
                                          SYSIA_Size, size,
                                          SYSIA_DrawInfo, amiga_DrInfo,
                                          SYSIA_Which, ZOOMIMAGE,
                                          IA_Height, height,
                                          TAG_DONE );

       amiga_IconifyImage = (struct Image *)NewObject( NULL, "sysiclass",
                                                       SYSIA_Size, size,
                                                       SYSIA_DrawInfo, amiga_DrInfo,
                                                       SYSIA_Which, ICONIFYIMAGE,
                                                       IA_Height, height,
                                                       TAG_DONE );
       if (amiga_IconifyImage && idepth && izoom)
       {
          int32 relright;
          relright  = - amiga_IconifyImage->Width + 2;
          relright -= idepth->Width - 1;
          relright -= izoom->Width - 1;

          amiga_IconifyGadget = (struct Gadget *)NewObject( NULL, "buttongclass",
                                                            GA_Image, amiga_IconifyImage,
                                                            GA_TopBorder, TRUE,
                                                            GA_Titlebar, TRUE,
                                                            GA_RelRight, relright,
                                                            GA_Top, 0,
                                                            GA_ID, ETI_Iconify,
                                                            GA_RelVerify, TRUE,
                                                            TAG_DONE );
       }

       if (idepth) DisposeObject((Object *)idepth);
       if (izoom)  DisposeObject((Object *)izoom);

       if (amiga_IconifyGadget)
       {
          AddGList( amiga_window, amiga_IconifyGadget, 0, 1, NULL);
          RefreshGList( amiga_IconifyGadget, amiga_window, NULL, 1);
       }
    }
#endif
  }

  if( amiga_window == NULL )
  {
    error( "ui_create_window: Unable to open window.\n" );
    return False;
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
  
#ifdef __amigaos4__
   if (amiga_DrInfo)
   {
      FreeScreenDrawInfo(amiga_window->WScreen, amiga_DrInfo);
      amiga_DrInfo = NULL;
   }

   if (amiga_IconifyGadget)
   {
      RemoveGadget(amiga_window, amiga_IconifyGadget);
      DisposeObject((Object *)amiga_IconifyGadget);
      amiga_IconifyGadget = NULL;
   }

   if (amiga_IconifyImage)
   {
       DisposeObject((Object *)amiga_IconifyImage);
       amiga_IconifyImage = NULL;
   }
#endif

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

    CloseWindow( amiga_window );
    amiga_window = NULL;
  }


  if( amiga_screen != NULL )
  {
    CloseScreen( amiga_screen );
    amiga_screen = NULL;
  }

  WaitBlit();
  FreeBitMap( amiga_tmp_bitmap );
}


Bool
ui_select(int rdp_socket)
{
  int n = rdp_socket;
  fd_set rfds, wfds;
  struct timeval tv;
  Bool s_timeout = False;
  static BOOL quit = FALSE;

  while(TRUE)
  {
    int   res;
    ULONG mask;

    if (quit) {
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
      quit = TRUE;
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
	      ActivateWindow( amiga_window );
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

	      if( amiga_last_cursor != NULL )
	      {
		ui_set_cursor( amiga_last_cursor );
	      }
	      
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
	      if( g->GadgetID == ETI_Iconify &&
		  amiga_app_icon == NULL )
	      {
		amiga_icon->do_Type = 0;

		amiga_app_icon = AddAppIconA( 0, 0, g_title,
					      amiga_wb_port, NULL, amiga_icon, NULL );

		if( amiga_app_icon != NULL )
		{
		  HideWindow( amiga_window );
		}
	      }
#endif
		  
	      break;
	    }
            
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

	      printf("code %d %x, qualifier %x\n", msg->Code, msg->Code, msg->Qualifier);


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
		quit = TRUE;
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

	      capslock = (msg->Qualifier & IEQUALIFIER_CAPSLOCK) != 0;

	      // Sync NumLock/CapsLock
	      if (numlock != amiga_numlock ||
		  capslock != amiga_capslock) {
		amiga_numlock = numlock;
		amiga_capslock = capslock;

		rdp_send_input(0, RDP_INPUT_SYNCHRONIZE, 0,
			       ui_get_numlock_state(read_keyboard_state()), 0);
	      }

	      // Handle CAPS LOCK
              if (scancode == 0x3a) {
                // ... by not sending it
		break;
	      }

	      // Handle NUM LOCK
	      if (scancode == 0x45 && (flag & KBD_FLAG_UP)) {
		amiga_numlock = !amiga_numlock;
	      }

	      // Handle SCROLL LOCK
	      if (scancode == 0x46 && (flag & KBD_FLAG_UP)) {
		amiga_scrolllock = !amiga_scrolllock;
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

	    default:
	      error( "Unexpected IDCMP: %d\n", (int) msg->Class );
	      break;
          }

	  ReplyMsg( (struct Message*) msg );
	}
      }

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

  printf ("state is now %04x\n", state);
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
              SetAPen( &rp, fgpen );
            }
            else
            {
              SetAPen( &rp, bgpen );
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

  SetABPenDrMd( amiga_window->RPort, amiga_pen, 0, JAM1 );
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

  SetABPenDrMd( amiga_window->RPort, pen, 0, JAM1 );
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

  SetABPenDrMd( amiga_window->RPort, fgpen, bgpen,
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
ui_draw_text (uint8 font, uint8 flags, int mixmode, int x, int y,
	      int clipx, int clipy, int clipcx, int clipcy, int boxx,
	      int boxy, int boxcx, int boxcy, int bgcolour,
	      int fgcolour, uint8 * text, uint8 length)
{
  FONTGLYPH* glyph;
  int i,j, xyoffset;
  DATABLOB* entry;
  LONG bgpen = amiga_obtain_pen( bgcolour );

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

  SetABPenDrMd( amiga_window->RPort, bgpen, 0, JAM1 );

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
