
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx_bitmap.h"
#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_intern.h"

static enum glgfx_pixel_format select_format(int bits,
					     struct glgfx_bitmap* friend,
					     enum glgfx_pixel_format format) {
  enum glgfx_pixel_format fmt = friend != NULL ? friend->format : format;

  if (fmt == glgfx_pixel_format_unknown) {
    // I'm not sure about this but well ...
    fmt = glgfx_pixel_getformat(
      glgfx_pixel_attr_rgb,       true,
      glgfx_pixel_attr_redbits,   bits/3,
      glgfx_pixel_attr_greenbits, bits/3,
      glgfx_pixel_attr_bluebits,  bits/3,
      glgfx_tag_end);
  }

  if (fmt <= glgfx_pixel_format_unknown || fmt >= glgfx_pixel_format_max) {
    fmt = glgfx_pixel_format_unknown;
//    abort();
  }

  return fmt;
}

static enum glgfx_pixel_format format_from_visualid(struct glgfx_monitor* monitor, 
						    VisualID id,
						    int* fbconfig_index,
						    bool* y_inverted) {
  *fbconfig_index = -1;
  *y_inverted = false;

  XVisualInfo template = {
    visualid : id
  };

  int i;

  for (i = 0; i < monitor->num_fbconfigs; ++i) {
    XVisualInfo* visinfo = glXGetVisualFromFBConfig(monitor->display, 
						    monitor->fbconfigs[i]);
    
    if (visinfo == NULL) {
      continue;
    }

    if (visinfo->visualid != id) {
      XFree(visinfo);
      continue;
    }

    XFree(visinfo);

    int value;
    glXGetFBConfigAttrib(monitor->display, monitor->fbconfigs[i],
			 GLX_DRAWABLE_TYPE, &value);
    
    if ((value & GLX_PIXMAP_BIT) == 0) {
      continue;
    }

    glXGetFBConfigAttrib(monitor->display, monitor->fbconfigs[i],
			 GLX_BIND_TO_TEXTURE_TARGETS_EXT, &value);
    if ((value & GLX_TEXTURE_RECTANGLE_BIT_EXT) == 0) {
      continue;
    }

    glXGetFBConfigAttrib(monitor->display, monitor->fbconfigs[i],
                              GLX_BIND_TO_TEXTURE_RGBA_EXT, &value);
    if (value == 0) {
      glXGetFBConfigAttrib(monitor->display, monitor->fbconfigs[i],
                                  GLX_BIND_TO_TEXTURE_RGB_EXT, &value);
      if (value == 0) {
	continue;
      }
    }

    glXGetFBConfigAttrib(monitor->display, monitor->fbconfigs[i],
			 GLX_Y_INVERTED_EXT, &value);
    *y_inverted = value;

    *fbconfig_index = i;
    break;
  }

  if (*fbconfig_index == -1) {
    BUG("Unable to find suitable FBConfig for visualid %lx\n", (long) id);
  }

  int items = 0;
  XVisualInfo* vinfo = XGetVisualInfo(monitor->display, VisualIDMask, &template, &items);

  if (vinfo == NULL) {
    return glgfx_pixel_format_unknown;
  }

  if (items != 1) {
    BUG("XGetVisualInfo returned more than one XVisualInfo??\n");
    XFree(vinfo);
    return glgfx_pixel_format_unknown;
  }

  printf("masks: %08lx %08lx %08lx\n", vinfo->red_mask, vinfo->green_mask, vinfo->blue_mask);
  printf("fbconfig index: %d (inv: %d)\n", *fbconfig_index, *y_inverted);

  struct glgfx_tagitem const px_tags[] = {
    { glgfx_pixel_attr_rgb,       true              },
    { glgfx_pixel_attr_redmask,   vinfo->red_mask   },
    { glgfx_pixel_attr_greenmask, vinfo->green_mask },
    { glgfx_pixel_attr_bluemask,  vinfo->blue_mask  },
    { glgfx_tag_end,              0                 }
  };

  return glgfx_pixel_getformat_a(px_tags);
}


static size_t glgfx_texture_size(int width, int height, enum glgfx_pixel_format format) {
  if (format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max ||
    width < 0 || height < 0) {
    BUG("width: %d; height: %d; format: %d\n", width, height, (int) format);
    abort();
  }

  return width * height * formats[format].size;
}


struct glgfx_bitmap* glgfx_bitmap_create_a(struct glgfx_tagitem const* tags) {
  struct glgfx_bitmap* bitmap;

  bitmap = calloc(1, sizeof *bitmap);

  if (bitmap == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  bitmap->format = glgfx_pixel_format_unknown;

  if (!glgfx_bitmap_setattrs_a(bitmap, tags)) {
    glgfx_bitmap_destroy(bitmap);
    return NULL;
  }

  return bitmap;
}



static void free_cliprect(gpointer data, gpointer userdata) {
  (void) userdata;

  free(data);
}


void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap) {
  struct glgfx_context* context = glgfx_context_getcurrent();

  if (bitmap == NULL) {
    return;
  }

  glgfx_context_forget(bitmap);

  pthread_mutex_lock(&glgfx_mutex);

  glgfx_bitmap_unlock_a(bitmap, NULL);
  if (context->monitor->have_GL_ARB_pixel_buffer_object) {
    glDeleteBuffers(1, &bitmap->pbo);
  }
  else {
    free(bitmap->buffer);
  }
  glDeleteTextures(1, &bitmap->texture);
  
  if (bitmap->glx_pixmap != None) {
    glXReleaseTexImageEXT(context->monitor->display,
			  bitmap->glx_pixmap,
			  GLX_FRONT_LEFT_EXT);
    glXDestroyPixmap(context->monitor->display, 
		     bitmap->glx_pixmap);
  }

  g_list_foreach(bitmap->cliprects, free_cliprect, NULL);
  free(bitmap);

  pthread_mutex_unlock(&glgfx_mutex);
}


void* glgfx_bitmap_lock_a(struct glgfx_bitmap* bitmap, bool read, bool write,
			  struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  struct glgfx_tagitem const* tag;
  void* res = NULL;

  if (bitmap == NULL || (!read && !write)) {
    return NULL;
  }
  
  pthread_mutex_lock(&glgfx_mutex);

  bitmap->locked_x = 0;
  bitmap->locked_y = 0;
  bitmap->locked_width = bitmap->width;
  bitmap->locked_height = bitmap->height;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_copy_tag) tag->tag) {
      case glgfx_bitmap_copy_x:
	bitmap->locked_x = tag->data;
	break;

      case glgfx_bitmap_copy_y:
	bitmap->locked_y = tag->data;
	break;

      case glgfx_bitmap_copy_width:
	bitmap->locked_width = tag->data;
	break;

      case glgfx_bitmap_copy_height:
	bitmap->locked_height = tag->data;
	break;

      case glgfx_bitmap_copy_data:
      case glgfx_bitmap_copy_format:
      case glgfx_bitmap_copy_bytesperrow:
      case glgfx_bitmap_copy_unknown:
      case glgfx_bitmap_copy_max:
	/* Make compiler happy */
	break;
    }
  }

  if (bitmap->locked_x < 0 || bitmap->locked_width <= 0 ||
      bitmap->locked_x + bitmap->locked_width > bitmap->width ||
      bitmap->locked_y < 0 || bitmap->locked_height <= 0 ||
      bitmap->locked_y + bitmap->locked_height > bitmap->height) {
    pthread_mutex_unlock(&glgfx_mutex);
    return NULL;
  }

  if (read && write) {
    bitmap->locked_usage = GL_STREAM_COPY;
    bitmap->locked_access = GL_READ_WRITE;
  }
  else if (!read && write) {
    bitmap->locked_usage = GL_STREAM_DRAW;
    bitmap->locked_access = GL_WRITE_ONLY;
  }
  else {
    bitmap->locked_usage = GL_STATIC_READ;
    bitmap->locked_access = GL_READ_ONLY;
  }
  
  if (context->monitor->have_GL_ARB_pixel_buffer_object) {
    if (bitmap->pbo == 0) {
      glGenBuffers(1, &bitmap->pbo);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo_size, 
		      NULL, bitmap->locked_usage);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

      GLGFX_CHECKERROR();
    }
  }
  else {
    if (bitmap->buffer == NULL) {
      bitmap->buffer = malloc(bitmap->pbo_size);
    }
  }

  bitmap->locked = true;

  // I have no idea why, but on some GeForces, this needs to be done
  // even if we're only writing. Driver 1.0.8756 bug?
  glgfx_context_bindfbo(context, 1, &bitmap);

  if (read) {
    // Bind FBO and attach texture
// always done    glgfx_context_bindfbo(context, 1, &bitmap); 

    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

    if (context->monitor->have_GL_ARB_pixel_buffer_object) {
      glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, bitmap->pbo);
      glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->width);
      glReadPixels(bitmap->locked_x, bitmap->locked_y, 
		   bitmap->locked_width, bitmap->locked_height,
		   formats[bitmap->format].format, 
		   formats[bitmap->format].type, 
		   (void*) (bitmap->locked_x * formats[bitmap->format].size +
			    bitmap->locked_y * bitmap->pbo_bytes_per_row));
      glPixelStorei(GL_PACK_ROW_LENGTH, 0);
      glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

      GLGFX_CHECKERROR();
    }
    else {
      glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->width);
      glReadPixels(bitmap->locked_x, bitmap->locked_y, 
		   bitmap->locked_width, bitmap->locked_height,
		   formats[bitmap->format].format, 
		   formats[bitmap->format].type, 
		   (void*) (bitmap->buffer + 
			    bitmap->locked_x * formats[bitmap->format].size +
			    bitmap->locked_y * bitmap->pbo_bytes_per_row));
      glPixelStorei(GL_PACK_ROW_LENGTH, 0);
      
      GLGFX_CHECKERROR();
    }
  }
  
  if (context->monitor->have_GL_ARB_pixel_buffer_object) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
    bitmap->locked_memory = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB,
					   bitmap->locked_access);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    GLGFX_CHECKERROR();
  }
  else {
    bitmap->locked_memory = bitmap->buffer;
  }

  res = bitmap->locked_memory;

  pthread_mutex_unlock(&glgfx_mutex);

  return res;
}


bool glgfx_bitmap_unlock_a(struct glgfx_bitmap* bitmap, 
			   struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  struct glgfx_tagitem const* tag;
  bool rc = false;

  if (bitmap == NULL || !bitmap->locked) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  int x = bitmap->locked_x;
  int y = bitmap->locked_y;
  int width = bitmap->locked_width;
  int height = bitmap->locked_height;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_copy_tag) tag->tag) {
      case glgfx_bitmap_copy_x:
	x = tag->data;
	break;

      case glgfx_bitmap_copy_y:
	y = tag->data;
	break;

      case glgfx_bitmap_copy_width:
	width = tag->data;
	break;

      case glgfx_bitmap_copy_height:
	height = tag->data;
	break;

      case glgfx_bitmap_copy_data:
      case glgfx_bitmap_copy_format:
      case glgfx_bitmap_copy_bytesperrow:
      case glgfx_bitmap_copy_unknown:
      case glgfx_bitmap_copy_max:
	/* Make compiler happy */
	break;
    }
  }

  if (x < 0 || y < 0 || width < 0 || height < 0 ||
      (x + width) > bitmap->width || (y + height) > bitmap->height) {
    pthread_mutex_unlock(&glgfx_mutex);
    return false;
  }

  if (bitmap->locked_memory != NULL) {
    if (context->monitor->have_GL_ARB_pixel_buffer_object) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, bitmap->pbo);
      if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB)) {
	rc = true;
      }

      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE ||
	   bitmap->locked_access == GL_WRITE_ONLY)) {
#if 0
	glgfx_context_bindfbo(context, 1, &bitmap);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	glWindowPos2i(x, y);
	glDrawPixels(width, height, 
		     formats[bitmap->format].format,
		     formats[bitmap->format].type,
		     (void*) (x * formats[bitmap->format].size +
			      y * bitmap->pbo_bytes_per_row));
#else
	glgfx_context_bindtex(context, 0, bitmap, false);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	glTexSubImage2D(bitmap->texture_target, 0,
			x, y, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (x * formats[bitmap->format].size +
				 y * bitmap->pbo_bytes_per_row));
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	glgfx_context_unbindtex(context, 0);
#endif
      }

      glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      GLGFX_CHECKERROR();
    }
    else {
      rc = true;

      if (width != 0 && height != 0 &&
	  (bitmap->locked_access == GL_READ_WRITE ||
	   bitmap->locked_access == GL_WRITE_ONLY)) {
	glgfx_context_bindtex(context, 0, bitmap, false);
	
	glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->width);
	glTexSubImage2D(bitmap->texture_target, 0,
			x, y, width, height,
			formats[bitmap->format].format,
			formats[bitmap->format].type,
			(void*) (bitmap->buffer +
				 x * formats[bitmap->format].size +
				 y * bitmap->pbo_bytes_per_row));
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	glgfx_context_unbindtex(context, 0);

	GLGFX_CHECKERROR();
      }
    }

    bitmap->locked_memory = NULL;
  }

  bitmap->locked = false;
  bitmap->has_changed = true;
  
  pthread_mutex_unlock(&glgfx_mutex);

  return rc;
}


bool glgfx_bitmap_write_a(struct glgfx_bitmap* bitmap, 
			  struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  int x = 0, y = 0, width = 0, height = 0;
  void* data = NULL;
  enum glgfx_pixel_format format = glgfx_pixel_format_unknown;
  size_t bytes_per_row = 0;

  bool got_bytes_per_row = false;

  struct glgfx_tagitem const* tag;

  if (bitmap == NULL || tags == NULL) {
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_copy_tag) tag->tag) {
      case glgfx_bitmap_copy_x:
	x = tag->data;
	break;

      case glgfx_bitmap_copy_y:
	y = tag->data;
	break;

      case glgfx_bitmap_copy_width:
	width = tag->data;
	break;

      case glgfx_bitmap_copy_height:
	height = tag->data;
	break;

      case glgfx_bitmap_copy_data:
	data = (void*) tag->data;
	break;

      case glgfx_bitmap_copy_format:
	format = tag->data;
	break;

      case glgfx_bitmap_copy_bytesperrow:
	bytes_per_row = tag->data;
	got_bytes_per_row = true;
	break;

      case glgfx_bitmap_copy_unknown:
      case glgfx_bitmap_copy_max:
	/* Make compiler happy */
	break;
    }
  }

  if (format <= glgfx_pixel_format_unknown || format >= glgfx_pixel_format_max) {
    return false;
  }

  if (!got_bytes_per_row) {
    bytes_per_row = formats[format].size * width;
  }

  if (width <= 0 || height <= 0 || 
      (x + width) > bitmap->width || (y + height) > bitmap->height ||
      data == NULL || bitmap->locked || bytes_per_row == 0) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  glgfx_context_bindtex(context, 0, bitmap, false);
   
  glPixelStorei(GL_UNPACK_ROW_LENGTH, bytes_per_row / formats[format].size);
  glTexSubImage2D(bitmap->texture_target, 0,
		  x, y, width, height,
		  formats[format].format,
		  formats[format].type,
		  data);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  GLGFX_CHECKERROR();

  glgfx_context_unbindtex(context, 0);

  bitmap->has_changed = true;

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


bool glgfx_bitmap_setattrs_a(struct glgfx_bitmap* bitmap,
			     struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();
  struct glgfx_tagitem const* tag;
  struct glgfx_bitmap* friend = NULL;
  bool rc = true;
  
  bool recreate = false;

  if (bitmap == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  if (bitmap->locked) {
    // Uh, uh.
    errno = EBUSY;
    pthread_mutex_unlock(&glgfx_mutex);
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_attr) tag->tag) {
      
      case glgfx_bitmap_attr_width:
	bitmap->width = tag->data;
	recreate = true;
	break;

      case glgfx_bitmap_attr_height:
	bitmap->height = tag->data;
	recreate = true;
	break;

      case glgfx_bitmap_attr_bits:
	bitmap->bits = tag->data;
	recreate = true;
	break;

      case glgfx_bitmap_attr_friend:
	friend = (struct glgfx_bitmap*) tag->data;
	recreate = true;
	break;

      case glgfx_bitmap_attr_format:
	bitmap->format = tag->data;
	recreate = true;
	break;

      case glgfx_bitmap_attr_pixmap:
	if (bitmap->glx_pixmap != None) {
	  glXReleaseTexImageEXT(context->monitor->display,
				bitmap->glx_pixmap,
				GLX_FRONT_LEFT_EXT);
	  glXDestroyPixmap(context->monitor->display, 
			   bitmap->glx_pixmap);
	  // TODO: DestroyGLXPixmap?
	  bitmap->glx_pixmap = None;
	}

	bitmap->pixmap = tag->data;
	recreate = true;
	break;

      case glgfx_bitmap_attr_visualid:
	// Don't recalculate format unless we have to
	if (tag->data != (intptr_t) bitmap->visualid || 
	    bitmap->format == glgfx_pixel_format_unknown) {
	  bitmap->visualid = tag->data;
	  bitmap->format = format_from_visualid(context->monitor, bitmap->visualid,
						&bitmap->fbconfig_index,
						&bitmap->y_inverted);
	  recreate = true;
	}
	break;

      case glgfx_bitmap_attr_bytesperrow:
      case glgfx_bitmap_attr_locked:
      case glgfx_bitmap_attr_mapaddr:
      case glgfx_bitmap_attr_unknown:
      case glgfx_bitmap_attr_max:
	/* Make compiler happy */
	break;
    }
  }

  bitmap->format            = select_format(bitmap->bits, friend, bitmap->format);
  bitmap->pbo_size          = glgfx_texture_size(bitmap->width, bitmap->height, bitmap->format);
  bitmap->pbo_bytes_per_row = glgfx_texture_size(bitmap->width, 1, bitmap->format);

  if (bitmap->width <= 0 || bitmap->height <= 0 || bitmap->format == glgfx_pixel_format_unknown) {
    errno = EINVAL;
    rc = false;
  }

  if (bitmap->pixmap != None && 
      (!context->monitor->have_GLX_EXT_texture_from_pixmap || bitmap->fbconfig_index == -1)) {
    errno = ENOTSUP;
    rc = false;
  }
  
  if (rc && recreate) {
    if (context->monitor->have_GL_ARB_pixel_buffer_object) {
      if (bitmap->pbo != 0) {
	glDeleteBuffers(1, &bitmap->pbo);
	bitmap->pbo = 0;
      }
    }
    else {
      if (bitmap->buffer != NULL) {
	free(bitmap->buffer);
	bitmap->buffer = NULL;
      }
    }

    if (bitmap->texture != 0) {
      glDeleteTextures(1, &bitmap->texture);
    }
    
    glGenTextures(1, &bitmap->texture);
    GLGFX_CHECKERROR();

    if (context->monitor->have_GL_ARB_texture_rectangle) {
      bitmap->texture_target = GL_TEXTURE_RECTANGLE_ARB;
    }
    else {
      bitmap->texture_target = GL_TEXTURE_2D;
    }

    glgfx_context_bindtex(context, 0, bitmap, false);

    if (bitmap->pixmap != None) {
      int pixmap_attribs[] = { GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_RECTANGLE_EXT, None };
      bitmap->glx_pixmap = glXCreatePixmap(context->monitor->display,
					   context->monitor->fbconfigs[bitmap->fbconfig_index],
					   bitmap->pixmap, pixmap_attribs);
      
      glXBindTexImageEXT(context->monitor->display,
			 bitmap->glx_pixmap,
			 GLX_FRONT_LEFT_EXT,
			 NULL);
    }
    else {
      glTexImage2D(bitmap->texture_target, 0,
		   formats[bitmap->format].internal_format,
		   bitmap->width, bitmap->height, 0,
		   formats[bitmap->format].format,
		   formats[bitmap->format].type,
		   NULL);
    }

    if (glGetError() == GL_INVALID_ENUM) {
      // Format not supported by hardware

      glgfx_context_unbindtex(context, 0);
      pthread_mutex_unlock(&glgfx_mutex);
      errno = ENOTSUP;
      rc = false;
    }
    else {
      GLGFX_CHECKERROR();

      glTexParameteri(bitmap->texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(bitmap->texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      GLGFX_CHECKERROR();
      glgfx_context_unbindtex(context, 0);
    }
  }

  bitmap->has_changed = true;

  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


bool glgfx_bitmap_getattr(struct glgfx_bitmap* bm,
			  enum glgfx_bitmap_attr attr,
			  intptr_t* storage) {
  bool rc = true;

  if (bm == NULL || storage == NULL ||
      attr <= glgfx_bitmap_attr_unknown || attr >= glgfx_bitmap_attr_max) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  switch (attr) {
    case glgfx_bitmap_attr_width:
      *storage = bm->width;
      break;

    case glgfx_bitmap_attr_height:
      *storage = bm->height;
      break;

    case glgfx_bitmap_attr_bits:
      *storage = (formats[bm->format].redbits + 
		  formats[bm->format].greenbits + 
		  formats[bm->format].bluebits + 
		  formats[bm->format].alphabits);
      break;

    case glgfx_bitmap_attr_friend:
      rc = false;
      break;

    case glgfx_bitmap_attr_format: 
      *storage = bm->format;
      break;

    case glgfx_bitmap_attr_pixmap:
      *storage = bm->pixmap;
      break;

    case glgfx_bitmap_attr_visualid:
      *storage = bm->visualid;
      break;

    case glgfx_bitmap_attr_bytesperrow:
      *storage = formats[bm->format].size * bm->width;
      break;
      
    case glgfx_bitmap_attr_locked:
      *storage = bm->locked;
      break;
      
    case glgfx_bitmap_attr_mapaddr:
      *storage = (intptr_t) bm->locked_memory;
      break;

    case glgfx_bitmap_attr_unknown:
    case glgfx_bitmap_attr_max:
      rc = false;
      break;
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


bool glgfx_bitmap_haschanged(struct glgfx_bitmap* bitmap) {
  bool has_changed = bitmap->has_changed;

  bitmap->has_changed = false;

  return has_changed;
}


/* Blitter minterms -> glLogicOp() argument

# Bit number in minterm
A Source rectangle (always on for non-twisted blits)
B Source pixel
C Destination pixel

# ABC		Minterm 	glLogicOp
- ---		-------		---------
0 000		00		GL_CLEAR	
1 001		10		GL_NOR		
2 010		20		GL_AND_INVERTED	
3 011		30		GL_COPY_INVERTED
4 100		40		GL_AND_REVERSE	
5 101		50		GL_INVERT	
6 110		60		GL_XOR		
7 111		70		GL_NAND		
		80		GL_AND		
		90		GL_EQUIV	
		a0		GL_NOOP		
		b0		GL_OR_INVERTED	
		c0		GL_COPY		
		d0		GL_OR_REVERSE	
		e0		GL_OR		
		f0		GL_SET		
*/

bool glgfx_bitmap_blit_a(struct glgfx_bitmap* bitmap, 
			 struct glgfx_tagitem const* tags) {
  struct glgfx_context* context = glgfx_context_getcurrent();

  struct glgfx_tagitem const* tag;
  int src_x = -1, src_y = -1, src_width = -1, src_height = -1;
  int mod_x =  0, mod_y =  0, mod_width = -1, mod_height = -1;
  int dst_x = -1, dst_y = -1, dst_width = -1, dst_height = -1;
  struct glgfx_bitmap* src_bitmap = bitmap;
  struct glgfx_bitmap* mod_bitmap = NULL;
  struct glgfx_bitmap* dst_bitmap = bitmap;
  bool src_interpolated = false;
  bool mod_interpolated = false;
  void* mask = NULL;
  int mask_x = 0, mask_y = 0;
  int mask_bpp = 0;
  int minterm = 0xc0;
  GLfloat mod_r = 1.0, mod_g = 1.0, mod_b = 1.0, mod_a = 1.0;
  enum glgfx_blend_equation blend_eq = glgfx_blend_equation_disabled;
  enum glgfx_blend_equation blend_eq_alpha = glgfx_blend_equation_unknown;
  enum glgfx_blend_func blend_func_src = glgfx_blend_func_one;
  enum glgfx_blend_func blend_func_src_alpha = glgfx_blend_func_unknown;
  enum glgfx_blend_func blend_func_dst = glgfx_blend_func_srcalpha_inv;
  enum glgfx_blend_func blend_func_dst_alpha = glgfx_blend_func_unknown;

  bool got_src_width = false;
  bool got_src_height = false;
  bool got_mod_width = false;
  bool got_mod_height = false;
  bool got_mod_rgba = false;

  static GLenum const ops[16] = {
    GL_CLEAR, GL_NOR, GL_AND_INVERTED, GL_COPY_INVERTED, 
    GL_AND_REVERSE, GL_INVERT, GL_XOR, GL_NAND,
    GL_AND, GL_EQUIV, GL_NOOP, GL_OR_INVERTED,
    GL_COPY, GL_OR_REVERSE, GL_OR, GL_SET
  };


  if (bitmap == NULL || tags == NULL) {
    errno = EINVAL;
    return false;
  }

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_bitmap_blit_tag) tag->tag) {
      case glgfx_bitmap_blit_x:
	dst_x = tag->data;
	break;

      case glgfx_bitmap_blit_y:
	dst_y = tag->data;
	break;

      case glgfx_bitmap_blit_width:
	dst_width = tag->data;
	break;

      case glgfx_bitmap_blit_height:
	dst_height = tag->data;
	break;

      case glgfx_bitmap_blit_src_x:
	src_x = tag->data;
	break;

      case glgfx_bitmap_blit_src_y:
	src_y = tag->data;
	break;

      case glgfx_bitmap_blit_src_width:
	src_width = tag->data;
	got_src_width = true;
	break;

      case glgfx_bitmap_blit_src_height:
	src_height = tag->data;
	got_src_height = true;
	break;

      case glgfx_bitmap_blit_src_bitmap:
	src_bitmap = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_bitmap_blit_src_interpolated:
	src_interpolated = (bool) tag->data;
	break;

      case glgfx_bitmap_blit_mask_x:
	mask_x = tag->data;
	break;

      case glgfx_bitmap_blit_mask_y:
	mask_y = tag->data;
	break;

      case glgfx_bitmap_blit_mask_ptr:
	mask = (void*) tag->data;
	break;

      case glgfx_bitmap_blit_mask_bytesperrow:
	mask_bpp = tag->data;
	break;
	
      case glgfx_bitmap_blit_mod_r:
	mod_r = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_g:
	mod_g = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_b:
	mod_b = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_a:
	mod_a = tag->data / 65536.0;
	got_mod_rgba = true;
	break;

      case glgfx_bitmap_blit_mod_bitmap:
	mod_bitmap = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_bitmap_blit_mod_interpolated:
	mod_interpolated = (bool) tag->data;
	break;

      case glgfx_bitmap_blit_mod_x:
	mod_x = tag->data;
	break;

      case glgfx_bitmap_blit_mod_y:
	mod_y = tag->data;
	break;

      case glgfx_bitmap_blit_mod_width:
	mod_width = tag->data;
	got_mod_width = true;
	break;

      case glgfx_bitmap_blit_mod_height:
	mod_height = tag->data;
	got_mod_height = true;
	break;

      case glgfx_bitmap_blit_minterm:
	minterm = tag->data;
	break;

      case glgfx_bitmap_blit_blend_equation:
	blend_eq = tag->data;
	break;
	
      case glgfx_bitmap_blit_blend_equation_alpha:
	blend_eq_alpha = tag->data;
	break;

      case glgfx_bitmap_blit_blend_srcfunc:
	blend_func_src = tag->data;
	break;

      case glgfx_bitmap_blit_blend_srcfunc_alpha:
	blend_func_src_alpha = tag->data;
	break;

      case glgfx_bitmap_blit_blend_dstfunc:
	blend_func_dst = tag->data;
	break;

      case glgfx_bitmap_blit_blend_dstfunc_alpha:
	blend_func_dst_alpha = tag->data;
	break;

      case glgfx_bitmap_blit_unknown:
      case glgfx_bitmap_blit_max:
	break;
    }
  }

  if (!got_src_width) {
    // Default is 1:1 copy
    src_width = dst_width;
  }

  if (!got_src_height) {
    // Default is 1:1 copy
    src_height = dst_height;
  }

  if (mod_bitmap != NULL && !got_mod_width) {
    // Default is to use full bitmap
    mod_width = mod_bitmap->width;
  }

  if (mod_bitmap != NULL && !got_mod_height) {
    // Default is to use full bitmap
    mod_height = mod_bitmap->height;
  }

  if (mask_bpp == 0) {
    mask_bpp = src_width / 8;
  }

  if (blend_eq_alpha == glgfx_blend_equation_unknown||
      blend_eq_alpha == glgfx_blend_equation_disabled) {
    blend_eq_alpha = blend_eq;
  }

  if (blend_func_src_alpha == glgfx_blend_func_unknown) {
    blend_func_src_alpha = blend_func_src;
  }

  if (blend_func_dst_alpha == glgfx_blend_func_unknown) {
    blend_func_dst_alpha = blend_func_dst;
  }


  if (src_bitmap == NULL && mod_bitmap != NULL) {
    // NULL src bitmap components are always 1.0 and the coordinates are
    // irrelevant. If mod_bitmap is present, set it as the new source.
    src_bitmap = mod_bitmap;
    src_interpolated = mod_interpolated;
    src_x = mod_x;
    src_y = mod_y;
    src_width = mod_width;
    src_height = mod_height;

    mod_bitmap = NULL;
  }

  if (/* src_x < 0 || src_y < 0 || */ src_width <= 0 || src_height <= 0 ||
      /* dst_x < 0 || dst_y < 0 || */ dst_width <= 0 || dst_height <= 0 ||
      /* (src_bitmap != NULL && src_x + src_width > src_bitmap->width) || */
      /* dst_x + dst_width > dst_bitmap->width ||  */
      /* (src_bitmap != NULL && src_y + src_height > src_bitmap->height) || */
      /* dst_y + dst_height > dst_bitmap->height || */
      (minterm & ~0xff) != 0) {
    errno = EINVAL;
    return false;
  }

  if (mod_bitmap != NULL) {
    if (/* mod_x < 0 || mod_y < 0 || */ mod_width <= 0 || mod_height <= 0 /* || */
	/* mod_x + mod_width > mod_bitmap->width ||  */
	/* mod_y + mod_height > mod_bitmap->height */) {
      errno = EINVAL;
      return false;
    }
  }

  if (mask != NULL) {
    if (mask_bpp < 0 || 
	mask_x < 0 || mask_y < 0 ||
	mask_x >= mask_bpp * 8) {
      errno = EINVAL;
      return false;
    }
  }

  if (blend_eq <= glgfx_blend_equation_unknown ||
      blend_eq_alpha <= glgfx_blend_equation_unknown ||
      blend_func_src <= glgfx_blend_func_unknown ||
      blend_func_src_alpha <= glgfx_blend_func_unknown ||
      blend_func_dst <= glgfx_blend_func_unknown ||
      blend_func_dst_alpha <= glgfx_blend_func_unknown ||
      blend_eq >= glgfx_blend_equation_max ||
      blend_eq_alpha >= glgfx_blend_equation_max ||
      blend_func_src >= glgfx_blend_func_max ||
      blend_func_src_alpha >= glgfx_blend_func_max ||
      blend_func_dst >= glgfx_blend_func_max ||
      blend_func_dst_alpha >= glgfx_blend_func_max) {
    errno = EINVAL;
    return false;
  }


  bool rc = true;

  if (src_bitmap == dst_bitmap && 
      mod_bitmap == NULL &&
      dst_width == src_width && 
      dst_height == dst_height &&
      mask == NULL &&
      !got_mod_rgba &&
      (!context->monitor->miss_pixel_ops || (minterm & 0xf0) == 0xc0)) {
    if ((minterm & 0xf0) != 0xc0) {
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(ops[minterm >> 4]);
    }

    // Bind FBO and attach texture
    glgfx_context_bindfbo(context, 1, &dst_bitmap);

    // Blit using glCopyPixels(), no texturing
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glWindowPos2i(dst_x, dst_y);

    GList* cr = g_list_first(bitmap->cliprects);

    if (cr != NULL) {
      glEnable(GL_SCISSOR_TEST);
    }

    glgfx_context_unbindprogram(context);
    glgfx_context_checkstate(context);

    do {
      if (cr != NULL) {
	struct glgfx_cliprect* cliprect = (struct glgfx_cliprect*) cr->data;

	if (dst_x + dst_width < cliprect->x ||
	    cliprect->x + cliprect->width < dst_x ||
	    dst_y + dst_height < cliprect->y ||
	    cliprect->y + cliprect->height < dst_y) {
	  continue;
	}

	glScissor(cliprect->x, cliprect->y, cliprect->width, cliprect->height);
      }

      glCopyPixels(src_x, src_y, src_width, src_height, GL_COLOR);

    } while (cr != NULL && (cr = g_list_next(cr)) != NULL);

    if (g_list_first(bitmap->cliprects) != NULL) {
      glDisable(GL_SCISSOR_TEST);
    }

    if ((minterm & 0xf0) != 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }    
  }
  else {
    if (src_bitmap == dst_bitmap || mask != NULL) {
      struct glgfx_bitmap* tmp_bitmap;
      enum glgfx_pixel_format fmt;
      
      if (mask == NULL) {
	fmt = src_bitmap->format;
      }
      else {
	fmt = glgfx_pixel_format_a8r8g8b8;
      }

      tmp_bitmap = glgfx_context_gettempbitmap(context,
					       src_width, src_height,
					       fmt);
      if (tmp_bitmap == NULL) {
	errno = ENOMEM;
	rc = false;
      }
      else {
	// Bind FBO and attach destination (aka the temp src) bitmap
	glgfx_context_bindfbo(context, 1, &tmp_bitmap);

	if (mask != NULL && // FIXME: glBitmap() is totally broken on ATI
	    !context->monitor->is_ati) { 
	  // Clear area
	  glColor4f(0, 0, 0, 0);
	  glgfx_context_bindprogram(context, &color_blitter);
	  glgfx_context_checkstate(context);

	  glBegin(GL_QUADS); {
	    glVertex2i(0,
		       src_height);
	    glVertex2i(src_width,
		       src_height);
	    glVertex2i(src_width,
		       0);
	    glVertex2i(0,  
		       0);
	  }
	  glEnd();

	  // Draw mask
	  glColor4f(1, 1, 1, 1);

	  if (mask_x != 0) {
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, mask_bpp * 8);
	  }

	  glWindowPos2i(0, 0);
	  glBitmap(mask_bpp * 8 - mask_x, src_height,
		   mask_x, mask_y,
		   0, 0,
		   mask);
	  
	  if (mask_x != 0) {
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	  }

	  // Enable blending
	  glEnable(GL_BLEND);
	  glBlendFunc(GL_DST_COLOR, GL_ZERO);
	  glBlendEquation(GL_FUNC_ADD);
	}

	GLenum unit = GL_TEXTURE0;

	if (src_bitmap != NULL) {
	  // Bind src bitmap as texture
	  unit = glgfx_context_bindtex(context, 0, src_bitmap, false);

	  if (mask == NULL) {
	    // Make a plain copy, with no color-space transformations
	    glgfx_context_bindprogram(context, &raw_texture_blitter);
	  }
	  else {
	    // Copy bitmap to RGBA temp bitmap (FP bitmaps will be clamped
	    // here, but we NEED a working alpha channel)
	    glgfx_context_bindprogram(context, &plain_texture_blitter);
	  }

	  glgfx_context_checkstate(context);

	  glBegin(GL_QUADS); {
	    glMultiTexCoord2i(unit, 
			      src_x,
			      src_y);
	    glVertex2i(0,
		       src_height);

	    glMultiTexCoord2i(unit,
			      src_x + src_width,
			      src_y);
	    glVertex2i(src_width,
		       src_height);

	    glMultiTexCoord2i(unit,
			      src_x + src_width, 
			      src_y + src_height);
	    glVertex2i(src_width,
		       0);

	    glMultiTexCoord2i(unit,
			      src_x,
			      src_y + src_height);
	    glVertex2i(0,  
		       0);
	  }
	  glEnd();
	}

	if (mask != NULL) {
	  glDisable(GL_BLEND);
	}

	src_x = 0;
	src_y = 0;

	// Install tmp bitmap as new source
	src_bitmap = tmp_bitmap;
      }
    }

    if ((minterm & 0xf0) != 0xc0) {
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(ops[minterm >> 4]);
    }

    // Bind FBO and attach texture
    glgfx_context_bindfbo(context, 1, &dst_bitmap);

    GLenum src_unit = GL_TEXTURE0;
    GLenum mod_unit = GL_TEXTURE1;

    if (src_bitmap != NULL) {
      // Bind temp src bitmap as texture
      src_unit = glgfx_context_bindtex(context, 0, src_bitmap, src_interpolated);

      if (mod_bitmap != NULL) {
	// Bind mod bitmap as texture
	mod_unit = glgfx_context_bindtex(context, 1, mod_bitmap, mod_interpolated);

	glColor4f(mod_r, mod_g, mod_b, mod_a);
	glgfx_context_bindprogram(context, &modulated_texture_blitter);
      }
      else if (got_mod_rgba) {
	glColor4f(mod_r, mod_g, mod_b, mod_a);
	glgfx_context_bindprogram(context, &color_texture_blitter);
      }
      else {
	glgfx_context_bindprogram(context, &plain_texture_blitter);
      }
    }
    else {
      // NULL source texture -> mod_bitmap is also NULL -> plain color blit
      glColor4f(mod_r, mod_g, mod_b, mod_a);
      glgfx_context_bindprogram(context, &color_blitter);
    }

    if (blend_eq != glgfx_blend_equation_disabled) {
      glEnable(GL_BLEND);
      glBlendEquationSeparate(glgfx_blend_equations[blend_eq], 
			      glgfx_blend_equations[blend_eq_alpha]);
      glBlendFuncSeparate(glgfx_blend_funcs[blend_func_src], 
			  glgfx_blend_funcs[blend_func_dst],
			  glgfx_blend_funcs[blend_func_src_alpha], 
			  glgfx_blend_funcs[blend_func_dst_alpha]);
    }


    GList* cr = g_list_first(bitmap->cliprects);

    if (cr != NULL) {
      glEnable(GL_SCISSOR_TEST);
    }

    glgfx_context_checkstate(context);

    do {
      if (cr != NULL) {
	struct glgfx_cliprect* cliprect = (struct glgfx_cliprect*) cr->data;

	if (dst_x + dst_width < cliprect->x ||
	    cliprect->x + cliprect->width < dst_x ||
	    dst_y + dst_height < cliprect->y ||
	    cliprect->y + cliprect->height < dst_y) {
	  continue;
	}

	glScissor(cliprect->x, cliprect->y, cliprect->width, cliprect->height);
      }

      glBegin(GL_QUADS); {
	glMultiTexCoord2i(src_unit,
			  src_x,
			  src_y);
	glMultiTexCoord2i(mod_unit,
			  mod_x,
			  mod_y);
	glVertex2i(dst_x,
		   dst_bitmap->height - dst_y);

	glMultiTexCoord2i(src_unit,
			  src_x + src_width, 
			  src_y);
	glMultiTexCoord2i(mod_unit,
			  mod_x + mod_width, 
			  mod_y);
	glVertex2i(dst_x + dst_width, 
		   dst_bitmap->height - dst_y);

	glMultiTexCoord2i(src_unit,
			  src_x + src_width, 
			  src_y + src_height);
	glMultiTexCoord2i(mod_unit,
			  mod_x + mod_width, 
			  mod_y + mod_height);
	glVertex2i(dst_x + dst_width, 
		   dst_bitmap->height - (dst_y + dst_height));

	glMultiTexCoord2i(src_unit,
			  src_x,
			  src_y + src_height);
	glMultiTexCoord2i(mod_unit,
			  mod_x,
			  mod_y + mod_height);
	glVertex2i(dst_x,
		   dst_bitmap->height - (dst_y + dst_height));
      }
      glEnd();

    } while (cr != NULL && (cr = g_list_next(cr)) != NULL);

    if (blend_eq != glgfx_blend_equation_disabled) {
      glDisable(GL_BLEND);
    }

    if (g_list_first(bitmap->cliprects) != NULL) {
      glDisable(GL_SCISSOR_TEST);
    }

    if ((minterm & 0xf0) != 0xc0) {
      glDisable(GL_COLOR_LOGIC_OP);
    }

    GLGFX_CHECKERROR();
  }

  glgfx_context_unbindtex(context, 0);
  glgfx_context_unbindtex(context, 1);
  
  if (rc) {
    dst_bitmap->has_changed = true;
  }

  return rc;
}


struct glgfx_cliprect* glgfx_bitmap_addcliprect(struct glgfx_bitmap* bitmap,
						int x, int y, int width, int height) {
  struct glgfx_cliprect* cliprect;
  
  if (bitmap == NULL || width < 0 || height < 0) {
    errno = EINVAL;
    return NULL;
  }

  cliprect = malloc(sizeof (*cliprect));

  if (cliprect == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  cliprect->x      = x;
  cliprect->y      = y;
  cliprect->width  = width;
  cliprect->height = height;

  pthread_mutex_lock(&glgfx_mutex);

  bitmap->cliprects = g_list_append(bitmap->cliprects, cliprect);

  pthread_mutex_unlock(&glgfx_mutex);
  return cliprect;
}


bool glgfx_bitmap_remcliprect(struct glgfx_bitmap* bitmap,
			      struct glgfx_cliprect* cliprect) {
  if (bitmap == NULL || cliprect == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  bitmap->cliprects = g_list_remove(bitmap->cliprects, cliprect);
  free(cliprect);

  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


int glgfx_bitmap_numcliprects(struct glgfx_bitmap* bitmap) {
  int res;
  
  if (bitmap == NULL) {
    errno = EINVAL;
    return 0;
  }

  pthread_mutex_lock(&glgfx_mutex);

  res = g_list_length(bitmap->cliprects);

  pthread_mutex_unlock(&glgfx_mutex);
  return res;
}
