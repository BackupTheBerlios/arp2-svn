
#define _GNU_SOURCE  // for PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#include "glgfx-config.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>

#include "glgfx.h"
#include "glgfx_glext.h"
#include "glgfx_monitor.h"
#include "glgfx_intern.h"

static __thread struct glgfx_context* current_context  = NULL;

pthread_mutex_t glgfx_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

int                   glgfx_num_monitors;
struct glgfx_monitor* glgfx_monitors[max_monitors];


struct glgfx_tagitem const* glgfx_nexttagitem(struct glgfx_tagitem const** taglist_ptr) {
  if (taglist_ptr == NULL || *taglist_ptr == NULL) {
    return NULL;
  }

  while (true) {
    switch ((*taglist_ptr)->tag) {
      case glgfx_tag_end:
	*taglist_ptr = NULL;
	return NULL;

      case glgfx_tag_more:
	*taglist_ptr = (struct glgfx_tagitem const*) (*taglist_ptr)->data;
	if (*taglist_ptr == NULL) {
	  return NULL;
	}
	break;

      case glgfx_tag_ignore:
	++(*taglist_ptr);
	break;

      case glgfx_tag_skip:
	*taglist_ptr += (*taglist_ptr)->data + 1;
	break;

      default: {
	struct glgfx_tagitem const* res = *taglist_ptr;

	++(*taglist_ptr);
	return res;
      }
    }
  }
}


int glgfx_getattrs_a(void* object, 
		     glgfx_getattr_proto* fn, 
		     struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;
  int count = 0;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    if (fn(object, tag->tag, (intptr_t*) tag->data)) {
      ++count;
    }
  }

  return count;
}


bool glgfx_init_a(struct glgfx_tagitem const* tags) {
  (void) tags;
  return true;
}


void glgfx_cleanup() {
}


int glgfx_createmonitors_a(struct glgfx_tagitem const* tags) {
  struct glgfx_monitor* friend = NULL;
  struct glgfx_tagitem const* tag;

  pthread_mutex_lock(&glgfx_mutex);
  
  glgfx_destroymonitors();

  glgfx_num_monitors = 0;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_create_monitors_tag) tag->tag) {

      case glgfx_create_monitors_tag_display: {
	char const* name = (char const*) tag->data;

	glgfx_monitors[glgfx_num_monitors] = glgfx_monitor_create(name, friend);

	if (glgfx_monitors[glgfx_num_monitors] != NULL) {
	  friend = glgfx_monitors[glgfx_num_monitors];
      
	  ++glgfx_num_monitors;
	}

	break;
      }

      case glgfx_create_monitors_tag_unknown:
      case glgfx_create_monitors_tag_max:
	/* Make compiler happy */
	break;
    }
  }
 
  pthread_mutex_unlock(&glgfx_mutex);
  return glgfx_num_monitors;
}


void glgfx_destroymonitors(void) {
  int i;

  pthread_mutex_lock(&glgfx_mutex);

  for (i = 0; i < glgfx_num_monitors; ++i) {
    glgfx_monitor_destroy(glgfx_monitors[i]);
    glgfx_monitors[i] = NULL;
  }

  glgfx_num_monitors = 0;

  pthread_mutex_unlock(&glgfx_mutex);
}


struct glgfx_context* glgfx_context_create(struct glgfx_monitor* monitor) {
  struct glgfx_context* context;

  context = glgfx_monitor_createcontext(monitor);

  // Make the newly created context active
  if (context != NULL) {
    glgfx_context_select(context);
  }

  return context;
}


bool glgfx_context_select(struct glgfx_context* context) {
  if (context == NULL || context->monitor == NULL) {
    errno = EINVAL;
    return false;
  }

  bool rc;

  if (current_context != context) {
    pthread_mutex_lock(&glgfx_mutex);

    if (glXMakeCurrent(context->monitor->display,
		       context->monitor->window,
		       context->glx_context)) {
      current_context = context;
      rc = true;
    }
    else {
      errno = EINVAL;
      rc = false;
    }

    pthread_mutex_unlock(&glgfx_mutex);
  }
  else {
    rc = true;
  }

  return rc;
}


struct glgfx_context* glgfx_context_getcurrent(void) {
  if (current_context == NULL) {
    BUG("No current context!\n");
    abort();
  }

  return current_context;
}

bool glgfx_context_bindfbo(struct glgfx_context* context,
			   struct glgfx_bitmap* bitmap) {
  bool check = false;
  bool rc = true;

  if (context == NULL) {
    errno = EINVAL;
    return false;
  }

  if (!context->fbo_bound) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, context->fbo);
    check = true;
  }

  if (context->fbo_bitmap != bitmap) {
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, 
			      bitmap != NULL ? bitmap->texture : 0,
			      0);
    check = true;
  }
  
  if (check) {
    if (bitmap != NULL &&
	glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
      BUG("FBO incomplete! (%d)\n", glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT));
      rc = false;
    }
    else {
      context->fbo_bound  = true;
      context->fbo_bitmap = bitmap;
    }
  }

  return rc;
}


bool glgfx_context_unbindfbo(struct glgfx_context* context) {
  if (context == NULL) {
    errno = EINVAL;
    return false;
  }

  if (context->fbo_bound) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    context->fbo_bound = false;
  }

  return true;
}


bool glgfx_context_destroy(struct glgfx_context* context) {
  if (context == NULL || context->monitor == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  if (context->monitor != NULL) {
    if (current_context == context) {
      glXMakeCurrent(context->monitor->display, None, NULL);
      current_context = NULL;
    }

    glXDestroyContext(context->monitor->display, context->glx_context);
  }
  
  if (context->fbo != 0) {
    glDeleteFramebuffersEXT(1, &context->fbo);
  }

  if (context->extensions != NULL) {
    void cleanup(gpointer key, gpointer value, gpointer userdata) {
      (void) value;
      (void) userdata;
      free(key);
    }

    g_hash_table_foreach(context->extensions, cleanup, NULL);
    g_hash_table_destroy(context->extensions);
  }

  free(context);

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


void glgfx_checkerror(char const* func, char const* file, int line) {
  GLenum error = glGetError();

  if (error != 0) {
    char const* msg = (char const*) gluErrorString(error);
    
    BUG("OpenGL error %d %s:%d (%s): %s\n", error, file, line, func, msg);
    abort();
  }
}

