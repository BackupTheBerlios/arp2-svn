#ifndef arp2_glgfx_glgfx_bitmap_h
#define arp2_glgfx_glgfx_bitmap_h

#include <glgfx.h>
#include <glgfx_monitor.h>
#include <inttypes.h>

struct glgfx_bitmap;

enum glgfx_bitmap_attr {
  glgfx_bitmap_attr_unknown,

  glgfx_bitmap_attr_width,
  glgfx_bitmap_attr_height,
  glgfx_bitmap_attr_format,
  glgfx_bitmap_attr_bytesperrow,
  glgfx_bitmap_attr_locked,
  glgfx_bitmap_attr_mapaddr,

  glgfx_bitmap_attr_max
};

struct glgfx_bitmap* glgfx_bitmap_create(int width, int height, int bits,
					 int flags, struct glgfx_bitmap* friend,
					 int format, struct glgfx_monitor* monitor);
void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_lock(struct glgfx_bitmap* bitmap, bool read, bool write);
bool glgfx_bitmap_unlock(struct glgfx_bitmap* bitmap, int x, int y, int width, int height);
bool glgfx_bitmap_update(struct glgfx_bitmap* bitmap,
			 int x, int y, int width, int height,
			 void* data, enum glgfx_pixel_format format, size_t bytes_per_row);

bool glgfx_bitmap_getattr(struct glgfx_bitmap* bm,
			  enum glgfx_bitmap_attr attr,
			  uint32_t* storage);

bool glgfx_bitmap_select(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_waitblit(struct glgfx_bitmap* bitmap);

#endif /* arp2_glgfx_glgfx_bitmap_h */

