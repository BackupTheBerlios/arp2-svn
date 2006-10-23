
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>
#include <glib.h>
#include <GL/gl.h>

#include "glgfx.h"
#include "glgfx_intern.h"
#include "glgfx_view.h"
#include "glgfx_viewport.h"

struct glgfx_view* glgfx_view_create(void) {
  struct glgfx_view* view;

  view = calloc(1, sizeof (*view));

  if (view == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  view->viewports = g_queue_new();
  view->sprites   = g_queue_new();

  if (view->viewports == NULL || view->sprites == NULL) {
    glgfx_view_destroy(view);
    errno = ENOMEM;
    return NULL;
  }

  view->has_changed = true;

  return view;
}


void glgfx_view_destroy(struct glgfx_view* view) {
  if (view == NULL) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);

  g_queue_free(view->viewports);
  g_queue_free(view->sprites);
  free(view);

  pthread_mutex_unlock(&glgfx_mutex);
}

bool glgfx_view_addviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport) {
  if (view == NULL || viewport == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  g_queue_push_head(view->viewports, viewport);
  view->has_changed = true;
  pthread_mutex_unlock(&glgfx_mutex);

  return true;
}

bool glgfx_view_remviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport) {
  if (view == NULL || viewport == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  g_queue_remove(view->viewports, viewport);
  view->has_changed = true;
  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

int glgfx_view_numviewports(struct glgfx_view* view) {
  int res;
  
  if (view == NULL) {
    return 0;
  }

  pthread_mutex_lock(&glgfx_mutex);

  res = g_queue_get_length(view->viewports);

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}


bool glgfx_view_addsprite(struct glgfx_view* view,
			  struct glgfx_sprite* sprite) {
  if (view == NULL || sprite == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  g_queue_push_head(view->sprites, sprite);
  view->has_changed = true;
  pthread_mutex_unlock(&glgfx_mutex);

  return true;
}

bool glgfx_view_remsprite(struct glgfx_view* view,
			    struct glgfx_sprite* sprite) {
  if (view == NULL || sprite == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  g_queue_remove(view->sprites, sprite);
  view->has_changed = true;
  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

int glgfx_view_numsprites(struct glgfx_view* view) {
  int res;
  
  if (view == NULL) {
    return 0;
  }

  pthread_mutex_lock(&glgfx_mutex);

  res = g_queue_get_length(view->sprites);

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}


static void check_viewport(gpointer data, gpointer userdata) {
  struct glgfx_viewport* viewport = (struct glgfx_viewport*) data;
  bool* has_changed_ptr = (bool*) userdata;

  if (glgfx_viewport_haschanged(viewport)) {
    *has_changed_ptr = true;
  }
}

static void check_sprite(gpointer data, gpointer userdata) {
  struct glgfx_sprite* sprite = (struct glgfx_sprite*) data;
  bool* has_changed_ptr = (bool*) userdata;

  if (glgfx_sprite_haschanged(sprite)) {
    *has_changed_ptr = true;
  }
}

bool glgfx_view_haschanged(struct glgfx_view* view) {
  bool has_changed = view->has_changed;

  view->has_changed = false;

  // Always call glgfx_viewport/sprite_haschanged, since we want to
  // reset their has_changed flags.
  g_queue_foreach(view->viewports, check_viewport, &has_changed);
  g_queue_foreach(view->sprites, check_sprite, &has_changed);

  return has_changed;
}


bool glgfx_view_render(struct glgfx_view* view, struct glgfx_hook* mode_hook, bool front_to_back) {
  GList* node;

  if (front_to_back) {
    for (node = g_queue_peek_head_link(view->viewports);
	 node != NULL; node = node->next) {
      glgfx_viewport_render(node->data, mode_hook, front_to_back);
    }
  }
  else {
    for (node = g_queue_peek_tail_link(view->viewports);
	 node != NULL; node = node->prev) {
      glgfx_viewport_render(node->data, mode_hook, front_to_back);
    }
  }
  return true;
}


bool glgfx_view_rendersprites(struct glgfx_view* view) {
  GList* node;

  // Sprites are always rendered back-to-front
  for (node = g_queue_peek_tail_link(view->sprites);
       node != NULL; node = node->prev) {
    glgfx_sprite_render(node->data);
  }

  return true;
}
