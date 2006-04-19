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

/* #include <sys/time.h> */
/* #include <ctype.h> */
/* #include <errno.h> */
/* #include <stdlib.h> */

/* #if defined(__amigaos4__) */
/* # include <unistd.h> */
/* # include <time.h> */
/* # include <proto/bsdsocket.h> */
/* #elif defined(__libnix__) */
/* #  include <libnix.h> */
/* #elif defined(__ixemul__) */
/* # include <ix.h> */
/* #else */
/* # error I need OS4, ixemul or libnix! */
/* #endif */

#include <cybergraphx/cybergraphics.h>
/* #include <exec/execbase.h> */
#include <exec/memory.h>
/* #ifdef HAVE_DEVICES_NEWMOUSE_H */
/* # include <devices/newmouse.h> */
/* #endif */
/* #ifdef HAVE_DEVICES_RAWKEYCODES_H */
/* # include <devices/rawkeycodes.h> */
/* #endif */
/* #include <devices/timer.h> */
/* #include <graphics/displayinfo.h> */
/* #include <graphics/gfxbase.h> */
/* #include <graphics/gfxmacros.h> */
/* #include <graphics/modeid.h> */
/* #include <graphics/rastport.h> */
#include <graphics/rpattr.h>
#include <intuition/intuition.h>
/* #include <intuition/gadgetclass.h> */
/* #include <intuition/imageclass.h> */
/* #include <intuition/pointerclass.h> */
/* #include <libraries/asl.h> */
/* #ifdef HAVE_NEWMOUSE_H */
/* # include <newmouse.h> */
/* #endif */
/* #include <workbench/workbench.h> */
/* #include <proto/asl.h> */
/* #include <proto/dos.h> */
#include <proto/cybergraphics.h>
#include <proto/exec.h>
#undef GetOutlinePen // I get an annoying warning if this is defined
#include <proto/graphics.h>
#include <proto/intuition.h>
/* #include <proto/keymap.h> */
#include <proto/layers.h>
/* #include <proto/wb.h> */

/* #ifdef __MORPHOS__ */
/* # include <intuition/extensions.h> */
/* #endif */

/* #include "amiga_clipboard.h" */

extern int  g_width;
extern int  g_height;
extern Bool g_fullscreen;
extern int  g_server_bpp;

static BOOL           amiga_is_os4;
extern BOOL           amiga_broken_blitter;
extern ULONG          amiga_bpp;
extern UBYTE*         amiga_backup;
extern struct Window* amiga_window;
extern LONG           amiga_pens[ 256 ];
extern APTR           amiga_tmp_buffer;
extern ULONG          amiga_tmp_buffer_length;
extern struct BitMap* amiga_tmp_bitmap;
extern BOOL           amiga_clipping;

extern BOOL           amiga_connection_bar_visible;


void
amiga_req(char* prefix, char* txt)
{
  struct EasyStruct es =  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) "RDesktop",
    (STRPTR) "%s: %s",
    "OK|No more requesters",
#ifdef __amigaos4__
    NULL,NULL
#endif
  };
  ULONG args[] = { (ULONG) prefix, (ULONG) txt };

  static int requesters_disabled = FALSE;
    
  if (requesters_disabled)
  {
     fprintf(stderr, "%s: %s\n", prefix, txt);
  } else {
     LONG result = EasyRequestArgs( amiga_window, &es, NULL, args );
     if (0 == result)
     {
        requesters_disabled = TRUE;
     }
  }
}


void
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


APTR
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


struct BitMap*
amiga_get_tmp_bitmap( int width, int height )
{
  if( amiga_tmp_bitmap == NULL ||
      (int) GetBitMapAttr( amiga_tmp_bitmap, BMA_WIDTH ) < width ||
      (int) GetBitMapAttr( amiga_tmp_bitmap, BMA_HEIGHT ) < height )
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


LONG
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

#if defined (__MORPHOS__) || defined (__amigaos4__)
  return (r >> 8) | (g >> 16) | (b >> 24);
#else
  return ObtainBestPen( amiga_window->WScreen->ViewPort.ColorMap,
			r, g, b,
			OBP_Precision, PRECISION_EXACT,
			TAG_DONE );
#endif
}

void
amiga_release_pen( LONG pen )
{
#if !defined (__MORPHOS__) && !defined (__amigaos4__)
  if( g_server_bpp != 8)
  {
    ReleasePen( amiga_window->WScreen->ViewPort.ColorMap, pen );
  }
#endif
}

void
amiga_set_abpen_drmd( struct RastPort *rp, ULONG apen, ULONG bpen, ULONG mode ) {
  if( amiga_bpp > 8 )
  {
#if defined (__MORPHOS__)
    SetRPAttrs( rp, 
		RPTAG_PenMode, FALSE, 
		RPTAG_FgColor, apen, 
		RPTAG_BgColor, bpen, 
		RPTAG_DrMd,    mode, 
		TAG_DONE );
#elif defined (__amigaos4__)
    SetRPAttrs( rp, 
		RPTAG_APenColor, apen, 
		RPTAG_BPenColor, bpen, 
		RPTAG_DrMd,      mode, 
		TAG_DONE );
#else
    SetABPenDrMd( rp, apen, bpen, mode );
#endif
  }
  else 
  {
    SetABPenDrMd( rp, apen, bpen, mode );
  }
}

VOID
SoftClipBlit( struct RastPort *srcRP, LONG xSrc, LONG ySrc,
	      struct RastPort *destRP, LONG xDest, LONG yDest,
	      LONG xSize, LONG ySize, ULONG minterm )
{
  int x;

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
    LONG pen = g_server_bpp == 8 ? 16 + 1 : amiga_obtain_pen( 0x00000000 ); // Use black color of the mouse pointer for 8 bpp
  
    amiga_set_abpen_drmd( amiga_window->RPort, pen, 0, JAM1 );
    RectFill( amiga_window->RPort,
	      xDest, yDest,
	      xDest + xSize - 1, yDest + ySize - 1 );

    amiga_release_pen( pen );
  }
  else if( minterm == 0xf0 )
  {
    LONG pen = g_server_bpp == 8 ? 16 + 3 : amiga_obtain_pen( 0xffffffff ); // Use white color of the mouse pointer for 8 bpp
  
    amiga_set_abpen_drmd( amiga_window->RPort, pen, 0, JAM1 );
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
    LONG pen = g_server_bpp == 8 ? 16 + 1 : amiga_obtain_pen( 0x00000000 ); // Use black color of the mouse pointer for 8 bpp
  
    amiga_set_abpen_drmd( amiga_window->RPort, pen, 0, JAM1 );
    RectFill( amiga_window->RPort,
	      xDest, yDest,
	      xDest + xSize - 1, yDest + ySize - 1 );

    amiga_release_pen( pen );
  }
  else if( minterm == 0xf0 )
  {
    LONG pen = g_server_bpp == 8 ? 16 + 3 : amiga_obtain_pen( 0xffffffff ); // Use white color of the mouse pointer for 8 bpp

    amiga_set_abpen_drmd( amiga_window->RPort, pen, 0, JAM1 );
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
    
  if( g_fullscreen && !amiga_connection_bar_visible )
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







void
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


void
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

void
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


void
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


void
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


int
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
