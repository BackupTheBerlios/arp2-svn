
#include <stdlib.h>
#include <glib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx.h"
#include "glgfx_bitmap.h"
#include "glgfx_viewport.h"

struct glgfx_view* glgfx_view_create(struct glgfx_monitor* monitor) {
  struct glgfx_view* view;

  if (monitor == NULL) {
    return NULL;
  }

  view = calloc(1, sizeof (*view));

  if (view == NULL) {
    return NULL;
  }

  view->monitor = monitor;

  return view;
}


void glgfx_view_destroy(struct glgfx_view* view) {
  if (view == NULL) {
    return;
  }

  g_list_free(view->viewports);
  free(view);
}

bool glgfx_view_addviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport) {
  if (view == NULL || viewport == NULL) {
    return NULL;
  }

  view->viewports = g_list_append(view->viewports, viewport);
  return rasinfo;
}

bool glgfx_view_remviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport) {
  if (view == NULL || viewport == NULL) {
    return false;
  }

  viewport->viewports = g_list_remove(view->viewports, viewport);
}

int glgfx_view_numviewports(struct glgfx_view* view) {
  if (view == NULL) {
    return 0;
  }

  return g_list_length(view->viewports);
}

bool glgfx_view_render(struct glgfx_view* view) {

  void render(gpointer* data, gpointer* userdata) {
    struct glgfx_viewport* viewport = (struct glgfx_viewport*) data;
    struct glgfx_view* view = (struct glgfx_view*) userdata;

    glDrawBuffer(GL_BACK);
    glClearColor( 0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glgfx_viewport_render(viewport);
  }

  g_list_foreach(view->viewports, (GFunc) render, view);

  return true;
}
