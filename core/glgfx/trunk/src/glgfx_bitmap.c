
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"
#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_intern.h"

static enum glgfx_pixel_format select_format(int bits,
					     struct glgfx_bitmap* friend,
					     enum glgfx_pixel_format format) {
  enum glgfx_pixel_format fmt = friend != NULL ? friend->format : format;

  if (fmt == glgfx_pixel_format_unknown) {
    // I'm not sure about this but well ...
    fmt = glgfx_pixel_getformat(
      glgfx_pixel_attr_rgb,       true,
      glgfx_pixel_attr_redbits,   bits/3,
      glgfx_pixel_attr_greenbits, bits/3,
      glgfx_pixel_attr_bluebits,  bits/3,
      glgfx_tag_end);
  }

  if (fmt <= glgfx_pixel_format_unknown || fmt >= glgfx_pixel_format_max) {
    abort();
  }

  return fmt;
}

static size_t glgfx_texture_size(int width, int height, enum glgfx_pixel_format format) {
  if (format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max ||
    width < 0 || height < 0) {
    BUG("width: %d; height: %d; format: %d\n", width, height, (int) format);
    abort();
  }

  return width * height * formats[format].size;
}


struct glgfx_bitmap* glgfx_bitmap_create_a(struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  struct glgfx_bitmap* bitmap;

  int width = 0, height = 0, bits = 0;
  struct glgfx_bitmap* friend = NULL;
  enum glgfx_pixel_format format = glgfx_pixel_format_unknown;
  
  struct glgfx_tagitem const* tag;

  if (tags == NULL) {
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_attr) tag->tag) {
      
      case glgfx_bitmap_attr_width:
	width = tag->data;
	break;

      case glgfx_bitmap_attr_height:
	height = tag->data;
	break;

      case glgfx_bitmap_attr_bits:
	bits = tag->data;
	break;

      case glgfx_bitmap_attr_friend:
	friend = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_bitmap_attr_format:
	format = tag->data;
	break;

      case glgfx_bitmap_attr_bytesperrow:
      case glgfx_bitmap_attr_locked:
      case glgfx_bitmap_attr_mapaddr:
      case glgfx_bitmap_attr_unknown:
      case glgfx_bitmap_attr_max:
	/* Make compiler happy */
	break;
    }
  }
  
  format = select_format(bits, friend, format);

  if (width <= 0 || height <= 0 || format == glgfx_pixel_format_unknown) {
    return NULL;
  }

  bitmap = calloc(1, sizeof *bitmap);

  if (bitmap == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&glgfx_mutex);
  
  bitmap->width             = width;
  bitmap->height            = height;
  bitmap->bits              = bits;
  bitmap->format            = format;
  bitmap->pbo_size          = glgfx_texture_size(width, height, bitmap->format);
  bitmap->pbo_bytes_per_row = glgfx_texture_size(width, 1, bitmap->format);

  glGenTextures(1, &bitmap->texture);
  GLGFX_CHECKERROR();

  if (context->monitor->have_GL_ARB_texture_rectangle) {
    bitmap->texture_target = GL_TEXTURE_RECTANGLE_ARB;
  }
  else {
    bitmap->texture_target = GL_TEXTURE_2D;
  }

  glgfx_context_bindtex(context, 0, bitmap);

  glTexImage2D(bitmap->texture_target, 0,
	       formats[bitmap->format].internal_format,
	       width, height, 0,
	       formats[bitmap->format].format,
	       formats[bitmap->format].type,
	       NULL);
  if (glGetError() == GL_INVALID_ENUM) {
    // Format not supported by hardware

    glgfx_context_unbindtex(context, 0);
    pthread_mutex_unlock(&glgfx_mutex);
    glgfx_bitmap_destroy(bitmap);
    return NULL;
  }

  GLGFX_CHECKERROR();

  glTexParameteri(bitmap->texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(bitmap->texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  if (bitmap->format == glgfx_pixel_format_r32g32b32a32f) {
    // Currently, there's no hardware support for FP32 filtering.
    // TODO: Detect support for it run-time?
    glTexParameteri(bitmap->texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(bitmap->texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }
  else {
    glTexParameteri(bitmap->texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(bitmap->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  GLGFX_CHECKERROR();
  glgfx_context_unbindtex(context, 0);

  bitmap->has_changed = true;

  pthread_mutex_unlock(&glgfx_mutex);
  return bitmap;
}


void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap) {
  struct glgfx_context* context = glgfx_context_getcurrent();

  if (bitmap == NULL) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);
  glgfx_bitmap_unlock_a(bitmap, NULL);
  if (context->monitor->have_GL_ARB_pixel_buffer_object) {
    glDeleteBuffers(1, &bitmap->pbo);
  }
  else {
    free(bitmap->buffer);
  }
  glDeleteTextures(1, &bitmap->texture);
  free(bitmap);
  pthread_mutex_unlock(&glgfx_mutex);
}


void* glgfx_bitmap_lock_a(struct glgfx_bitmap* bitmap, bool read, bool write,
			  struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  struct glgfx_tagitem const* tag;
  void* res = NULL;

  if (bitmap == NULL || (!read && !write)) {
    return NULL;
  }
  
  pthread_mutex_lock(&glgfx_mutex);

  bitmap->locked_x = 0;
  bitmap->locked_y = 0;
  bitmap->locked_width = bitmap->width;
  bitmap->locked_height = bitmap->height;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_copy_tag) tag->tag) {
      case glgfx_bitmap_copy_x:
	bitmap->locked_x = tag->data;
	break;

      case glgfx_bitmap_copy_y:
	bitmap->locked_y = tag->data;
	break;

      case glgfx_bitmap_copy_width:
	bitmap->locked_width = tag->data;
	break;

      case glgfx_bitmap_copy_height:
	bitmap->locked_height = tag->data;
	break;

      case glgfx_bitmap_copy_data:
      case glgfx_bitmap_copy_format:
      case glgfx_bitmap_copy_bytesperrow:
      case glgfx_bitmap_copy_unknown:
      case glgfx_bitmap_copy_max:
	/* Make compiler happy */
	break;
    }
  }

  if (bitmap->locked_x < 0 || bitmap->locked_width <= 0 ||
      bitmap->locked_x + bitmap->locked_width > bitmap->width ||
      bitmap->locked_y < 0 || bitmap->locked_height <= 0 ||
      bitmap->locked_y + bitmap->locked_height > bitmap->height) {
    pthread_mutex_unlock(&glgfx_mutex);
    return NULL;
  }

  if (read && write) {
    bitmap->locked_usage = GL_STREAM_COPY;
    bitmap->locked_access = GL_READ_WRITE;
  }
  else if (!read && write) {
    bitmap->locked_usage = GL_STREAM_DRAW;
    bitmap->locked_access = GL_WRITE_ONLY;
  }
  else {
    bitmap->locked_usage = GL_STATIC_READ;
    bitmap->locked_access = GL_READ_ONLY;
  }
  
  if (context->monitor->have_GL_ARB_pixel_buffer_object) {
    if (bitmap->pbo == 0) {
      glGenBuffers(1, &bitmap->pbo);
      GLGFX_CHECKERROR();
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      GLGFX_CHECKERROR();
      glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo_size, 
		      NULL, bitmap->locked_usage);
      GLGFX_CHECKERROR();
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }
  }
  else {
    if (bitmap->buffer == NULL) {
      bitmap->buffer = malloc(bitmap->pbo_size);
    }
  }

  bitmap->locked = true;

  // I have no idea why, but on some GeForces, this needs to be done
  // even if we're only writing. Driver 1.0.8756 bug?
  glgfx_context_bindfbo(context, bitmap);

  if (read) {
    // Bind FBO and attach texture
// always done    glgfx_context_bindfbo(context, bitmap); 

    if (context->monitor->have_GL_ARB_pixel_buffer_object) {
      glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, bitmap->pbo);
      GLGFX_CHECKERROR();
      glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->width);
      glReadPixels(bitmap->locked_x, bitmap->locked_y, 
		   bitmap->locked_width, bitmap->locked_height,
		   formats[bitmap->format].format, 
		   formats[bitmap->format].type, 
		   (void*) (bitmap->locked_x * formats[bitmap->format].size +
			    bitmap->locked_y * bitmap->pbo_bytes_per_row));
      glPixelStorei(GL_PACK_ROW_LENGTH, 0);
      GLGFX_CHECKERROR();
      glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }
    else {
      glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->width);
      glReadPixels(bitmap->locked_x, bitmap->locked_y, 
		   bitmap->locked_width, bitmap->locked_height,
		   formats[bitmap->format].format, 
		   formats[bitmap->format].type, 
		   (void*) (bitmap->buffer + 
			    bitmap->locked_x * formats[bitmap->format].size +
			    bitmap->locked_y * bitmap->pbo_bytes_per_row));
      glPixelStorei(GL_PACK_ROW_LENGTH, 0);
      
      GLGFX_CHECKERROR();
    }
  }
  
  if (context->monitor->have_GL_ARB_pixel_buffer_object) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
    bitmap->locked_memory = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB,
					   bitmap->locked_access);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    GLGFX_CHECKERROR();
  }
  else {
    bitmap->locked_memory = bitmap->buffer;
  }

  res = bitmap->locked_memory;

  pthread_mutex_unlock(&glgfx_mutex);

  return res;
}


bool glgfx_bitmap_unlock_a(struct glgfx_bitmap* bitmap, 
			   struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  struct glgfx_tagitem const* tag;
  bool rc = false;

  if (bitmap == NULL || !bitmap->locked) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  int x = bitmap->locked_x;
  int y = bitmap->locked_y;
  int width = bitmap->locked_width;
  int height = bitmap->locked_height;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_copy_tag) tag->tag) {
      case glgfx_bitmap_copy_x:
	x = tag->data;
	break;

      case glgfx_bitmap_copy_y:
	y = tag->data;
	break;

      case glgfx_bitmap_copy_width:
	width = tag->data;
	break;

      case glgfx_bitmap_copy_height:
	height = tag->data;
	break;

      case glgfx_bitmap_copy_data:
      case glgfx_bitmap_copy_format:
      case glgfx_bitmap_copy_bytesperrow:
      case glgfx_bitmap_copy_unknown:
      case glgfx_bitmap_copy_max:
	/* Make compiler happy */
	break;
    }
  }

  if (x < 0 || y < 0 || width < 0 || height < 0 ||
      (x + width) > bitmap->width || (y + height) > bitmap->height) {
    pthread_mutex_unlock(&glgfx_mutex);
    return false;
  }

  if (bitmap->locked_memory != NULL) {
    if (context->monitor->have_GL_ARB_pixel_buffer_object) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB)) {
	rc = true;
      }

      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE ||
	   bitmap->locked_access == GL_WRITE_ONLY)) {
#if 0
	glgfx_context_bindfbo(context, bitmap);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	glWindowPos2i(x, y);
	glDrawPixels(width, height, 
		     formats[bitmap->format].format,
		     formats[bitmap->format].type,
		     (void*) (x * formats[bitmap->format].size +
			      y * bitmap->pbo_bytes_per_row));
	GLGFX_CHECKERROR();
#else
	glgfx_context_bindtex(context, 0, bitmap);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	glTexSubImage2D(bitmap->texture_target, 0,
			x, y, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (x * formats[bitmap->format].size +
				 y * bitmap->pbo_bytes_per_row));
	GLGFX_CHECKERROR();
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	glgfx_context_unbindtex(context, 0);
#endif
      }

      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      GLGFX_CHECKERROR();
    }
    else {
      rc = true;

      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE ||
	   bitmap->locked_access == GL_WRITE_ONLY)) {
	glgfx_context_bindtex(context, 0, bitmap);
	
	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	GLGFX_CHECKERROR();
	glTexSubImage2D(bitmap->texture_target, 0,
			x, y, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (bitmap->buffer +
				 x * formats[bitmap->format].size +
				 y * bitmap->pbo_bytes_per_row));
	GLGFX_CHECKERROR();
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	glgfx_context_unbindtex(context, 0);
      }
    }

    bitmap->locked_memory = NULL;
  }

  bitmap->locked = false;
  bitmap->has_changed = true;
  
  pthread_mutex_unlock(&glgfx_mutex);

  return rc;
}


bool glgfx_bitmap_write_a(struct glgfx_bitmap* bitmap, 
			  struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  int x = 0, y = 0, width = 0, height = 0;
  void* data = NULL;
  enum glgfx_pixel_format format = glgfx_pixel_format_unknown;
  size_t bytes_per_row = 0;

  bool got_bytes_per_row = false;

  struct glgfx_tagitem const* tag;

  if (bitmap == NULL || tags == NULL) {
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_copy_tag) tag->tag) {
      case glgfx_bitmap_copy_x:
	x = tag->data;
	break;

      case glgfx_bitmap_copy_y:
	y = tag->data;
	break;

      case glgfx_bitmap_copy_width:
	width = tag->data;
	break;

      case glgfx_bitmap_copy_height:
	height = tag->data;
	break;

      case glgfx_bitmap_copy_data:
	data = (void*) tag->data;
	break;

      case glgfx_bitmap_copy_format:
	format = tag->data;
	break;

      case glgfx_bitmap_copy_bytesperrow:
	bytes_per_row = tag->data;
	got_bytes_per_row = true;
	break;

      case glgfx_bitmap_copy_unknown:
      case glgfx_bitmap_copy_max:
	/* Make compiler happy */
	break;
    }
  }

  if (format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max) {
    return false;
  }

  if (!got_bytes_per_row) {
    bytes_per_row = formats[format].size * width;
  }

  if (width <= 0 || height <= 0 || 
      (x + width) > bitmap->width || (y + height) > bitmap->height ||
      data == NULL || bitmap->locked || bytes_per_row == 0) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  glgfx_context_bindtex(context, 0, bitmap);
   
  glPixelStorei(GL_UNPACK_ROW_LENGTH, bytes_per_row / formats[format].size);
  glTexSubImage2D(bitmap->texture_target, 0,
		  x, y, width, height,
		  formats[format].format,
		  formats[format].type,
		  data);
  GLGFX_CHECKERROR();
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  glgfx_context_unbindtex(context, 0);

  bitmap->has_changed = true;

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


bool glgfx_bitmap_getattr(struct glgfx_bitmap* bm,
			  enum glgfx_bitmap_attr attr,
			  intptr_t* storage) {
  bool rc = true;

  if (bm == NULL || storage == NULL ||
      attr <= glgfx_bitmap_attr_unknown || attr >= glgfx_bitmap_attr_max) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  switch (attr) {
    case glgfx_bitmap_attr_width:
      *storage = bm->width;
      break;

    case glgfx_bitmap_attr_height:
      *storage = bm->height;
      break;

    case glgfx_bitmap_attr_bits:
      *storage = (formats[bm->format].redbits + 
		  formats[bm->format].greenbits + 
		  formats[bm->format].bluebits + 
		  formats[bm->format].alphabits);
      break;

    case glgfx_bitmap_attr_friend:
      rc = false;
      break;

    case glgfx_bitmap_attr_format: 
      *storage = bm->format;
      break;

    case glgfx_bitmap_attr_bytesperrow:
      *storage = formats[bm->format].size * bm->width;
      break;
      
    case glgfx_bitmap_attr_locked:
      *storage = bm->locked;
      break;
      
    case glgfx_bitmap_attr_mapaddr:
      *storage = (intptr_t) bm->locked_memory;
      break;

    case glgfx_bitmap_attr_unknown:
    case glgfx_bitmap_attr_max:
      rc = false;
      break;
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


bool glgfx_bitmap_select(struct glgfx_bitmap* bitmap) {
  if (bitmap == NULL) {
    return false;
  }

  // TODO: Set rendering buffer to the bitmap here
  abort();
  return false;
}

bool glgfx_bitmap_waitblit(struct glgfx_bitmap* bitmap) {
  if (bitmap == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  glgfx_bitmap_select(bitmap);
  glFinish();
  
  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


bool glgfx_bitmap_haschanged(struct glgfx_bitmap* bitmap) {
  bool has_changed = bitmap->has_changed;

  bitmap->has_changed = false;

  return has_changed;
}


/* Blitter minterms -> glLogicOp() argument

# Bit number in minterm
A Source rectangle (always on for non-twisted blits)
B Source pixel
C Destination pixel

# ABC		Minterm 	glLogicOp
- ---		-------		---------
0 000		00		GL_CLEAR	
1 001		10		GL_NOR		
2 010		20		GL_AND_INVERTED	
3 011		30		GL_COPY_INVERTED
4 100		40		GL_AND_REVERSE	
5 101		50		GL_INVERT	
6 110		60		GL_XOR		
7 111		70		GL_NAND		
		80		GL_AND		
		90		GL_EQUIV	
		a0		GL_NOOP		
		b0		GL_OR_INVERTED	
		c0		GL_COPY		
		d0		GL_OR_REVERSE	
		e0		GL_OR		
		f0		GL_SET		
*/

bool glgfx_bitmap_blit_a(struct glgfx_bitmap* bitmap, 
			 struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();

  struct glgfx_tagitem const* tag;
  int src_x = -1, src_y = -1, src_width = -1, src_height = -1;
  int mod_x =  0, mod_y =  0, mod_width = -1, mod_height = -1;
  int dst_x = -1, dst_y = -1, dst_width = -1, dst_height = -1;
  struct glgfx_bitmap* src_bitmap = bitmap;
  struct glgfx_bitmap* mod_bitmap = NULL;
  struct glgfx_bitmap* dst_bitmap = bitmap;
  void* mask = NULL;
  int mask_x = 0, mask_y = 0;
  int mask_bpp = 0;
  int minterm = 0xc0;
  GLfloat mod_r = 1.0, mod_g = 1.0, mod_b = 1.0, mod_a = 1.0;
  enum glgfx_blend_equation blend_eq = glgfx_blend_equation_disabled;
  enum glgfx_blend_equation blend_eq_alpha = glgfx_blend_equation_unknown;
  enum glgfx_blend_func blend_func_src = glgfx_blend_func_srcalpha;
  enum glgfx_blend_func blend_func_src_alpha = glgfx_blend_func_unknown;
  enum glgfx_blend_func blend_func_dst = glgfx_blend_func_srcalpha_inv;
  enum glgfx_blend_func blend_func_dst_alpha = glgfx_blend_func_unknown;

  bool got_src_width = false;
  bool got_src_height = false;
  bool got_mod_width = false;
  bool got_mod_height = false;
  bool got_mod_rgba = false;

  static GLenum const ops[16] = {
    GL_CLEAR, GL_NOR, GL_AND_INVERTED, GL_COPY_INVERTED, 
    GL_AND_REVERSE, GL_INVERT, GL_XOR, GL_NAND,
    GL_AND, GL_EQUIV, GL_NOOP, GL_OR_INVERTED,
    GL_COPY, GL_OR_REVERSE, GL_OR, GL_SET
  };


  if (bitmap == NULL || tags == NULL) {
    errno = EINVAL;
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_blit_tag) tag->tag) {
      case glgfx_bitmap_blit_x:
	dst_x = tag->data;
	break;

      case glgfx_bitmap_blit_y:
	dst_y = tag->data;
	break;

      case glgfx_bitmap_blit_width:
	dst_width = tag->data;
	break;

      case glgfx_bitmap_blit_height:
	dst_height = tag->data;
	break;

      case glgfx_bitmap_blit_src_x:
	src_x = tag->data;
	break;

      case glgfx_bitmap_blit_src_y:
	src_y = tag->data;
	break;

      case glgfx_bitmap_blit_src_width:
	src_width = tag->data;
	got_src_width = true;
	break;

      case glgfx_bitmap_blit_src_height:
	src_height = tag->data;
	got_src_height = true;
	break;

      case glgfx_bitmap_blit_src_bitmap:
	src_bitmap = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_bitmap_blit_mask_x:
	mask_x = tag->data;
	break;

      case glgfx_bitmap_blit_mask_y:
	mask_y = tag->data;
	break;

      case glgfx_bitmap_blit_mask_ptr:
	mask = (void*) tag->data;
	break;

      case glgfx_bitmap_blit_mask_bytesperrow:
	mask_bpp = tag->data;
	break;
	
      case glgfx_bitmap_blit_mod_r:
	mod_r = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_g:
	mod_g = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_b:
	mod_b = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_a:
	mod_a = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_bitmap:
	mod_bitmap = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_bitmap_blit_mod_x:
	mod_x = tag->data;
	break;

      case glgfx_bitmap_blit_mod_y:
	mod_y = tag->data;
	break;

      case glgfx_bitmap_blit_mod_width:
	mod_width = tag->data;
	got_mod_width = true;
	break;

      case glgfx_bitmap_blit_mod_height:
	mod_height = tag->data;
	got_mod_height = true;
	break;

      case glgfx_bitmap_blit_minterm:
	minterm = tag->data;
	break;

      case glgfx_bitmap_blit_blend_equation:
	blend_eq = tag->data;
	break;
	
      case glgfx_bitmap_blit_blend_equation_alpha:
	blend_eq_alpha = tag->data;
	break;

      case glgfx_bitmap_blit_blend_srcfunc:
	blend_func_src = tag->data;
	break;

      case glgfx_bitmap_blit_blend_srcfunc_alpha:
	blend_func_src_alpha = tag->data;
	break;

      case glgfx_bitmap_blit_blend_dstfunc:
	blend_func_dst = tag->data;
	break;

      case glgfx_bitmap_blit_blend_dstfunc_alpha:
	blend_func_dst_alpha = tag->data;
	break;

      case glgfx_bitmap_blit_unknown:
      case glgfx_bitmap_blit_max:
	break;
    }
  }

  if (!got_src_width) {
    // Default is 1:1 copy
    src_width = dst_width;
  }

  if (!got_src_height) {
    // Default is 1:1 copy
    src_height = dst_height;
  }

  if (mod_bitmap != NULL && !got_mod_width) {
    // Default is to use full bitmap
    mod_width = mod_bitmap->width;
  }

  if (mod_bitmap != NULL && !got_mod_height) {
    // Default is to use full bitmap
    mod_height = mod_bitmap->height;
  }

  if (mask_bpp == 0) {
    mask_bpp = src_width / 8;
  }

  if (blend_eq_alpha == glgfx_blend_equation_unknown||
      blend_eq_alpha == glgfx_blend_equation_disabled) {
    blend_eq_alpha = blend_eq;
  }

  if (blend_func_src_alpha == glgfx_blend_func_unknown) {
    blend_func_src_alpha = blend_func_src;
  }

  if (blend_func_dst_alpha == glgfx_blend_func_unknown) {
    blend_func_dst_alpha = blend_func_dst;
  }


  if (src_bitmap == NULL && mod_bitmap != NULL) {
    // NULL src bitmap components are always 1.0 and the coordinates are
    // irrelevant. If mod_bitmap is present, set it as the new source.
    src_bitmap = mod_bitmap;
    src_x = mod_x;
    src_y = mod_y;
    src_width = mod_width;
    src_height = mod_height;

    mod_bitmap = NULL;
  }

  if (src_x < 0 || src_y < 0 || src_width <= 0 || src_height <= 0 ||
      dst_x < 0 || dst_y < 0 || dst_width <= 0 || dst_height <= 0 ||
      (src_bitmap != NULL && src_x + src_width > src_bitmap->width) || 
      dst_x + dst_width > dst_bitmap->width || 
      (src_bitmap != NULL && src_y + src_height > src_bitmap->height) ||
      dst_y + dst_height > dst_bitmap->height ||
      (minterm & ~0xff) != 0) {
    errno = EINVAL;
    return false;
  }

  if (mod_bitmap != NULL) {
    if (mod_x < 0 || mod_y < 0 || mod_width <= 0 || mod_height <= 0 ||
	mod_x + mod_width > mod_bitmap->width || 
	mod_y + mod_height > mod_bitmap->height) {
      errno = EINVAL;
      return false;
    }
  }

  if (mask != NULL) {
    if (mask_bpp < 0 || 
	mask_x < 0 || mask_y < 0 ||
	mask_x >= mask_bpp * 8) {
      errno = EINVAL;
      return false;
    }
  }

  if (blend_eq <= glgfx_blend_equation_unknown ||
      blend_eq_alpha <= glgfx_blend_equation_unknown ||
      blend_func_src <= glgfx_blend_func_unknown ||
      blend_func_src_alpha <= glgfx_blend_func_unknown ||
      blend_func_dst <= glgfx_blend_func_unknown ||
      blend_func_dst_alpha <= glgfx_blend_func_unknown ||
      blend_eq >= glgfx_blend_equation_max ||
      blend_eq_alpha >= glgfx_blend_equation_max ||
      blend_func_src >= glgfx_blend_func_max ||
      blend_func_src_alpha >= glgfx_blend_func_max ||
      blend_func_dst >= glgfx_blend_func_max ||
      blend_func_dst_alpha >= glgfx_blend_func_max) {
    errno = EINVAL;
    return false;
  }


  bool rc = true;

  if (src_bitmap == dst_bitmap && 
      mod_bitmap == NULL &&
      dst_width == src_width && 
      dst_height == dst_height &&
      mask == NULL &&
      !got_mod_rgba &&
      (!context->monitor->miss_pixel_ops || (minterm & 0xf0) == 0xc0)) {
    if ((minterm & 0xf0) != 0xc0) {
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(ops[minterm >> 4]);
    }

    // Bind FBO and attach texture
    glgfx_context_bindfbo(context, dst_bitmap);

    // Blit using glCopyPixels(), no texturing
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glWindowPos2i(dst_x, dst_y);
    glCopyPixels(src_x, src_y, src_width, src_height, GL_COLOR);

    if ((minterm & 0xf0) != 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }
  }
  else {
    if (src_bitmap == dst_bitmap || mask != NULL) {
      struct glgfx_bitmap* tmp_bitmap;
      enum glgfx_pixel_format fmt;
      
      if (mask == NULL) {
	fmt = src_bitmap->format;
      }
      else {
	fmt = glgfx_pixel_format_a8r8g8b8;
      }

      tmp_bitmap = glgfx_context_gettempbitmap(context,
					       src_width, src_height,
					       fmt);
      if (tmp_bitmap == NULL) {
	errno = ENOMEM;
	rc = false;
      }
      else {
	// Bind FBO and attach destination (aka the temp src) bitmap
	glgfx_context_bindfbo(context, tmp_bitmap);

	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

	if (mask != NULL) {
	  // Clear area
	  glColor4f(0, 0, 0, 0);
	  glgfx_context_bindprogram(context, &color_blitter);

	  glBegin(GL_QUADS); {
	    glVertex2i(0,
		       src_height);
	    glVertex2i(src_width,
		       src_height);
	    glVertex2i(src_width,
		       0);
	    glVertex2i(0,  
		       0);
	  }
	  glEnd();

	  // Draw mask
	  glColor4f(1, 1, 1, 1);

	  if (mask_x != 0) {
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, mask_bpp * 8);
	  }

	  glWindowPos2i(0, 0);
	  glBitmap(mask_bpp * 8 - mask_x, src_height,
		   mask_x, mask_y,
		   0, 0,
		   mask);
	  
	  if (mask_x != 0) {
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	  }

	  // Enable blending
	  glEnable(GL_BLEND);
	  glBlendFunc(GL_DST_COLOR, GL_ZERO);
	  glBlendEquation(GL_FUNC_ADD);
	}

	GLenum unit = GL_TEXTURE0;

	if (src_bitmap != NULL) {
	  // Bind src bitmap as texture
	  unit = glgfx_context_bindtex(context, 0, src_bitmap);

	  if (mask == NULL) {
	    // Make a plain copy, with no color-space transformations
	    glgfx_context_bindprogram(context, &raw_texture_blitter);
	  }
	  else {
	    // Copy bitmap to RGBA temp bitmap (FP bitmaps will be clamped
	    // here, but we NEED a working alpha channel)
	    glgfx_context_bindprogram(context, &plain_texture_blitter);
	  }

	  glBegin(GL_QUADS); {
	    glMultiTexCoord2i(unit, 
			      src_x,
			      src_y);
	    glVertex2i(0,
		       src_height);

	    glMultiTexCoord2i(unit,
			      src_x + src_width,
			      src_y);
	    glVertex2i(src_width,
		       src_height);

	    glMultiTexCoord2i(unit,
			      src_x + src_width, 
			      src_y + src_height);
	    glVertex2i(src_width,
		       0);

	    glMultiTexCoord2i(unit,
			      src_x,
			      src_y + src_height);
	    glVertex2i(0,  
		       0);
	  }
	  glEnd();
	}

	if (mask != NULL) {
	  glDisable(GL_BLEND);
	  GLGFX_CHECKERROR();
	}

	src_x = 0;
	src_y = 0;

	// Install tmp bitmap as new source
	src_bitmap = tmp_bitmap;
      }
    }

    if ((minterm & 0xf0) != 0xc0) {
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(ops[minterm >> 4]);
    }

    // Bind FBO and attach texture
    glgfx_context_bindfbo(context, dst_bitmap);

    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

    GLenum src_unit = GL_TEXTURE0;
    GLenum mod_unit = GL_TEXTURE1;

    if (src_bitmap != NULL) {
      // Bind temp src bitmap as texture
      src_unit = glgfx_context_bindtex(context, 0, src_bitmap);

      if (mod_bitmap != NULL) {
	// Bind mod bitmap as texture
	mod_unit = glgfx_context_bindtex(context, 1, mod_bitmap);

	glColor4f(mod_r, mod_g, mod_b, mod_a);
	glgfx_context_bindprogram(context, &modulated_texture_blitter);
      }
      else if (got_mod_rgba) {
	glColor4f(mod_r, mod_g, mod_b, mod_a);
	glgfx_context_bindprogram(context, &color_texture_blitter);
      }
      else {
	glgfx_context_bindprogram(context, &plain_texture_blitter);
      }
    }
    else {
      // NULL source texture -> mod_bitmap is also NULL -> plain color blit
      glColor4f(mod_r, mod_g, mod_b, mod_a);
      glgfx_context_bindprogram(context, &color_blitter);
    }

    if (blend_eq != glgfx_blend_equation_disabled) {
      static GLenum const eq[glgfx_blend_equation_max] = {
	0, 0,
	GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN, GL_MAX
      };

      static GLenum const func[glgfx_blend_func_max] = {
	0,
	GL_ZERO, GL_ONE, 
	GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
	GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,
	GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA,
/* 	GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, */
/* 	GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, */
	GL_SRC_ALPHA_SATURATE
      };

      glEnable(GL_BLEND);
      glBlendEquationSeparate(eq[blend_eq], eq[blend_eq_alpha]);
      glBlendFuncSeparate(func[blend_func_src], func[blend_func_dst],
			  func[blend_func_src_alpha], func[blend_func_dst_alpha]);
    }

    glBegin(GL_QUADS); {
      glMultiTexCoord2i(src_unit,
			src_x,
			src_y);
      glMultiTexCoord2i(mod_unit,
			mod_x,
			mod_y);
      glVertex2i(dst_x,
		 dst_bitmap->height - dst_y);

      glMultiTexCoord2i(src_unit,
			src_x + src_width, 
			src_y);
      glMultiTexCoord2i(mod_unit,
			mod_x + mod_width, 
			mod_y);
      glVertex2i(dst_x + dst_width, 
		 dst_bitmap->height - dst_y);

      glMultiTexCoord2i(src_unit,
			src_x + src_width, 
			src_y + src_height);
      glMultiTexCoord2i(mod_unit,
			mod_x + mod_width, 
			mod_y + mod_height);
      glVertex2i(dst_x + dst_width, 
		 dst_bitmap->height - (dst_y + dst_height));

      glMultiTexCoord2i(src_unit,
			src_x,
			src_y + src_height);
      glMultiTexCoord2i(mod_unit,
			mod_x,
			mod_y + mod_height);
      glVertex2i(dst_x,
		 dst_bitmap->height - (dst_y + dst_height));
    }
    glEnd();

    if (blend_eq != glgfx_blend_equation_disabled) {
      glDisable(GL_BLEND);
    }

    if ((minterm & 0xf0) != 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }

    GLGFX_CHECKERROR();
  }

  glgfx_context_unbindtex(context, 0);
  glgfx_context_unbindtex(context, 1);
  
  if (rc) {
    dst_bitmap->has_changed = true;
  }

  return rc;
}
