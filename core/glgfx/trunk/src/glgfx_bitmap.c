
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"
#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_intern.h"

#ifndef GL_PIXEL_PACK_BUFFER_ARB
# define GL_PIXEL_PACK_BUFFER_ARB GL_PIXEL_PACK_BUFFER_EXT
# define GL_PIXEL_UNPACK_BUFFER_ARB GL_PIXEL_UNPACK_BUFFER_EXT
#endif

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

  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
  GLGFX_CHECKERROR();

  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
	       formats[bitmap->format].internal_format,
	       width, height, 0,
	       formats[bitmap->format].format,
	       formats[bitmap->format].type,
	       NULL);
  GLGFX_CHECKERROR();

/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST); */
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  
  GLGFX_CHECKERROR();

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
    glDeleteBuffersARB(1, &bitmap->pbo);
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
    bitmap->locked_usage = GL_STREAM_COPY_ARB;
    bitmap->locked_access = GL_READ_WRITE_ARB;
  }
  else if (!read && write) {
    bitmap->locked_usage = GL_STREAM_DRAW_ARB;
    bitmap->locked_access = GL_WRITE_ONLY_ARB;
  }
  else {
    bitmap->locked_usage = GL_STATIC_READ_ARB;
    bitmap->locked_access = GL_READ_ONLY_ARB;
  }
  
  if (context->monitor->have_GL_ARB_pixel_buffer_object) {
    if (bitmap->pbo == 0) {
      glGenBuffersARB(1, &bitmap->pbo);
      GLGFX_CHECKERROR();
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      GLGFX_CHECKERROR();
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo_size, 
		      NULL, bitmap->locked_usage);
      GLGFX_CHECKERROR();
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }
  }
  else {
    if (bitmap->buffer == NULL) {
      bitmap->buffer = malloc(bitmap->pbo_size);
    }
  }

  bitmap->locked = true;

  if (read) {
    // Bind FBO and attach texture
    glgfx_context_bindfbo(context, bitmap);

    if (context->monitor->have_GL_ARB_pixel_buffer_object) {
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, bitmap->pbo);
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
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }
    else {
/*       glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture); */
/*       GLGFX_CHECKERROR(); */
/*       glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, */
/* 		    formats[bitmap->format].format, */
/* 		    formats[bitmap->format].type, */
/* 		    bitmap->buffer); */
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
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
    bitmap->locked_memory = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
					   bitmap->locked_access);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
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
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      if (glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB)) {
	rc = true;
      }

      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE_ARB ||
	   bitmap->locked_access == GL_WRITE_ONLY_ARB)) {
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
	GLGFX_CHECKERROR();

	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
			x, y, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (x * formats[bitmap->format].size +
				 y * bitmap->pbo_bytes_per_row));
	GLGFX_CHECKERROR();
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      }

      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      GLGFX_CHECKERROR();
    }
    else {
      rc = true;

      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE_ARB ||
	   bitmap->locked_access == GL_WRITE_ONLY_ARB)) {
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
	GLGFX_CHECKERROR();
	
	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	GLGFX_CHECKERROR();
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
			x, y, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (bitmap->buffer +
				 x * formats[bitmap->format].size +
				 y * bitmap->pbo_bytes_per_row));
	GLGFX_CHECKERROR();
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
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

  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
  GLGFX_CHECKERROR();
   
  glPixelStorei(GL_UNPACK_ROW_LENGTH, bytes_per_row / formats[format].size);
  glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
		  x, y, width, height,
		  formats[format].format,
		  formats[format].type,
		  data);
  GLGFX_CHECKERROR();
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

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
  int dst_x = -1, dst_y = -1, dst_width = -1, dst_height = -1;
  struct glgfx_bitmap* src_bitmap = bitmap;
  struct glgfx_bitmap* dst_bitmap = bitmap;
  int minterm = 0xc0;
  GLuint mod_r = 0xffffffff, mod_g = 0xffffffff, mod_b = 0xffffffff, mod_a = 0xffffffff;

  bool got_src_width = false;
  bool got_src_height = false;
  bool got_mod = false;

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
	
      case glgfx_bitmap_blit_minterm:
	minterm = tag->data;
	break;

      case glgfx_bitmap_blit_mod_r:
	mod_r = tag->data;
	got_mod = true;
	break;

      case glgfx_bitmap_blit_mod_g:
	mod_g = tag->data;
	got_mod = true;
	break;

      case glgfx_bitmap_blit_mod_b:
	mod_b = tag->data;
	got_mod = true;
	break;

      case glgfx_bitmap_blit_mod_a:
	mod_a = tag->data;
	got_mod = true;
	break;

      case glgfx_bitmap_blit_unknown:
      case glgfx_bitmap_blit_max:
	break;
    }
  }

  if (!got_src_width) {
    src_width = dst_width;
  }

  if (!got_src_height) {
    src_height = dst_height;
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


  bool rc = true;

  if (src_bitmap == dst_bitmap && 
      dst_width == src_width && 
      dst_height == dst_height &&
      !got_mod &&
      (!context->monitor->miss_pixel_ops || (minterm & 0xf0) == 0xc0)) {
    if ((minterm & 0xf0) == 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }
    else {
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(ops[minterm >> 4]);
    }

    // Bind FBO and attach texture
    glgfx_context_bindfbo(context, dst_bitmap);

    // Blit using glCopyPixels()
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDisable(GL_TEXTURE_RECTANGLE_ARB); // This is apparently important. Why??

    glRasterPos2i(dst_x, dst_bitmap->height - dst_y);
    glCopyPixels(src_x, src_y, src_width, src_height, GL_COLOR);

    if ((minterm & 0xf0) != 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }
  }
  else {
    if (src_bitmap == dst_bitmap) {
      src_bitmap = glgfx_context_gettempbitmap(context,
					       src_width, src_height,
					       src_bitmap->format);
      if (src_bitmap == NULL) {
	errno = ENOMEM;
	rc = false;
      }
      else {
	// Bind FBO and attach source bitmap
	glgfx_context_bindfbo(context, dst_bitmap);

	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

	// Bind temp src bitmap as texture
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, src_bitmap->texture);
	GLGFX_CHECKERROR();

	// Copy source bitmap into temp bitmap
	glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
			    0, 0,
			    src_x, src_y, src_width, src_height);
	src_x = 0;
	src_y = 0;
      }
    }

    if ((minterm & 0xf0) == 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }
    else {
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(ops[minterm >> 4]);
    }

    // Bind FBO and attach texture
    glgfx_context_bindfbo(context, dst_bitmap);

    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

    if (src_bitmap != NULL) {
      // Bind temp src bitmap as texture
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, src_bitmap->texture);
      GLGFX_CHECKERROR();
      
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
    }
    else {
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

    glColor4ui(mod_r, mod_g, mod_b, mod_a);

    glBegin(GL_QUADS); {
      glTexCoord2i(src_x,             src_y);
      glVertex2i  (dst_x,             dst_bitmap->height - dst_y);

      glTexCoord2i(src_x + src_width, src_y);
      glVertex2i  (dst_x + dst_width, dst_bitmap->height - dst_y);

      glTexCoord2i(src_x + src_width, src_y + src_height);
      glVertex2i  (dst_x + dst_width, dst_bitmap->height - (dst_y + dst_height));

      glTexCoord2i(src_x,             src_y + src_height);
      glVertex2i  (dst_x,             dst_bitmap->height - (dst_y + dst_height));
    }
    glEnd();

    if (src_bitmap != NULL) {
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

    if ((minterm & 0xf0) != 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }

    GLGFX_CHECKERROR();
  }

  
  if (rc) {
    dst_bitmap->has_changed = true;
  }

  return rc;
}
		       

/*     GLcharARB const* source =  */
/*       "void main(void) {\n" */
/*       "  gl_FragColor = vec4(1.0, 1.0, 1.0, 0.5);\n" */
/*       "}"; */

/*     GLhandleARB program = glCreateProgramObjectARB(); */
/*     GLhandleARB shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB); */
    
/*     glShaderSourceARB(shader, 1, &source, NULL); */
/*     glCompileShaderARB(shader); */
/*     glAttachObjectARB(program, shader); */
/*     glLinkProgramARB(program); */

/*     void printInfoLog(GLhandleARB obj) { */
/*       int infologLength = 0; */
/*       int charsWritten  = 0; */
/*       char *infoLog; */

/*       glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, */
/* 				&infologLength); */

/*       if (infologLength > 0) { */
/* 	infoLog = (char*) malloc(infologLength); */
/* 	glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog); */
/* 	printf("%s\n",infoLog); */
/* 	free(infoLog); */
/*       } */
/*     } */

/*     printInfoLog(program); */
