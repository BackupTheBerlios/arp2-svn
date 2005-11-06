
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

struct glgfx_viewport* glgfx_viewport_create_a(struct glgfx_tagitem const* tags) {
  struct glgfx_viewport* viewport;

  viewport = calloc(1, sizeof (*viewport));

  if (viewport == NULL) {
    return NULL;
  }

  if (!glgfx_viewport_setattrs_a(viewport, tags)) {
    glgfx_viewport_destroy(viewport);
    return NULL;
  }

  return viewport;
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


bool glgfx_viewport_setattrs_a(struct glgfx_viewport* viewport,
			       struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;
  bool rc = true;

  if (viewport == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_viewport_attr) tag->tag) {
      case glgfx_viewport_attr_width:
	viewport->width = tag->data;
	break;

      case glgfx_viewport_attr_height:
	viewport->height = tag->data;
	break;

      case glgfx_viewport_attr_x:
	viewport->xoffset = tag->data;
	break;

      case glgfx_viewport_attr_y:
	viewport->yoffset = tag->data;
	break;

      case glgfx_viewport_attr_unknown:
      case glgfx_viewport_attr_max:
	/* Make compiler happy */
	break;
    }
  }

  if (viewport->width <= 0 || 
      viewport->height <= 0) {
    rc = false;
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


bool glgfx_viewport_getattr(struct glgfx_viewport* viewport,
			    enum glgfx_viewport_attr attr,
			    intptr_t* storage) {
  if (viewport == NULL || storage == NULL ) {
    return false;
  }

  switch (attr) {
    case glgfx_viewport_attr_width:
      *storage = viewport->width;
      break;

    case glgfx_viewport_attr_height:
      *storage = viewport->height;
      break;

    case glgfx_viewport_attr_x:
      *storage = viewport->xoffset;
      break;

    case glgfx_viewport_attr_y:
      *storage = viewport->yoffset;
      break;

    default:
      return false;
  }

  return true;
}


struct glgfx_rasinfo* glgfx_viewport_addbitmap_a(struct glgfx_viewport* viewport,
						 struct glgfx_bitmap* bitmap,
						 struct glgfx_tagitem const* tags) {
  struct glgfx_rasinfo* rasinfo;
  
  if (viewport == NULL || bitmap == NULL) {
    return NULL;
  }

  rasinfo = calloc(1, sizeof (*rasinfo));

  if (rasinfo == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&glgfx_mutex);

  struct glgfx_tagitem bm_tags[] = {
    { glgfx_rasinfo_attr_bitmap, (intptr_t) bitmap },
    { glgfx_tag_more,            (intptr_t) tags }
  };

  if (!glgfx_rasinfo_setattrs_a(rasinfo, bm_tags)) {
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

bool glgfx_rasinfo_setattrs_a(struct glgfx_rasinfo* rasinfo,
			      struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;

  if (rasinfo == NULL) {
    return false;
  }
  
  pthread_mutex_lock(&glgfx_mutex);

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_viewport_attr) tag->tag) {
      case glgfx_rasinfo_attr_width:
	rasinfo->width = tag->data;
	break;

      case glgfx_rasinfo_attr_height:
	rasinfo->height = tag->data;
	break;

      case glgfx_rasinfo_attr_x:
	rasinfo->xoffset = tag->data;
	break;

      case glgfx_rasinfo_attr_y:
	rasinfo->yoffset = tag->data;
	break;

      case glgfx_rasinfo_attr_bitmap:
	rasinfo->bitmap = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_rasinfo_attr_unknown:
      case glgfx_rasinfo_attr_max:
	/* Make compiler happy */
	break;
    }
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return rasinfo->bitmap != NULL;
}


bool glgfx_rasinfo_getattr(struct glgfx_rasinfo* rasinfo,
			    enum glgfx_rasinfo_attr attr,
			    intptr_t* storage) {
  if (rasinfo == NULL || storage == NULL ) {
    return false;
  }

  switch (attr) {
    case glgfx_rasinfo_attr_width:
      *storage = rasinfo->width;
      break;

    case glgfx_rasinfo_attr_height:
      *storage = rasinfo->height;
      break;

    case glgfx_rasinfo_attr_x:
      *storage = rasinfo->xoffset;
      break;

    case glgfx_rasinfo_attr_y:
      *storage = rasinfo->yoffset;
      break;

    case glgfx_rasinfo_attr_bitmap:
      *storage = (intptr_t) rasinfo->bitmap;
      break;

    default:
      return false;
  }

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
