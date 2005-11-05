#ifndef arp2_glgfx_glgfx_bitmap_h
#define arp2_glgfx_glgfx_bitmap_h

#include <glgfx.h>
#include <glgfx_monitor.h>
#include <glgfx_pixel.h>
#include <inttypes.h>

struct glgfx_bitmap;

enum glgfx_bitmap_attr {
  glgfx_bitmap_attr_unknown = glgfx_tag_user,

  glgfx_bitmap_attr_width,
  glgfx_bitmap_attr_height,
  glgfx_bitmap_attr_format,
  glgfx_bitmap_attr_bytesperrow,
  glgfx_bitmap_attr_locked,
  glgfx_bitmap_attr_mapaddr,

  glgfx_bitmap_attr_max
} __attribute__((mode(__pointer__)));


enum glgfx_bitmap_tag {
  glgfx_bitmap_tag_unknown = glgfx_tag_user,

  glgfx_bitmap_tag_width,
  glgfx_bitmap_tag_height,
  glgfx_bitmap_tag_bits,
  glgfx_bitmap_tag_friend,
  glgfx_bitmap_tag_format,
  glgfx_bitmap_tag_x,
  glgfx_bitmap_tag_y,
  glgfx_bitmap_tag_data,
  glgfx_bitmap_tag_bytesperrow,

  glgfx_bitmap_tag_max
} __attribute__((mode(__pointer__)));


struct glgfx_bitmap* glgfx_bitmap_create_a(struct glgfx_monitor* monitor,
					   struct glgfx_tagitem const* tags);
void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_lock(struct glgfx_bitmap* bitmap, bool read, bool write);
bool glgfx_bitmap_unlock(struct glgfx_bitmap* bitmap, 
			 int x, int y, int width, int height);
bool glgfx_bitmap_update_a(struct glgfx_bitmap* bitmap, 
			   struct glgfx_tagitem const* tags);

bool glgfx_bitmap_getattr(struct glgfx_bitmap* bm,
			  enum glgfx_bitmap_attr attr,
			  intptr_t* storage);

bool glgfx_bitmap_select(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_waitblit(struct glgfx_bitmap* bitmap);


#define glgfx_bitmap_create(monitor, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_create_a((monitor), (struct glgfx_tagitem const*) _tags); })

#define glgfx_bitmap_update(bitmap, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_update_a((bitmap), (struct glgfx_tagitem const*) _tags); })

#endif /* arp2_glgfx_glgfx_bitmap_h */

