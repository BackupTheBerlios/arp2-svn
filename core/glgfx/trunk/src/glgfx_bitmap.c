
#include "glgfx-config.h"
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"
#include "glgfx_glext.h"
#include "glgfx_intern.h"

#define GL_PIXEL_UNPACK_BUFFER_ARB GL_PIXEL_UNPACK_BUFFER_EXT
#define GL_PIXEL_PACK_BUFFER_ARB GL_PIXEL_PACK_BUFFER_EXT

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


struct glgfx_bitmap* glgfx_bitmap_create_a(struct glgfx_monitor* monitor,
					   struct glgfx_tagitem const* tags) {
  struct glgfx_bitmap* bitmap;

  int width = 0, height = 0, bits = 0;
  struct glgfx_bitmap* friend = NULL;
  enum glgfx_pixel_format format = glgfx_pixel_format_unknown;
  
  struct glgfx_tagitem const* tag;

  if (monitor == NULL || tags == NULL) {
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_tag) tag->tag) {
      
      case glgfx_bitmap_tag_width:
	width = tag->data;
	break;

      case glgfx_bitmap_tag_height:
	height = tag->data;
	break;

      case glgfx_bitmap_tag_bits:
	bits = tag->data;
	break;

      case glgfx_bitmap_tag_friend:
	friend = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_bitmap_tag_format:
	format = tag->data;
	break;

      case glgfx_bitmap_tag_x:
      case glgfx_bitmap_tag_y:
      case glgfx_bitmap_tag_data:
      case glgfx_bitmap_tag_bytesperrow:
      case glgfx_bitmap_tag_unknown:
      case glgfx_bitmap_tag_max:
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
  
  glgfx_monitor_select(monitor);

  bitmap->monitor           = monitor;
  bitmap->width             = width;
  bitmap->height            = height;
  bitmap->bits              = bits;
  bitmap->format            = format;
  bitmap->pbo_size          = glgfx_texture_size(width, height, bitmap->format);
  bitmap->pbo_bytes_per_row = glgfx_texture_size(width, 1, bitmap->format);

  glGenTextures(1, &bitmap->texture);
  check_error();

  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
  check_error();

  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
	       formats[bitmap->format].internal_format,
	       width, height, 0,
	       formats[bitmap->format].format,
	       formats[bitmap->format].type,
	       NULL);
  check_error();

/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST); */
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  check_error();

  D(BUG("Returning bitmap %p\n", bitmap));

  pthread_mutex_unlock(&glgfx_mutex);
  return bitmap;
}


void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap) {
  if (bitmap == NULL) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);
  glgfx_bitmap_unlock(bitmap, 0, 0, 0, 0);
  if (bitmap->monitor->have_GL_ARB_pixel_buffer_object) {
    glDeleteBuffersARB(1, &bitmap->pbo);
  }
  else {
    free(bitmap->buffer);
  }
  glDeleteTextures(1, &bitmap->texture);
  free(bitmap);
  pthread_mutex_unlock(&glgfx_mutex);
}


bool glgfx_bitmap_lock(struct glgfx_bitmap* bitmap, bool read, bool write) {
  void* res = NULL;

  if (bitmap == NULL || (!read && !write)) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
    
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
  
  if (bitmap->monitor->have_GL_ARB_pixel_buffer_object) {
    if (bitmap->pbo == 0) {
      glGenBuffersARB(1, &bitmap->pbo);
      check_error();
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      check_error();
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo_size, 
		      NULL, bitmap->locked_usage);
      check_error();
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }
  }
  else {
    if (bitmap->buffer == NULL) {
      bitmap->buffer = malloc(bitmap->pbo_size);
    }
  }

  bitmap->locked = true;

  if (bitmap->locked_access == GL_READ_WRITE_ARB ||
      bitmap->locked_access == GL_READ_ONLY_ARB) {
    if (bitmap->monitor->have_GL_ARB_pixel_buffer_object) {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
      check_error();
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, bitmap->pbo);
      check_error();
      glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0,
		    formats[bitmap->format].format,
		    formats[bitmap->format].type,
		    NULL);
      check_error();
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }
    else {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
      check_error();
      glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0,
		    formats[bitmap->format].format,
		    formats[bitmap->format].type,
		    bitmap->buffer);
      check_error();
    }
  }
  
  if (bitmap->monitor->have_GL_ARB_pixel_buffer_object) {
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
    bitmap->locked_memory = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
					   bitmap->locked_access);
  }
  else {
    bitmap->locked_memory = bitmap->buffer;
  }

  res = bitmap->locked_memory;

  pthread_mutex_unlock(&glgfx_mutex);

  return res != NULL;
}


bool glgfx_bitmap_unlock(struct glgfx_bitmap* bitmap, 
			 int x, int y, int width, int height) {
  bool rc = false;

  if (bitmap == NULL || !bitmap->locked) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  if (bitmap->locked_memory != NULL) {
    if (bitmap->monitor->have_GL_ARB_pixel_buffer_object) {
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      if (glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB)) {
	rc = true;
      }

      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE_ARB ||
	   bitmap->locked_access == GL_WRITE_ONLY_ARB)) {
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
	check_error();

	glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->width);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
			0, 0, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (x + y * bitmap->pbo_bytes_per_row));
	check_error();
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
      }

      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      check_error();
    }
    else {
      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE_ARB ||
	   bitmap->locked_access == GL_WRITE_ONLY_ARB)) {
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
	check_error();
	
	glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->width);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
			0, 0, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (bitmap->buffer + x + y * bitmap->pbo_bytes_per_row));
	check_error();
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
      }
    }

    bitmap->locked_memory = NULL;
  }

  bitmap->locked = false;
  
  pthread_mutex_unlock(&glgfx_mutex);

  return rc;
}


bool glgfx_bitmap_update_a(struct glgfx_bitmap* bitmap, 
			   struct glgfx_tagitem const* tags) {
  int x = 0, y = 0, width = 0, height = 0;
  void* data = NULL;
  enum glgfx_pixel_format format = glgfx_pixel_format_unknown;
  size_t bytes_per_row = 0;

  struct glgfx_tagitem const* tag;

  if (bitmap == NULL || tags == NULL) {
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_tag) tag->tag) {
      case glgfx_bitmap_tag_x:
	x = tag->data;
	break;

      case glgfx_bitmap_tag_y:
	y = tag->data;
	break;

      case glgfx_bitmap_tag_width:
	width = tag->data;
	break;

      case glgfx_bitmap_tag_height:
	height = tag->data;
	break;

      case glgfx_bitmap_tag_data:
	data = (void*) tag->data;
	break;

      case glgfx_bitmap_tag_format:
	format = tag->data;
	break;

      case glgfx_bitmap_tag_bytesperrow:
	bytes_per_row = tag->data;
	break;

      case glgfx_bitmap_tag_bits:
      case glgfx_bitmap_tag_friend:
      case glgfx_bitmap_tag_unknown:
      case glgfx_bitmap_tag_max:
	/* Make compiler happy */
	break;
    }
  }

  if (width <= 0 || height <= 0 || data == NULL || bitmap->locked ||
      format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max) {
    return false;
  }

  if (bytes_per_row == 0) {
    bytes_per_row = formats[format].size * width;
  }

  pthread_mutex_lock(&glgfx_mutex);

  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
  check_error();
   
  glPixelStorei(GL_PACK_ROW_LENGTH, bytes_per_row / formats[format].size);
  glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
		  0, 0, width, height,
		  formats[format].format,
		  formats[format].type,
		  data);
  check_error();

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


bool glgfx_bitmap_getattr(struct glgfx_bitmap* bm,
			  enum glgfx_bitmap_attr attr,
			  intptr_t* storage) {
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
      
    default:
      abort();
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
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
