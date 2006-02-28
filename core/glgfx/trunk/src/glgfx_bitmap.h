#ifndef arp2_glgfx_glgfx_bitmap_h
#define arp2_glgfx_glgfx_bitmap_h

#include <glgfx.h>
#include <glgfx_pixel.h>
#include <inttypes.h>

struct glgfx_bitmap;

enum glgfx_bitmap_attr {
  glgfx_bitmap_attr_unknown = glgfx_tag_user + 2000,

  glgfx_bitmap_attr_width,
  glgfx_bitmap_attr_height,
  glgfx_bitmap_attr_bits,
  glgfx_bitmap_attr_friend,
  glgfx_bitmap_attr_format,
  glgfx_bitmap_attr_bytesperrow,
  glgfx_bitmap_attr_locked,
  glgfx_bitmap_attr_mapaddr,

  glgfx_bitmap_attr_max
} __attribute__((mode(__pointer__)));


enum glgfx_bitmap_copy_tag {
  glgfx_bitmap_copy_unknown = glgfx_tag_user + 2100,

  glgfx_bitmap_copy_x,
  glgfx_bitmap_copy_y,
  glgfx_bitmap_copy_width,
  glgfx_bitmap_copy_height,

  glgfx_bitmap_copy_format,
  glgfx_bitmap_copy_data,
  glgfx_bitmap_copy_bytesperrow,

  glgfx_bitmap_copy_max
} __attribute__((mode(__pointer__)));


enum glgfx_bitmap_blit_tag {
  glgfx_bitmap_blit_unknown = glgfx_tag_user + 2200,

  glgfx_bitmap_blit_x,
  glgfx_bitmap_blit_y,
  glgfx_bitmap_blit_width,
  glgfx_bitmap_blit_height,

  glgfx_bitmap_blit_src_x,
  glgfx_bitmap_blit_src_y,
  glgfx_bitmap_blit_src_width,
  glgfx_bitmap_blit_src_height,
  glgfx_bitmap_blit_src_bitmap, /* Can be NULL! */
  
  glgfx_bitmap_blit_mod_r, /* Range is 0 - 0x10000 (0.0 - 1.0) */
  glgfx_bitmap_blit_mod_g,
  glgfx_bitmap_blit_mod_b,
  glgfx_bitmap_blit_mod_a,

  glgfx_bitmap_blit_mod_x,
  glgfx_bitmap_blit_mod_y,
  glgfx_bitmap_blit_mod_width,
  glgfx_bitmap_blit_mod_height,
  glgfx_bitmap_blit_mod_bitmap,

  /* MinTerms are currently ignored by the hardware for floating point
     bitmaps. Never specify anyting but 0xc0 (or leave unspecified) for such
     formats. */
  glgfx_bitmap_blit_minterm,


  glgfx_bitmap_blit_max
} __attribute__((mode(__pointer__)));


struct glgfx_bitmap* glgfx_bitmap_create_a(struct glgfx_tagitem const* tags);
void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap);
void* glgfx_bitmap_lock_a(struct glgfx_bitmap* bitmap, bool read, bool write,
			  struct glgfx_tagitem const* tags);
bool glgfx_bitmap_unlock_a(struct glgfx_bitmap* bitmap, 
			   struct glgfx_tagitem const* tags);
bool glgfx_bitmap_write_a(struct glgfx_bitmap* bitmap, 
			  struct glgfx_tagitem const* tags);

bool glgfx_bitmap_getattr(struct glgfx_bitmap* bm,
			  enum glgfx_bitmap_attr attr,
			  intptr_t* storage);

bool glgfx_bitmap_blit_a(struct glgfx_bitmap* bitmap, struct glgfx_tagitem const* tags);


bool glgfx_bitmap_select(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_waitblit(struct glgfx_bitmap* bitmap);


#define glgfx_bitmap_create(tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_create_a((struct glgfx_tagitem const*) _tags); })

#define glgfx_bitmap_write(bitmap, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_write_a((bitmap), (struct glgfx_tagitem const*) _tags); })

#define glgfx_bitmap_lock(bitmap, read, write, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_lock_a((bitmap), (read), (write), (struct glgfx_tagitem const*) _tags); })

#define glgfx_bitmap_unlock(bitmap, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_unlock_a((bitmap), (struct glgfx_tagitem const*) _tags); })

#define glgfx_bitmap_blit(bitmap, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_blit_a((bitmap), (struct glgfx_tagitem const*) _tags); })

#endif /* arp2_glgfx_glgfx_bitmap_h */

