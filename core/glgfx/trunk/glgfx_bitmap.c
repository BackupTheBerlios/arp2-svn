
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"

struct pixel_info {
    GLint  internal_format;
    GLenum format;
    GLenum type;
};

static struct pixel_info formats[] = {
  { 3, GL_RGB,  GL_UNSIGNED_BYTE }, // glgfx_pixel_r8g8b8,
  { 4, GL_RGBA, GL_UNSIGNED_BYTE }, // glgfx_pixel_r8g8b8a8
};

static enum glgfx_pixel_format select_format(int depth, struct glgfx_bitmap* friend, int format) {
  return friend != NULL ? friend->format : format;
}

struct glgfx_bitmap* glgfx_bitmap_create(int width, int height, int depth,
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
  bitmap->depth   = depth;
  bitmap->flags   = flags;
  bitmap->format  = select_format(depth, friend, format);

  glGenTextures(1, &bitmap->texture);

  glBindTexture(GL_TEXTURE_RECTANGLE_EXT, bitmap->texture);
  glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0,
	       formats[bitmap->format].internal_format,
	       width, height, 0, 
	       formats[bitmap->format].format,
	       formats[bitmap->format].type,
	       NULL);
  glEnable(GL_TEXTURE_RECTANGLE_EXT);

  return bitmap;
}

void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap) {
  if (bitmap == NULL) {
    return;
  }

  glDeleteTextures(1, &bitmap->texture);
  free(bitmap);
}

bool glgfx_bitmap_select(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_waitblit(struct glgfx_bitmap* bitmap);

