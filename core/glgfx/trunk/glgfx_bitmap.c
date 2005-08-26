
#include "glgfx-config.h"
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"
#include "glgfx_intern.h"

#undef ENABLE_PIXEL_BUFFER

#ifndef GL_TEXTURE_RECTANGLE_ARB
# define GL_TEXTURE_RECTANGLE_ARB GL_TEXTURE_RECTANGLE_EXT
#endif

static enum glgfx_pixel_format select_format(int bits __attribute__((unused)),
					     struct glgfx_bitmap* friend,
					     enum glgfx_pixel_format format) {
  enum glgfx_pixel_format fmt = friend != NULL ? friend->format : format;

  if (fmt <= glgfx_pixel_format_unknown || fmt >= glgfx_pixel_format_max) {
    abort();
  }

  return fmt;
}

static size_t glgfx_texture_size(int width, int height, enum glgfx_pixel_format format) {
  if (format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max ||
    width < 0 || height < 0) {
    BUG("width: %d; height: %d; format: %d\n", width, height, format);
    abort();
  }

  return width * height * formats[format].size;
}

struct glgfx_bitmap* glgfx_bitmap_create(int width, int height, int bits,
					 int flags, struct glgfx_bitmap* friend,
					 int format, struct glgfx_monitor* monitor) {
  struct glgfx_bitmap* bitmap;

  pthread_mutex_lock(&glgfx_mutex);
  
  glgfx_monitor_select(monitor);

  bitmap = calloc(1, sizeof *bitmap);

  if (bitmap == NULL) {
    pthread_mutex_unlock(&glgfx_mutex);
    return NULL;
  }

  bitmap->monitor           = monitor;
  bitmap->width             = width;
  bitmap->height            = height;
  bitmap->bits              = bits;
  bitmap->flags             = flags;
  bitmap->format            = select_format(bits, friend, format);
#ifdef ENABLE_PIXEL_BUFFER
  bitmap->pbo_size          = glgfx_texture_size(width, height, bitmap->format);
  bitmap->pbo_bytes_per_row = glgfx_texture_size(width, 1, bitmap->format);
#endif

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
#ifdef ENABLE_PIXEL_BUFFER
  glDeleteBuffers(1, &bitmap->pbo);
#else
  free(bitmap->buffer);
#endif
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
    
#ifdef ENABLE_PIXEL_BUFFER
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

  if (bitmap->pbo == 0) {
    glGenBuffers(1, &bitmap->pbo);
    check_error();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
    check_error();
    glBufferData(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo_size, NULL, bitmap->locked_usage);
    check_error();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
  }

  bitmap->locked = true;

  if (bitmap->locked_access == GL_READ_WRITE_ARB ||
      bitmap->locked_access == GL_READ_ONLY_ARB) {
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
    check_error();
    glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, bitmap->pbo);
    check_error();
    glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0,
		  formats[bitmap->format].format,
		  formats[bitmap->format].type,
		  NULL);
    check_error();
    glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
  }
  
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
  bitmap->locked_memory = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->locked_access);

#else // No PBO support

  if (read && write) {
    bitmap->locked_usage = 3;
  }
  else if (!read && write) {
    bitmap->locked_usage = 2;
  }
  else {
    bitmap->locked_usage = 1;
  }

  if (bitmap->buffer == NULL) {
    bitmap->buffer = malloc(bitmap->pbo_size);
  }

  bitmap->locked = true;

  if (bitmap->locked_usage & 1) {
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
    check_error();
    glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0,
		  formats[bitmap->format].format,
		  formats[bitmap->format].type,
		  bitmap->buffer);
    check_error();
  }
  
  bitmap->locked_memory = bitmap->buffer;
#endif

  res = bitmap->locked_memory;

  pthread_mutex_unlock(&glgfx_mutex);

  return res != NULL;
}


bool glgfx_bitmap_unlock(struct glgfx_bitmap* bitmap, int x, int y, int width, int height) {
  bool rc = false;

  if (bitmap == NULL || !bitmap->locked) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

#ifdef ENABLE_PIXEL_BUFFER
  if (bitmap->locked_memory != NULL) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
    if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT)) {
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
    
    bitmap->locked_memory = NULL;
  }

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
  check_error();

#else

  if (bitmap->locked_memory != NULL) {
    if (width != 0 && height != 0 &&
	bitmap->locked_usage & 2) {
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
    
    bitmap->locked_memory = NULL;
  }
#endif

  bitmap->locked = false;
  
  pthread_mutex_unlock(&glgfx_mutex);

  return rc;
}


bool glgfx_bitmap_update(struct glgfx_bitmap* bitmap,
			 int x, int y, int width, int height,
			 void* data, enum glgfx_pixel_format format, size_t bytes_per_row) {
  if (bitmap == NULL || data == NULL || bitmap->locked ||
      format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max) {
    return false;
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
			  uintptr_t* storage) {
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
      *storage = (uintptr_t) bm->locked_memory;
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
