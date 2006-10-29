#ifndef ARP2_glgfx_glgfx_pixel_h
#define ARP2_glgfx_glgfx_pixel_h

#include <stdbool.h>
#include <inttypes.h>
#include <glgfx.h>

#ifdef __cplusplus
extern "C" {
#endif

enum glgfx_pixel_format {
  glgfx_pixel_format_unknown = 0, // MUST start at 0!
  
  glgfx_pixel_format_a4r4g4b4,		// BGRA, 1 * UWORD 
  glgfx_pixel_format_r5g6b5,		// BGR,	 1 * UWORD 
  glgfx_pixel_format_a1r5g5b5,		// BGRA, 1 * UWORD 
  glgfx_pixel_format_a8b8g8r8,		// RGBA, 1 * ULONG 
  glgfx_pixel_format_a8r8g8b8,		// BGRA, 1 * ULONG 

  glgfx_pixel_format_r16g16b16a16f,	// RGBA, 4 * HALF 
  glgfx_pixel_format_r32g32b32a32f,	// RGBA, 4 * FLOAT

  glgfx_pixel_format_max
};

enum glgfx_pixel_attr {
  glgfx_pixel_attr_unknown = glgfx_tag_user + 4000,
  
  glgfx_pixel_attr_bytesperpixel,
  glgfx_pixel_attr_bigendian,
  glgfx_pixel_attr_float,
  glgfx_pixel_attr_rgb,
  
  glgfx_pixel_attr_redbits,
  glgfx_pixel_attr_greenbits,
  glgfx_pixel_attr_bluebits,
  glgfx_pixel_attr_alphabits,
  
  glgfx_pixel_attr_redshift,
  glgfx_pixel_attr_greenshift,
  glgfx_pixel_attr_blueshift,
  glgfx_pixel_attr_alphashift,

  glgfx_pixel_attr_redmask,		// These only work for < 4 bpp formats
  glgfx_pixel_attr_greenmask,
  glgfx_pixel_attr_bluemask,
  glgfx_pixel_attr_alphamask,
  
  glgfx_pixel_attr_max
};


typedef uint16_t glgfx_pixel_a4r4g4b4_t;
typedef uint16_t glgfx_pixel_r5g6b5_t;
typedef uint16_t glgfx_pixel_a1r5g6b5_t;
typedef uint32_t glgfx_pixel_a8b8g8r8_t;
typedef uint32_t glgfx_pixel_a8r8g8b8_t;
typedef struct { uint16_t r, g, b, a; } glgfx_pixel_r16g16b16a16f_t;
typedef struct { float r, g, b, a; } glgfx_pixel_r32g32b32a32f_t;

#define glgfx_pixel_create_a4r4g4b4(r, g, b, a) \
  (uint16_t) (((a) << 12) | ((r) << 8) | ((g) << 4) | (b))

#define glgfx_pixel_create_r5g6b5(r, g, b) \
  (uint16_t) (((r) << 11) | ((g) << 5) | (b))

#define glgfx_pixel_create_a1r5g6b5(r, g, b, a) \
  (uint16_t) (((a) << 15) |((r) << 10) | ((g) << 5) | (b))

#define glgfx_pixel_create_a8b8g8r8(r, g, b, a) \
  (uint32_t) (((a) << 24) |((b) << 16) | ((g) << 5) | (r))

#define glgfx_pixel_create_a8r8g8b8(r, g, b, a)	\
  (uint32_t) (((a) << 24) |((r) << 16) | ((g) << 5) | (b))

#define glgfx_pixel_create_r16g16b16a16f(r, g, b, a) \
  ((glgfx_pixel_r16g16b16a16f_t) { glgfx_float2half(r), glgfx_float2half(g), \
                                   glgfx_float2half(b), glgfx_float2half(a) })

#define glgfx_pixel_create_r32g32b32a32f(r, g, b, a) \
  ((glgfx_pixel_r32g32b32a32f_t) { r, g, b, a })

enum glgfx_pixel_format glgfx_pixel_getformat_a(struct glgfx_tagitem const* tags);

bool glgfx_pixel_getattr(enum glgfx_pixel_format format,
			 enum glgfx_pixel_attr attr,
			 intptr_t* storage);

#define glgfx_pixel_getformat(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_pixel_getformat_a((struct glgfx_tagitem const*) (void*) _tags); })


#ifdef __cplusplus
}
#endif

#endif /* ARP2_glgfx_glgfx_pixel_h */
