#ifndef RDESKTOP_AMIGA_SUPPORT_H
#define RDESKTOP_AMIGA_SUPPORT_H

#include <graphics/rastport.h>

void amiga_req(char* prefix, char* txt);
void amiga_remap_pens( UBYTE* from, UBYTE* to, ULONG length );
APTR amiga_get_tmp_buffer( ULONG length );
struct BitMap* amiga_get_tmp_bitmap( int width, int height );
LONG RemappedWriteChunkyPixels(struct RastPort *rp,LONG xstart,LONG ystart,
			       LONG xstop,LONG ystop,UBYTE *array,
			       LONG bytesperrow);
LONG SafeWriteChunkyPixels(struct RastPort *rp,LONG xstart,LONG ystart,
			   LONG xstop,LONG ystop,UBYTE *array,
			   LONG bytesperrow);
LONG amiga_obtain_pen( ULONG color );
void amiga_release_pen( LONG pen );
void amiga_set_abpen_drmd( struct RastPort *rp, ULONG apen, ULONG bpen, ULONG mode );
VOID SoftClipBlit( struct RastPort *srcRP, LONG xSrc, LONG ySrc,
		   struct RastPort *destRP, LONG xDest, LONG yDest,
		   LONG xSize, LONG ySize, ULONG minterm );
VOID SoftBltBitMapRastPort( struct BitMap *srcBitMap, LONG xSrc, LONG ySrc,
			    struct RastPort *destRP, LONG xDest, LONG yDest,
			    LONG xSize, LONG ySize,
			    ULONG minterm );
VOID WorkingClipBlit( struct RastPort *srcRP, LONG xSrc, LONG ySrc,
		      struct RastPort *destRP, LONG xDest, LONG yDest,
		      LONG xSize, LONG ySize,
		      ULONG minterm );
VOID WorkingBltBitMapRastPort( struct BitMap *srcBitMap, LONG xSrc, LONG ySrc,
			       struct RastPort *destRP, LONG xDest, LONG yDest,
			       LONG xSize, LONG ySize,
			       ULONG minterm );
VOID amiga_blt_rastport( struct RastPort *srcRP, LONG xSrc, LONG ySrc,
			 struct RastPort *destRP, LONG xDest, LONG yDest,
			 LONG xSize, LONG ySize,
			 ULONG minterm );
void amiga_write_pixels( struct RastPort* rp,
			 int x, int y, int width, int height,
			 UBYTE* data, int data_width );
void amiga_read_video_memory( void* buffer, int xmod,
			      int x, int y, int cx, int cy,
			      BOOL truecolor );
void amiga_write_video_memory( void* buffer, int xmod,
			       int x, int y, int cx, int cy,
			       BOOL truecolor );
void amiga_backup_window( void );
void amiga_restore_window( void );
int amiga_translate_key( int code, ULONG qualifier, BOOL* numlock );

#endif /* RDESKTOP_AMIGA_SUPPORT_H */
