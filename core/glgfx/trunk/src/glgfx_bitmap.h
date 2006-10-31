#ifndef arp2_glgfx_glgfx_bitmap_h
#define arp2_glgfx_glgfx_bitmap_h

#include <glgfx.h>
#include <glgfx_pixel.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct glgfx_bitmap;

enum glgfx_bitmap_attr {
  glgfx_bitmap_attr_unknown = glgfx_tag_user + 2000,

  glgfx_bitmap_attr_width,
  glgfx_bitmap_attr_height,
  glgfx_bitmap_attr_bits,
  glgfx_bitmap_attr_friend,
  glgfx_bitmap_attr_format,
  glgfx_bitmap_attr_glxpixmap,
  glgfx_bitmap_attr_bytesperrow,
  glgfx_bitmap_attr_locked,
  glgfx_bitmap_attr_mapaddr,

  glgfx_bitmap_attr_max
};


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
};


enum glgfx_bitmap_blit_tag {
  glgfx_bitmap_blit_unknown = glgfx_tag_user + 2200,

  glgfx_bitmap_blit_x,		/* Required */
  glgfx_bitmap_blit_y,		/* Required */
  glgfx_bitmap_blit_width,	/* Required */
  glgfx_bitmap_blit_height,	/* Required */

  glgfx_bitmap_blit_src_x,	/* Required */
  glgfx_bitmap_blit_src_y,	/* Required */
  glgfx_bitmap_blit_src_width,	/* Default is same as destination width */
  glgfx_bitmap_blit_src_height,	/* Default is same as destination width */
  glgfx_bitmap_blit_src_bitmap, /* Can also be NULL for a constant (1,1,1,1) bitmap.
				 * Default is same as destination. */
  glgfx_bitmap_blit_src_interpolated, /* Interpolate when reading */

  glgfx_bitmap_blit_mask_x,
  glgfx_bitmap_blit_mask_y,
  glgfx_bitmap_blit_mask_ptr,	/* A bitplane pointer */
  glgfx_bitmap_blit_mask_bytesperrow,

  glgfx_bitmap_blit_mod_r,	/* Range is 0 - 0x10000 - ... (0.0 - 1.0 - ...) */
  glgfx_bitmap_blit_mod_g,
  glgfx_bitmap_blit_mod_b,
  glgfx_bitmap_blit_mod_a,

  glgfx_bitmap_blit_mod_x,	/* Default is 0 */
  glgfx_bitmap_blit_mod_y,	/* Default is 0 */
  glgfx_bitmap_blit_mod_width,	/* Default is mod_bitmap width */
  glgfx_bitmap_blit_mod_height,	/* Default is mod_bitmap height */
  glgfx_bitmap_blit_mod_bitmap, /* Default is NULL, i.e. mod bitmap is disabled.
				 * Must not be the same as destination! */
  glgfx_bitmap_blit_mod_interpolated,

  /* MinTerms are currently ignored by the hardware for floating point
     bitmaps. Never specify anyting but 0xc0 (or leave unspecified) for such
     formats. */
  glgfx_bitmap_blit_minterm,

  glgfx_bitmap_blit_blend_equation,
  glgfx_bitmap_blit_blend_equation_alpha,
  glgfx_bitmap_blit_blend_srcfunc,
  glgfx_bitmap_blit_blend_srcfunc_alpha,
  glgfx_bitmap_blit_blend_dstfunc,
  glgfx_bitmap_blit_blend_dstfunc_alpha,

  glgfx_bitmap_blit_max
};


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


struct glgfx_cliprect* glgfx_bitmap_addcliprect(struct glgfx_bitmap* bitmap,
						int x, int y, int width, int height);
bool glgfx_bitmap_remcliprect(struct glgfx_bitmap* bitmap,
			      struct glgfx_cliprect* cliprect);

int glgfx_bitmap_numcliprects(struct glgfx_bitmap* bitmap);



#define glgfx_bitmap_create(tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_create_a((struct glgfx_tagitem const*) (void*) _tags); })

#define glgfx_bitmap_write(bitmap, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_write_a((bitmap), (struct glgfx_tagitem const*) (void*) _tags); })

#define glgfx_bitmap_lock(bitmap, read, write, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_lock_a((bitmap), (read), (write), (struct glgfx_tagitem const*) (void*) _tags); })

#define glgfx_bitmap_unlock(bitmap, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_unlock_a((bitmap), (struct glgfx_tagitem const*) (void*) _tags); })

#define glgfx_bitmap_blit(bitmap, tag1, ...)	\
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_bitmap_blit_a((bitmap), (struct glgfx_tagitem const*) (void*) _tags); })

#ifdef __cplusplus
}
#endif

#endif /* arp2_glgfx_glgfx_bitmap_h */
