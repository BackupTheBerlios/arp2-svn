
#include "glgfx-config.h"
#include <stdlib.h>
#include <glib.h>
#include <GL/gl.h>

#include "glgfx.h"
#include "glgfx_intern.h"
#include "glgfx_view.h"
#include "glgfx_viewport.h"

struct glgfx_view {
    GList* viewports;
    GList* sprites;
};

struct glgfx_view* glgfx_view_create(void) {
  struct glgfx_view* view;

  view = calloc(1, sizeof (*view));

  if (view == NULL) {
    return NULL;
  }

  return view;
}


void glgfx_view_destroy(struct glgfx_view* view) {
  if (view == NULL) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);

  g_list_free(view->viewports);
  free(view);

  pthread_mutex_unlock(&glgfx_mutex);
}

bool glgfx_view_addviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport) {
  if (view == NULL || viewport == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  view->viewports = g_list_append(view->viewports, viewport);
  pthread_mutex_unlock(&glgfx_mutex);

  return true;
}

bool glgfx_view_remviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport) {
  if (view == NULL || viewport == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  view->viewports = g_list_remove(view->viewports, viewport);
  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

int glgfx_view_numviewports(struct glgfx_view* view) {
  int res;
  
  if (view == NULL) {
    return 0;
  }

  pthread_mutex_lock(&glgfx_mutex);

  res = g_list_length(view->viewports);

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}


bool glgfx_view_addsprite(struct glgfx_view* view,
			  struct glgfx_sprite* sprite) {
  if (view == NULL || sprite == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  view->sprites = g_list_append(view->sprites, sprite);
  pthread_mutex_unlock(&glgfx_mutex);

  return true;
}

bool glgfx_view_remsprite(struct glgfx_view* view,
			    struct glgfx_sprite* sprite) {
  if (view == NULL || sprite == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  view->sprites = g_list_remove(view->sprites, sprite);
  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

int glgfx_view_numsprites(struct glgfx_view* view) {
  int res;
  
  if (view == NULL) {
    return 0;
  }

  pthread_mutex_lock(&glgfx_mutex);

  res = g_list_length(view->sprites);

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}



bool glgfx_view_render(struct glgfx_view* view) {

  void render_viewport(gpointer* data, gpointer* userdata) {
    struct glgfx_viewport* viewport = (struct glgfx_viewport*) data;
    struct glgfx_view* view = (struct glgfx_view*) userdata;

    (void) view;
    glgfx_viewport_render(viewport);
  }

  g_list_foreach(view->viewports, (GFunc) render_viewport, view);
  return true;
}


bool glgfx_view_rendersprites(struct glgfx_view* view) {

  void render_sprites(gpointer* data, gpointer* userdata) {
    struct glgfx_sprite* sprite = (struct glgfx_sprite*) data;
    struct glgfx_view* view = (struct glgfx_view*) userdata;

    (void) view;
    glgfx_sprite_render(sprite);
  }

  g_list_foreach(view->sprites, (GFunc) render_sprites, view);
  return true;
}
