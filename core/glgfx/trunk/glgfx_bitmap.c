
#include "glgfx-config.h"
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"
#include "glgfx_intern.h"

static enum glgfx_pixel_format select_format(int bits __attribute__((unused)),
					     struct glgfx_bitmap* friend,
					     enum glgfx_pixel_format format) {
  enum glgfx_pixel_format fmt = friend != NULL ? friend->format : format;

  if (fmt == glgfx_pixel_format_unknown) {
  }
  
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

  bitmap->monitor = monitor;
  bitmap->width   = width;
  bitmap->height  = height;
  bitmap->bits    = bits;
  bitmap->flags   = flags;
  bitmap->format  = select_format(bits, friend, format);
  bitmap->size    = glgfx_texture_size(width, height, bitmap->format);

  glGenTextures(1, &bitmap->texture);
  check_error();

  glBindTexture(GL_TEXTURE_RECTANGLE_EXT, bitmap->texture);
  check_error();

  glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0,
	       formats[bitmap->format].internal_format,
	       width, height, 0,
	       formats[bitmap->format].format,
	       formats[bitmap->format].type,
	       NULL);
  check_error();

/*   glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST); */
/*   glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST); */
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  
  glEnable(GL_TEXTURE_RECTANGLE_EXT);
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
  glgfx_bitmap_unlock(bitmap);
  glDeleteBuffers(1, &bitmap->pbo);
  glDeleteTextures(1, &bitmap->texture);
  free(bitmap);
  pthread_mutex_unlock(&glgfx_mutex);
}

bool glgfx_bitmap_lock(struct glgfx_bitmap* bitmap, bool read, bool write) {
  if (bitmap == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  
  if (read) {
    BUG("No EXT_framebuffer_object yet!\n");
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

  if (bitmap->pbo == 0) {
    glGenBuffers(1, &bitmap->pbo);
    check_error();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
    check_error();
    glBufferData(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->size, NULL, bitmap->locked_usage);
    check_error();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
  }

  if (read) {
    glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, bitmap->pbo);
    // TODO: Read texture data into buffer here
    glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
  }

  bitmap->locked = true;

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


void* glgfx_bitmap_map(struct glgfx_bitmap* bitmap) {
  void* res;

  if (bitmap == NULL || !bitmap->locked) {
    return NULL;
  }

  pthread_mutex_lock(&glgfx_mutex);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
  bitmap->locked_memory = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->locked_access);

  res = bitmap->locked_memory;

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}


bool glgfx_bitmap_unmap(struct glgfx_bitmap* bitmap) {
  bool rc = false;
  
  pthread_mutex_lock(&glgfx_mutex);

  if (bitmap->locked_memory != NULL) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
    if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT)) {
      rc = true;
    }

    bitmap->locked_memory = NULL;
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


bool glgfx_bitmap_update(struct glgfx_bitmap* bitmap, void* data, size_t size) {
  if (bitmap == NULL || data == NULL || size > bitmap->size) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  if (bitmap->locked) {
    if (bitmap->locked_access == GL_READ_WRITE_ARB ||
	bitmap->locked_access == GL_WRITE_ONLY_ARB) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
      check_error();

      glBufferSubDataARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0, size, data);
      check_error();
    }
  }
  else {
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, bitmap->texture);
    check_error();
   
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0,
		    0, 0, bitmap->width, bitmap->height,
		    formats[bitmap->format].format,
		    formats[bitmap->format].type,
		    data);
    check_error();
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

bool glgfx_bitmap_unlock(struct glgfx_bitmap* bitmap) {
  bool rc = false;

  if (bitmap == NULL || !bitmap->locked) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  glgfx_bitmap_unmap(bitmap);

  if (bitmap->locked_access == GL_READ_WRITE_ARB ||
      bitmap->locked_access == GL_WRITE_ONLY_ARB) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
    check_error();

    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, bitmap->texture);
    check_error();
   
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0,
		    0, 0, bitmap->width, bitmap->height,
		    formats[bitmap->format].format,
		    formats[bitmap->format].type,
		    NULL);
    check_error();
  }

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
  check_error();

  bitmap->locked = false;
  
  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


bool glgfx_bitmap_getattr(struct glgfx_bitmap* bm,
			  enum glgfx_bitmap_attr attr,
			  uint32_t* storage) {
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
      *storage = (uint32_t) bm->locked_memory;
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
