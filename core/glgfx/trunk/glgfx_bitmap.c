
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"

struct pixel_info {
    GLint  internal_format;
    GLenum format;
    GLenum type;
    size_t size;
};

static struct pixel_info formats[] = {
  { 0, 0, 0, 0},
  { GL_RGB8,  GL_RGB,  GL_UNSIGNED_BYTE, 3 }, // glgfx_pixel_r8g8b8,     RGB,  3 * UBYTE
  { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4 }, // glgfx_pixel_r8g8b8a8,   RGBA, 4 * UBYTE
  { GL_RGB8,  GL_BGR,  GL_UNSIGNED_BYTE, 3 }, // glgfx_pixel_b8g8r8,     BGR,  3 * UBYTE
  { GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, 4 }, // glgfx_pixel_b8g8r8a8   BGRA, 4 * UBYTE
};

static enum glgfx_pixel_format select_format(int bits __attribute__((unused)),
					     struct glgfx_bitmap* friend,
					     enum glgfx_pixel_format format) {
  enum glgfx_pixel_format fmt = friend != NULL ? friend->format : format;

  if (fmt <= glgfx_pixel_unknown || fmt >= glgfx_pixel_max) {
    abort();
  }

  return fmt;
}

static size_t glgfx_texture_size(int width, int height, enum glgfx_pixel_format format) {
  if (format <= glgfx_pixel_unknown || format >= glgfx_pixel_max ||
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

  glgfx_monitor_select(monitor);

  bitmap = calloc(1, sizeof *bitmap);

  if (bitmap == NULL) {
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

/*   float rgba[4] = { 0, 1, 0, 0.1 }; */
/*   glTexParameterfv(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_BORDER_COLOR, rgba); */
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  
  glEnable(GL_TEXTURE_RECTANGLE_EXT);
  check_error();

  D(BUG("Returning bitmap %p\n", bitmap));
  return bitmap;
}

void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap) {
  if (bitmap == NULL) {
    return;
  }

  glgfx_bitmap_unlock(bitmap);
  glDeleteBuffers(1, &bitmap->pbo);
  glDeleteTextures(1, &bitmap->texture);
  free(bitmap);
}

bool glgfx_bitmap_lock(struct glgfx_bitmap* bitmap, bool read, bool write) {
  GLenum usage;
  GLenum access;
  
  if (bitmap == NULL) {
    return false;
  }

  if (read) {
    BUG("No EXT_framebuffer_object yet!\n");
  }
  
  if (read && write) {
    usage = GL_STREAM_COPY_ARB;
    access = GL_READ_WRITE_ARB;
  }
  else if (!read && write) {
    usage = GL_STREAM_DRAW_ARB;
    access = GL_WRITE_ONLY_ARB;
  }
  else {
    usage = GL_STATIC_READ_ARB;
    access = GL_READ_ONLY_ARB;
  }

  if (bitmap->pbo == 0) {
    glGenBuffers(1, &bitmap->pbo);
    check_error();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
    check_error();
    glBufferData(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->size, NULL, usage);
    check_error();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
  }

  if (read) {
    glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, bitmap->pbo);
    // TODO: Read texture data into buffer here
    glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
  }

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, bitmap->pbo);
  bitmap->locked_memory = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, access);

  if (bitmap->locked_memory != NULL) {
    bitmap->locked = true;
    bitmap->locked_write = write;
  }

  return bitmap->locked;
}

bool glgfx_bitmap_unlock(struct glgfx_bitmap* bitmap) {
  bool rc = false;

  if (bitmap == NULL || !bitmap->locked) {
    return false;
  }

  if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT)) {
    rc = true;
  }

  if (bitmap->locked_write) {
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
  bitmap->locked_memory = NULL;
  bitmap->locked_write = false;
  
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

  glgfx_bitmap_select(bitmap);
  glFinish();
  return true;
}
