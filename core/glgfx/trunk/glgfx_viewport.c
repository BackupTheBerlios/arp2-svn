
#include "glgfx-config.h"
#include <stdlib.h>
#include <glib.h>
#include <GL/gl.h>

#include "glgfx.h"
#include "glgfx_bitmap.h"
#include "glgfx_viewport.h"
#include "glgfx_intern.h"

#ifndef GL_TEXTURE_RECTANGLE_ARB
# define GL_TEXTURE_RECTANGLE_ARB GL_TEXTURE_RECTANGLE_EXT
#endif

struct glgfx_viewport* glgfx_viewport_create(int width, int height,
					     int xoffset, int yoffset) {
  struct glgfx_viewport* viewport;

  viewport = calloc(1, sizeof (*viewport));

  if (viewport == NULL) {
    return NULL;
  }

  if (!glgfx_viewport_move(viewport, width, height, xoffset, yoffset)) {
    glgfx_viewport_destroy(viewport);
    return NULL;
  }

  return viewport;
}

bool glgfx_viewport_move(struct glgfx_viewport* viewport,
			 int width, int height,
			 int xoffset, int yoffset) {
  if (viewport == NULL || width <= 0 || height <= 0) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  viewport->width = width;
  viewport->height = height;
  viewport->xoffset = xoffset;
  viewport->yoffset = yoffset;

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


void glgfx_viewport_destroy(struct glgfx_viewport* viewport) {
  if (viewport == NULL) {
    return;
  }

  void free_rasinfo(gpointer* data, gpointer* userdata __attribute((unused))) {
    free(data);
  }
  
  pthread_mutex_lock(&glgfx_mutex);

  g_list_foreach(viewport->rasinfos, (GFunc) free_rasinfo, NULL);
  g_list_free(viewport->rasinfos);
  free(viewport);

  pthread_mutex_unlock(&glgfx_mutex);
}

struct glgfx_rasinfo* glgfx_viewport_addbitmap(struct glgfx_viewport* viewport,
					       struct glgfx_bitmap* bitmap,
					       int xoffset, int yoffset,
					       int width, int height) {
  struct glgfx_rasinfo* rasinfo;
  
  if (viewport == NULL || bitmap == NULL) {
    return NULL;
  }

  rasinfo = calloc(1, sizeof (*rasinfo));

  if (rasinfo == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&glgfx_mutex);

  if (!glgfx_viewport_setbitmap(viewport, rasinfo, bitmap,
				xoffset, yoffset, width, height)) {
    free(rasinfo);
    pthread_mutex_unlock(&glgfx_mutex);
    return NULL;
  }
  
  viewport->rasinfos = g_list_append(viewport->rasinfos, rasinfo);

  pthread_mutex_unlock(&glgfx_mutex);
  return rasinfo;
}

bool glgfx_viewport_rembitmap(struct glgfx_viewport* viewport,
			      struct glgfx_rasinfo* rasinfo) {
  if (viewport == NULL || rasinfo == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  viewport->rasinfos = g_list_remove(viewport->rasinfos, rasinfo);
  free(rasinfo);

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

bool glgfx_viewport_setbitmap(struct glgfx_viewport* viewport,
			      struct glgfx_rasinfo* rasinfo,
			      struct glgfx_bitmap* bitmap,
			      int xoffset, int yoffset,
			      int width, int height) {
  if (viewport == NULL || rasinfo == NULL || bitmap == NULL) {
    return false;
  }
  
  pthread_mutex_lock(&glgfx_mutex);

  rasinfo->bitmap = bitmap;
  rasinfo->xoffset = xoffset;
  rasinfo->yoffset = yoffset;
  rasinfo->width = width;
  rasinfo->height = height;

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

int glgfx_viewport_numbitmaps(struct glgfx_viewport* viewport) {
  int res;
  
  if (viewport == NULL) {
    return 0;
  }

  pthread_mutex_lock(&glgfx_mutex);

  res = g_list_length(viewport->rasinfos);

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}

bool glgfx_viewport_render(struct glgfx_viewport* viewport) {

  void render(gpointer* data, gpointer* userdata) {
    struct glgfx_rasinfo* rasinfo = (struct glgfx_rasinfo*) data;
    struct glgfx_viewport* viewport = (struct glgfx_viewport*) userdata;

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rasinfo->bitmap->texture);
    glColor4f(1,1,1,1);
    glBegin(GL_QUADS);
    glTexCoord2i(rasinfo->xoffset,
		 rasinfo->yoffset);
    glVertex3f(viewport->xoffset,
	       viewport->yoffset, 0);
    glTexCoord2i(rasinfo->xoffset + rasinfo->width,
		 rasinfo->yoffset);
    glVertex3f(viewport->xoffset + viewport->width,
	       viewport->yoffset, 0);
    glTexCoord2i(rasinfo->xoffset + rasinfo->width,
		 rasinfo->yoffset + rasinfo->height);
    glVertex3f(viewport->xoffset + viewport->width,
	       viewport->yoffset + viewport->height, 0);
    glTexCoord2i(rasinfo->xoffset,
		 rasinfo->yoffset + rasinfo->height);
    glVertex3f(viewport->xoffset,
	       viewport->yoffset + viewport->height, 0);
    glEnd();
  }

  pthread_mutex_lock(&glgfx_mutex);

  g_list_foreach(viewport->rasinfos, (GFunc) render, viewport);

  pthread_mutex_unlock(&glgfx_mutex);
  
  return true;
}
