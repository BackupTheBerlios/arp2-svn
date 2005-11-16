
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx.h"
#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_monitor.h"
#include "glgfx_intern.h"

/** A thread-local variable that holds the current thread's context */
static __thread struct glgfx_context* current_context  = NULL;

struct glgfx_context* glgfx_context_create(struct glgfx_monitor* monitor) {
  struct glgfx_context* context;
  
  if (monitor == NULL) {
    errno = EINVAL;
    return NULL;
  }

  context = calloc(1, sizeof (*context));

  if (context == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  pthread_mutex_lock(&glgfx_mutex);

  context->monitor = monitor;

  context->glx_context = glXCreateNewContext(
    monitor->display, monitor->fb_config[0], GLX_RGBA_TYPE,
    monitor->main_context->glx_context,
    True);

  if (context->glx_context == 0 || 
      !glXIsDirect(monitor->display, context->glx_context)) {
    BUG("Failed to create a direct GL context.\n");
    glgfx_context_destroy(context);
    context = NULL;
    errno = ENOTSUP;
  }
  else {
    context->glx_pbuffer = glXCreatePbuffer(monitor->display, 
					    monitor->fb_config[0],
					    NULL);

    if (context->glx_pbuffer == 0) {
      BUG("Unable to make create Pbuffer for context!\n");
      glgfx_context_destroy(context);
      context = NULL;
      errno = ENOTSUP;
    }
    else {
      if (!glXMakeContextCurrent(monitor->display, 
				 context->glx_pbuffer, context->glx_pbuffer,
				 context->glx_context)) {
	BUG("Unable to make GL Pbuffer context current!\n");
	glgfx_context_destroy(context);
	context = NULL;
	errno = ENOTSUP;
      }
      else {
	glGenFramebuffersEXT(1, &context->fbo);

	if (context->fbo == 0) {
	  BUG("Unable to create framebuffer_object!\n");
	  glgfx_context_destroy(context);
	  context = NULL;
	  errno = ENOTSUP;
	}
	else {
	  // Setup a standard integer 2D coordinate system
	  glDrawBuffer(GL_BACK);
	  glViewport(0, 0, monitor->mode.hdisplay, monitor->mode.vdisplay);
	  glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();
	  glOrtho(0, monitor->mode.hdisplay, monitor->mode.vdisplay, 0, -1, 0);
	  glMatrixMode(GL_MODELVIEW);
	  glLoadIdentity();

	  // Fix OpenGL's weired default alignment
	  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	  glPixelStorei(GL_PACK_ALIGNMENT, 1);
	
	  GLGFX_CHECKERROR();
	}
      }
    }
  }


  // Make the newly created context active
  if (context != NULL) {
    current_context = context;
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return context;
}


bool glgfx_context_select(struct glgfx_context* context) {
  if (context == NULL || context->monitor == NULL) {
    errno = EINVAL;
    return false;
  }

  bool rc = true;

  if (current_context != context) {
    pthread_mutex_lock(&glgfx_mutex);

    if (glXMakeContextCurrent(context->monitor->display, 
			      context->monitor->glx_window, 
			      context->monitor->glx_window, 
			      context->glx_context)) {
      current_context = context;
    }
    else {
      errno = EINVAL;
      rc = false;
    }

    pthread_mutex_unlock(&glgfx_mutex);
  }

  return rc;
}


struct glgfx_context* glgfx_context_getcurrent(void) {
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

  // Make sure the FBO is bound
  if (!context->fbo_bound) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, context->fbo);
    check = true;
  }

  // Make sure the bitmap is attached to buffer 0
  if (context->fbo_bitmap != bitmap) {
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_RECTANGLE_ARB, bitmap->texture, 0);
    check = true;
  }

  if (check) {
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != 
	GL_FRAMEBUFFER_COMPLETE_EXT) {
      BUG("FBO incomplete! (%d)\n", 
	  glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT));
      rc = false;
    }
    else {
      context->fbo_bound  = true;
      context->fbo_bitmap = bitmap;
    }
  }

  // Make sure the viewport and projection are correct
  if (context->fbo_width != bitmap->width ||
      context->fbo_height != bitmap->height) {
    glViewport(0, 0, bitmap->width, bitmap->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, bitmap->width, bitmap->height, 0, -1, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    context->fbo_width = bitmap->width;
    context->fbo_height = bitmap->height;
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


struct glgfx_bitmap* glgfx_context_gettempbitmap(struct glgfx_context* context,
						 int min_width, 
						 int min_height, 
						 enum glgfx_pixel_format format) {
  if (min_width < 0 || min_height < 0 ||
      format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max) {
    return NULL;
  }

  if (context->temp_bitmaps[format] == NULL ||
      context->temp_bitmaps[format]->width < min_width ||
      context->temp_bitmaps[format]->height < min_height) {

    glgfx_bitmap_destroy(context->temp_bitmaps[format]);

    context->temp_bitmaps[format] = glgfx_bitmap_create(
      glgfx_bitmap_attr_width,  min_width,
      glgfx_bitmap_attr_height, min_height,
      glgfx_bitmap_attr_format, format,
      glgfx_tag_end);
  }

  return context->temp_bitmaps[format];
}


bool glgfx_context_destroy(struct glgfx_context* context) {
  if (context == NULL || context->monitor == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  size_t i;

  for (i = 0; 
       i < sizeof (context->temp_bitmaps) / sizeof (context->temp_bitmaps[0]);
       ++i) {
    glgfx_bitmap_destroy(context->temp_bitmaps[i]);
  }

  if (context->monitor != NULL) {
    if (current_context == context) {
      if (context != context->monitor->main_context && 
	  context->monitor->main_context != NULL) {
	// Switch to monitor context if valid
	glgfx_context_select(context->monitor->main_context);
      }
      else {
	glXMakeContextCurrent(context->monitor->display, None, None, NULL);
	current_context = NULL;
      }
    }

    if (context->glx_context != 0) {
      glXDestroyContext(context->monitor->display, context->glx_context);
    }
    
    if (context->glx_pbuffer != 0) {
      glXDestroyPbuffer(context->monitor->display, context->glx_pbuffer);
    }
  }
  
  if (context->fbo != 0) {
    glDeleteFramebuffersEXT(1, &context->fbo);
  }

  free(context);

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

