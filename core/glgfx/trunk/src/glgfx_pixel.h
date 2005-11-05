#ifndef ARP2_glgfx_glgfx_pixel_h
#define ARP2_glgfx_glgfx_pixel_h

#include <stdbool.h>
#include <inttypes.h>
#include <glgfx.h>

enum glgfx_pixel_format {
  glgfx_pixel_format_unknown = 0,
  
  glgfx_pixel_format_a4r4g4b4,
  glgfx_pixel_format_r5g6b5,
  glgfx_pixel_format_a1r5g5b5,
//  glgfx_pixel_format_r8g8b8,
  glgfx_pixel_format_a8b8g8r8,
//  glgfx_pixel_format_b8g8r8,
  glgfx_pixel_format_a8r8g8b8,

  glgfx_pixel_format_max
} __attribute__((mode(__pointer__)));

enum glgfx_pixel_attr {
  glgfx_pixel_attr_unknown = glgfx_tag_user,
  
  glgfx_pixel_attr_bytesperpixel,
  glgfx_pixel_attr_bigendian,
  glgfx_pixel_attr_rgb,
  
  glgfx_pixel_attr_redbits,
  glgfx_pixel_attr_greenbits,
  glgfx_pixel_attr_bluebits,
  glgfx_pixel_attr_alphabits,
  
  glgfx_pixel_attr_redshift,
  glgfx_pixel_attr_greenshift,
  glgfx_pixel_attr_blueshift,
  glgfx_pixel_attr_alphashift,

  glgfx_pixel_attr_redmask,
  glgfx_pixel_attr_greenmask,
  glgfx_pixel_attr_bluemask,
  glgfx_pixel_attr_alphamask,
  
  glgfx_pixel_attr_max
} __attribute__((mode(__pointer__)));

enum glgfx_pixel_format glgfx_pixel_getformat_a(struct glgfx_tagitem const* tags);

bool glgfx_pixel_getattr(enum glgfx_pixel_format format,
			 enum glgfx_pixel_attr attr,
			 intptr_t* storage);


#define glgfx_pixel_getformat(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_pixel_getformat_a((struct glgfx_tagitem const*) _tags); })


#endif /* ARP2_glgfx_glgfx_pixel_h */

