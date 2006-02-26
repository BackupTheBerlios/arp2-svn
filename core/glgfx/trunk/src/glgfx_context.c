
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx.h"
#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_glxemul.h"
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
    static int const pb_attribs[] = { 
      // ATI's driver need these to be set to non-zero, at least in
      // version "1.3.5461 (X4.3.0-8.19.10)"
      GLX_PBUFFER_WIDTH,  1,
      GLX_PBUFFER_HEIGHT, 1,
      None
    };

    context->glx_pbuffer = glXCreatePbuffer(monitor->display, 
					    monitor->fb_config[0],
					    pb_attribs);

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
/*       if (context != context->monitor->main_context &&  */
/* 	  context->monitor->main_context != NULL) { */
/* 	// Switch to monitor context if valid */
/* 	glgfx_context_select(context->monitor->main_context); */
/*       } */
/*       else { */
	glXMakeContextCurrent(context->monitor->display, None, None, NULL);
	current_context = NULL;
/*       } */
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


bool glgfx_context_select(struct glgfx_context* context) {
  if (context == NULL || context->monitor == NULL) {
    errno = EINVAL;
    return false;
  }

  bool rc = true;

  if (current_context != context) {
    pthread_mutex_lock(&glgfx_mutex);
    
    GLXDrawable drawable = context->glx_pbuffer != 0 ?
      context->glx_pbuffer : context->monitor->glx_window;

    if (glXMakeContextCurrent(context->monitor->display, 
			      drawable, drawable, 
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


bool glgfx_context_unselect(void) {
  if (current_context == NULL) {
    return false;
  }

  glXMakeContextCurrent(current_context->monitor->display, None, None, NULL);
  return true;
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
      BUG("FBO incomplete! (%x)\n", 
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
    context->fbo_bitmap = NULL;
  }

  if (context->fbo_width != context->monitor->mode.hdisplay ||
      context->fbo_height != context->monitor->mode.vdisplay) {
    context->fbo_width = context->monitor->mode.hdisplay;
    context->fbo_height = context->monitor->mode.vdisplay;

    // Make sure the viewport and projection are correct
    glViewport(0, 0, 
	       context->monitor->mode.hdisplay, 
	       context->monitor->mode.vdisplay);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, context->monitor->mode.hdisplay, 
	    context->monitor->mode.vdisplay, 0, -1, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }

  return true;
}


bool glgfx_context_bindtex(struct glgfx_context* context,
			   struct glgfx_bitmap* bitmap) {
  if (context == NULL) {
    errno = EINVAL;
    return false;
  }

  if (/* bitmap->has extra bitmaps*/ false) {
    glActiveTexture(GL_TEXTURE1);
    context->tex_enable[1] = true;

    if (context->tex_bitmap != bitmap) {
      /* glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->secondary_texture); */
      GLGFX_CHECKERROR();
    }
  }

  glActiveTexture(GL_TEXTURE0);
  context->tex_enable[0] = true;

  if (context->tex_bitmap != bitmap) {
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, bitmap->texture);
    GLGFX_CHECKERROR();
    context->tex_bitmap = bitmap;
  }

  return true;
}


bool glgfx_context_unbindtex(struct glgfx_context* context) {
  if (context == NULL) {
    errno = EINVAL;
    return false;
  }

  if (context->tex_enable[1]) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

    context->tex_enable[1] = false;
  }

  if (context->tex_enable[0]) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

    context->tex_enable[0] = false;
  }

  context->tex_bitmap = NULL;

  return true;
}


bool glgfx_context_bindprogram(struct glgfx_context* context,
			       struct shader* shader) {
  if (context == NULL) {
    errno = EINVAL;
    return false;
  }

  // Defaults
  enum glgfx_pixel_format src = glgfx_pixel_format_a8r8g8b8;
  enum glgfx_pixel_format dst = context->monitor->format;

  if (context->tex_bitmap != NULL) {
    src = context->tex_bitmap->format;
  }

  if (context->fbo_bitmap != NULL) {
    dst = context->fbo_bitmap->format;
  }

  GLuint program = glgfx_shader_getprogram(src, dst, shader);

  if (program == 0) {
    return false;
  }

  glUseProgram(program);
  GLGFX_CHECKERROR();

  context->program = program;

  return true;
}


bool glgfx_context_unbindprogram(struct glgfx_context* context) {
  if (context == NULL) {
    errno = EINVAL;
    return false;
  }

  if (context->program != 0) {
    glUseProgram(0);
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


