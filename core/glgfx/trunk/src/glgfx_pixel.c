
#include "glgfx-config.h"
#include <stdlib.h>
#include <byteswap.h>
#include <GL/gl.h>
#include <GL/glext.h> // for RGBA_FLOAT16_ATI, RGBA_FLOAT32_ATI, GL_HALF_FLOAT_ARB

#include "glgfx.h"
#include "glgfx_pixel.h"
#include "glgfx_intern.h"

struct pixel_info formats[glgfx_pixel_format_max] = {
  { 0, 0, 0, 0, false, false, false, 0, 0, 0, 0, 0, 0, 0, 0 },

//  int.format           format     type                           bpp big    rgb   float   rb  rs  gb  gs  bb  bs  ab  as

  { GL_RGBA4,       GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV,  2, false, true, false,   4,  8,  4,  4,  4,  0,  4, 12 },   // glgfx_pixel_a4r4g4b4
  { GL_RGB5,        GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,        2, false, true, false,   5, 11,  6,  5,  5,  0,  0,  0 },   // glgfx_pixel_r5g6b5
  { GL_RGB5_A1,     GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,  2, false, true, false,   5, 10,  5,  5,  5,  0,  1, 15 },   // glgfx_pixel_a1r5g5b5
  { GL_RGBA8,       GL_RGBA, GL_UNSIGNED_BYTE,               4, false, true, false,   8,  0,  8,  8,  8, 16,  8, 24 },   // glgfx_pixel_a8b8g8r8
  { GL_RGBA8,       GL_BGRA, GL_UNSIGNED_BYTE,               4, false, true, false,   8, 16,  8,  8,  8,  0,  8, 24 },   // glgfx_pixel_a8r8g8b8

  { GL_RGBA16F_ARB, GL_RGBA, GL_HALF_FLOAT_ARB,              8, false, true, true,   16, 48, 16, 32, 16, 16, 16,  0 },   // glgfx_pixel_r16g16b16a16f
  { GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT,                      16, false, true, true,   32, 96, 32, 64, 32, 32, 32,  0 },   // glgfx_pixel_r32g32b32a32f
};

static uint64_t swap_mask(enum glgfx_pixel_format format, uint64_t mask) {
#ifdef WORDS_BIGENDIAN
  bool bigendian = true;
#else
  bool bigendian = false;
#endif
  uint64_t new_mask = mask;

  if (bigendian != formats[format].is_bigendian) {
    if (formats[format].size == 2) {
      new_mask = bswap_16(mask);
    }
    else if (formats[format].size == 4) {
      new_mask = bswap_32(mask);
    }
    else if (formats[format].size == 8) {
      new_mask = bswap_64(mask);
    }
    else {
      return 0;
    }
  }

  return new_mask;
}

static uint64_t get_redmask(enum glgfx_pixel_format format) {
  uint64_t res = ((1ULL << formats[format].redbits) - 1) << formats[format].redshift;

  return swap_mask(format, res);
}

static uint64_t get_greenmask(enum glgfx_pixel_format format) {
  uint64_t res = ((1ULL << formats[format].greenbits) - 1) << formats[format].greenshift;

  return swap_mask(format, res);
}

static uint64_t get_bluemask(enum glgfx_pixel_format format) {
  uint64_t res = ((1ULL << formats[format].bluebits) - 1) << formats[format].blueshift;

  return swap_mask(format, res);
}

static uint64_t get_alphamask(enum glgfx_pixel_format format) {
  uint64_t res = ((1ULL << formats[format].alphabits) - 1) << formats[format].alphashift;

  return swap_mask(format, res);
}

enum glgfx_pixel_format glgfx_pixel_getformat_a(struct glgfx_tagitem const* tags) {
  int i;

  for (i = 1; i < glgfx_pixel_format_max; ++i) {
    struct glgfx_tagitem const* taglist = tags;
    struct glgfx_tagitem const* tag;

    while ((tag = glgfx_nexttagitem(&taglist)) != NULL) {
      switch ((enum glgfx_pixel_attr) tag->tag) {
	case glgfx_pixel_attr_bytesperpixel:
	  if (formats[i].size != (size_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_bigendian:
	  if (formats[i].is_bigendian != (bool) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_rgb:
	  if (formats[i].is_rgb != (bool) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_float:
	  if (formats[i].is_float != (bool) tag->data) goto next_mode;
	  break;
  
	case glgfx_pixel_attr_redbits:
	  if (formats[i].redbits != (uint8_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_greenbits:
	  if (formats[i].greenbits != (uint8_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_bluebits:
	  if (formats[i].bluebits != (uint8_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_alphabits:
	  if (formats[i].alphabits != (uint8_t) tag->data) goto next_mode;
	  break;
  
	case glgfx_pixel_attr_redshift:
	  if (formats[i].redshift != (uint8_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_greenshift:
	  if (formats[i].greenshift != (uint8_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_blueshift:
	  if (formats[i].blueshift != (uint8_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_alphashift:
	  if (formats[i].alphashift != (uint8_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_redmask:
	  if (get_redmask(i) != (uint64_t) (uintptr_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_greenmask:
	  if (get_greenmask(i) != (uint64_t) (uintptr_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_bluemask:
	  if (get_bluemask(i) != (uint64_t) (uintptr_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_alphamask:
	  if (get_alphamask(i) != (uint64_t) (uintptr_t) tag->data) goto next_mode;
	  break;

	case glgfx_pixel_attr_unknown:
	case glgfx_pixel_attr_max:
	  /* Make compiler happy */
	  break;
      }
    }

    break;
    
    next_mode:
    continue;
  }

  if (i != glgfx_pixel_format_max) {
    D(BUG("returning format %d\n", i));
    return i;
  }
  else {
    D(BUG("returning format %d\n", glgfx_pixel_format_unknown));
    return glgfx_pixel_format_unknown;
  }
}

bool glgfx_pixel_getattr(enum glgfx_pixel_format format,
			 enum glgfx_pixel_attr attr,
			 intptr_t* storage) {
  if (format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max ||
      storage == NULL ||
      attr <= glgfx_pixel_attr_unknown || attr >= glgfx_pixel_attr_max) {
    return false;
  }

  switch (attr) {
    case glgfx_pixel_attr_bytesperpixel:
      *storage = formats[format].size;
      break;

    case glgfx_pixel_attr_bigendian:
      *storage = formats[format].is_bigendian;
      break;

    case glgfx_pixel_attr_rgb:
      *storage = formats[format].is_rgb;
      break;

    case glgfx_pixel_attr_float:
      *storage = formats[format].is_float;
      break;
  
    case glgfx_pixel_attr_redbits:
      *storage = formats[format].redbits;
      break;

    case glgfx_pixel_attr_greenbits:
      *storage = formats[format].greenbits;
      break;

    case glgfx_pixel_attr_bluebits:
      *storage = formats[format].bluebits;
      break;

    case glgfx_pixel_attr_alphabits:
      *storage = formats[format].alphabits;
      break;
  
    case glgfx_pixel_attr_redshift:
      *storage = formats[format].redshift;
      break;

    case glgfx_pixel_attr_greenshift:
      *storage = formats[format].greenshift;
      break;

    case glgfx_pixel_attr_blueshift:
      *storage = formats[format].blueshift;
      break;

    case glgfx_pixel_attr_alphashift:
      *storage = formats[format].alphashift;
      break;

    case glgfx_pixel_attr_redmask:
      *storage = get_redmask(format);
      break;

    case glgfx_pixel_attr_greenmask:
      *storage = get_greenmask(format);
      break;

    case glgfx_pixel_attr_bluemask:
      *storage = get_bluemask(format);
      break;

    case glgfx_pixel_attr_alphamask:
      *storage = get_alphamask(format);
      break;
      
    default:
      return false;
  }

  return true;
}
