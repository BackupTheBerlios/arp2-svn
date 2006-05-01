
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "glgfx.h"
#include "glgfx_bitmap.h"
#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_viewport.h"
#include "glgfx_intern.h"

struct glgfx_viewport* glgfx_viewport_create_a(struct glgfx_tagitem const* tags) {
  struct glgfx_viewport* viewport;

  viewport = calloc(1, sizeof (*viewport));

  if (viewport == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  viewport->rasinfos = g_queue_new();

  if (viewport->rasinfos == NULL) {
    glgfx_viewport_destroy(viewport);
    return NULL;
  }

  if (!glgfx_viewport_setattrs_a(viewport, tags)) {
    glgfx_viewport_destroy(viewport);
    return NULL;
  }
  
  viewport->has_changed = true;

  return viewport;
}


static void free_rasinfo(gpointer data, gpointer userdata) {
  (void) userdata;

  free(data);
}

void glgfx_viewport_destroy(struct glgfx_viewport* viewport) {
  if (viewport == NULL) {
    return;
  }

  
  pthread_mutex_lock(&glgfx_mutex);

  g_queue_foreach(viewport->rasinfos, free_rasinfo, NULL);
  g_queue_free(viewport->rasinfos);
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

  viewport->has_changed = true;

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

  rasinfo->blend_eq             = glgfx_blend_equation_disabled;
  rasinfo->blend_eq_alpha       = glgfx_blend_equation_unknown;
  rasinfo->blend_func_src       = glgfx_blend_func_srcalpha;
  rasinfo->blend_func_src_alpha = glgfx_blend_func_unknown;
  rasinfo->blend_func_dst       = glgfx_blend_func_srcalpha_inv;
  rasinfo->blend_func_dst_alpha = glgfx_blend_func_unknown;

  struct glgfx_tagitem bm_tags[] = {
    { glgfx_rasinfo_attr_bitmap, (intptr_t) bitmap },
    { glgfx_rasinfo_attr_width,  bitmap->width     },
    { glgfx_rasinfo_attr_height, bitmap->height    },
    { glgfx_tag_more,            (intptr_t) tags   }
  };

  if (!glgfx_rasinfo_setattrs_a(rasinfo, bm_tags)) {
    free(rasinfo);
    return NULL;
  }

  pthread_mutex_lock(&glgfx_mutex);
  
  g_queue_push_head(viewport->rasinfos, rasinfo);

  viewport->has_changed = true;

  pthread_mutex_unlock(&glgfx_mutex);
  return rasinfo;
}


bool glgfx_viewport_rembitmap(struct glgfx_viewport* viewport,
			      struct glgfx_rasinfo* rasinfo) {
  if (viewport == NULL || rasinfo == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  g_queue_remove(viewport->rasinfos, rasinfo);
  free(rasinfo);

  viewport->has_changed = true;

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
    switch ((enum glgfx_rasinfo_attr) tag->tag) {
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

      case glgfx_rasinfo_attr_blend_equation:
	rasinfo->blend_eq = tag->data;
	break;
	
      case glgfx_rasinfo_attr_blend_equation_alpha:
	rasinfo->blend_eq_alpha = tag->data;
	break;

      case glgfx_rasinfo_attr_blend_srcfunc:
	rasinfo->blend_func_src = tag->data;
	break;

      case glgfx_rasinfo_attr_blend_srcfunc_alpha:
	rasinfo->blend_func_src_alpha = tag->data;
	break;

      case glgfx_rasinfo_attr_blend_dstfunc:
	rasinfo->blend_func_dst = tag->data;
	break;

      case glgfx_rasinfo_attr_blend_dstfunc_alpha:
	rasinfo->blend_func_dst_alpha = tag->data;
	break;

      case glgfx_rasinfo_attr_unknown:
      case glgfx_rasinfo_attr_max:
	/* Make compiler happy */
	break;
    }
  }

  if (rasinfo->blend_eq_alpha == glgfx_blend_equation_unknown ||
      rasinfo->blend_eq_alpha == glgfx_blend_equation_disabled) {
    rasinfo->blend_eq_alpha = rasinfo->blend_eq;
  }

  if (rasinfo->blend_func_src_alpha == glgfx_blend_func_unknown) {
    rasinfo->blend_func_src_alpha = rasinfo->blend_func_src;
  }

  if (rasinfo->blend_func_dst_alpha == glgfx_blend_func_unknown) {
    rasinfo->blend_func_dst_alpha = rasinfo->blend_func_dst;
  }

  rasinfo->has_changed = true;

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

    case glgfx_rasinfo_attr_blend_equation:
      *storage = (intptr_t) rasinfo->blend_eq;
      break;
	
    case glgfx_rasinfo_attr_blend_equation_alpha:
      *storage = (intptr_t) rasinfo->blend_eq_alpha;
      break;

    case glgfx_rasinfo_attr_blend_srcfunc:
      *storage = (intptr_t) rasinfo->blend_func_src;
      break;

    case glgfx_rasinfo_attr_blend_srcfunc_alpha:
      *storage = (intptr_t) rasinfo->blend_func_src_alpha;
      break;

    case glgfx_rasinfo_attr_blend_dstfunc:
      *storage = (intptr_t) rasinfo->blend_func_dst;
      break;

    case glgfx_rasinfo_attr_blend_dstfunc_alpha:
      *storage = (intptr_t) rasinfo->blend_func_dst_alpha;
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

  res = g_queue_get_length(viewport->rasinfos);

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}


bool glgfx_viewport_orderbitmap(struct glgfx_viewport* viewport,
				struct glgfx_rasinfo* rasinfo,
				struct glgfx_rasinfo* in_front_of) {
  bool res = true;

  if (viewport == NULL || rasinfo == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  g_queue_remove(viewport->rasinfos, rasinfo);

  if (in_front_of == (struct glgfx_rasinfo*) -1) {
    g_queue_push_head(viewport->rasinfos, rasinfo);
  }
  else {
    GList* sib = g_queue_find(viewport->rasinfos, in_front_of);

    g_queue_insert_before(viewport->rasinfos, sib, rasinfo);
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}


static void check(gpointer data, gpointer userdata) {
  struct glgfx_rasinfo* rasinfo = (struct glgfx_rasinfo*) data;
  bool* has_changed_ptr = (bool*) userdata;

  if (glgfx_bitmap_haschanged(rasinfo->bitmap) || rasinfo->has_changed) {
    *has_changed_ptr = true;
  }

  rasinfo->has_changed = false;
}

bool glgfx_viewport_haschanged(struct glgfx_viewport* viewport) {
  bool has_changed = viewport->has_changed;

  viewport->has_changed = false;

  // Always call glgfx_bitmap_haschanged, since we want to reset its
  // has_changed flag.
  g_queue_foreach(viewport->rasinfos, check, &has_changed);

  return has_changed;
}


struct render_data {
    GLenum tex_unit;
};

static unsigned long render(struct glgfx_hook* hook,
			    struct glgfx_rasinfo* rasinfo, 
			    struct glgfx_viewport_rendermsg* msg) {
  struct render_data*    data     = hook->data;
  struct glgfx_viewport* viewport = msg->viewport;;

  // Let the hardware worry about clipping to the viewport, if required.
  glBegin(GL_QUADS); {
    glMultiTexCoord2i(data->tex_unit, 
		      0, 
		      0);
    glVertex3f(viewport->xoffset + -rasinfo->xoffset, 
	       viewport->yoffset + -rasinfo->yoffset, 
	       msg->z);

    glMultiTexCoord2i(data->tex_unit,
		      rasinfo->bitmap->width,
		      0);
    glVertex3f(viewport->xoffset + -rasinfo->xoffset + rasinfo->width,
	       viewport->yoffset + -rasinfo->yoffset,
	       msg->z);

    glMultiTexCoord2i(data->tex_unit,
		      rasinfo->bitmap->width,
		      rasinfo->bitmap->height);
    glVertex3f(viewport->xoffset + -rasinfo->xoffset + rasinfo->width,
	       viewport->yoffset + -rasinfo->yoffset + rasinfo->height,
	       msg->z);

    glMultiTexCoord2i(data->tex_unit,
		      0,
		      rasinfo->bitmap->height);
    glVertex3f(viewport->xoffset + -rasinfo->xoffset,
	       viewport->yoffset + -rasinfo->yoffset + rasinfo->height,
	       msg->z);
  }
  glEnd();

  GLGFX_CHECKERROR();

  return 0;
}

bool glgfx_viewport_render(struct glgfx_viewport* viewport, 
			   struct glgfx_hook* mode_hook,
			   bool front_to_back) {
  struct glgfx_context* context = glgfx_context_getcurrent();

  struct render_data render_data = {
    -1
  };

  struct glgfx_hook geometry_hook = {
    (glgfx_hookfunc) render, &render_data
  };

  struct glgfx_viewport_rendermsg msg = {
    viewport, &geometry_hook, front_to_back ? 1.0f : 0.0f
  };


  glEnable(GL_SCISSOR_TEST);
  glScissor(viewport->xoffset, 
	    context->monitor->mode.vdisplay - viewport->yoffset - viewport->height,
	    viewport->width, 
	    viewport->height);

  GList* node;
  float  dz = ((front_to_back ? -1 : 1) * 1.0f / 
	       g_queue_get_length(viewport->rasinfos));
  
  for (node = front_to_back ? 
	 g_queue_peek_head_link(viewport->rasinfos) :
	 g_queue_peek_tail_link(viewport->rasinfos);
       node != NULL; 
       node = front_to_back ? node->next : node->prev) {
    struct glgfx_rasinfo* rasinfo = node->data;

    // Set up rendering mode and shaders (calls geometry_hook too)
    render_data.tex_unit = glgfx_context_bindtex(context, 0, rasinfo->bitmap);
    glgfx_callhook(mode_hook, rasinfo, &msg);

    msg.z += dz;
  }

  glDisable(GL_SCISSOR_TEST);

  return true;
}
