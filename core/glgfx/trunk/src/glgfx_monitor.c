
#include "glgfx-config.h"

#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glgfx_glext.h"
#include "glgfx_monitor.h"
#include "glgfx_intern.h"

#include <GL/glxext.h>
#include <X11/extensions/xf86dga.h>


#define glXChooseVisualAttrs(d, s, tag1 ...) \
  ({ int _attrs[] = { tag1 }; glXChooseVisual((d), (s), _attrs); })


static bool check_extensions(struct glgfx_monitor* monitor) {
  void populate(char const* e) {
    if (e == NULL) {
      return;
    }

    char* ext_str  = strdup(e);
    char* ext_tail = ext_str;
    char* ext;

    while((ext = strsep(&ext_tail, " ")) != NULL) {
      if (ext[0] != 0) {
	if (!g_hash_table_lookup(monitor->extensions, ext)) {
	  g_hash_table_insert(monitor->extensions, strdup(ext), (gpointer) 1);
	}
      }
    }
      
    free(ext_str);
  }

  populate((char const*) glGetString(GL_EXTENSIONS));
  populate(glXGetClientString(monitor->display, GLX_EXTENSIONS));
  populate(glXQueryExtensionsString(monitor->display, 
				    DefaultScreen(monitor->display)));
  populate(glXQueryServerString(monitor->display,
				DefaultScreen(monitor->display), 
				GLX_EXTENSIONS)); 

  

  // Check for render-to-texture support
  if (g_hash_table_lookup(monitor->extensions, "GL_EXT_framebuffer_object") != NULL) {
    monitor->have_GL_EXT_framebuffer_object = true;
  }
  else {
    BUG("Required extension GL_EXT_framebuffer_object missing from display %s!\n",
	monitor->name);
  }

  // Check for texture rectangle extension
  if (g_hash_table_lookup(monitor->extensions, "GL_ARB_texture_rectangle") != NULL ||
      g_hash_table_lookup(monitor->extensions, "GL_EXT_texture_rectangle") != NULL) {
    monitor->have_GL_texture_rectangle = true;
  }
  else {
    BUG("Required extension GL_{ARB,EXT}_texture_rectangle missing from display %s!\n",
	monitor->name);
  }

  // Check for VBLANK sync
  if (g_hash_table_lookup(monitor->extensions, "GLX_SGI_video_sync") != NULL) {
    monitor->have_GLX_SGI_video_sync = true;
  }

  // Check for GL_ARB_vertex_buffer_object and GL_ARB_pixel_buffer_object
  if (g_hash_table_lookup(monitor->extensions, "GL_ARB_vertex_buffer_object") != NULL) {
    monitor->have_GL_ARB_vertex_buffer_object = true;

    if (g_hash_table_lookup(monitor->extensions, "GL_EXT_pixel_buffer_object") != NULL) {
      monitor->have_GL_ARB_pixel_buffer_object = true;
    }
  }

  if (!monitor->have_GL_ARB_pixel_buffer_object) {
    BUG("Warning: GL_EXT_pixel_buffer_object not supported; will emulate.\n");
  }

  // Init all extensions we might use
  glgfx_glext_init();

  // Return true if all required extensions are present
  return (monitor->have_GL_EXT_framebuffer_object && 
	  monitor->have_GL_texture_rectangle);
}


struct glgfx_monitor* glgfx_monitor_create(char const* display_name,
					   struct glgfx_monitor const* friend) {
  struct glgfx_monitor* monitor;

  // Default error code
  errno = ENOTSUP;

  monitor = calloc(1, sizeof (*monitor));

  if (monitor == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  monitor->extensions = g_hash_table_new(g_str_hash, g_str_equal);

  if (monitor->extensions == NULL) {
    glgfx_monitor_destroy(monitor);
    errno = ENOMEM;
    return NULL;
  }

  D(BUG("Opening display %s\n", display_name));
  monitor->name = strdup(display_name);
  monitor->xa_win_state = None;

  monitor->friend = friend;
  
  monitor->display = XOpenDisplay(display_name);

  if (monitor->display == NULL) {
    glgfx_monitor_destroy(monitor);
    errno = ENXIO;
    return NULL;
  }

  int dummy;
  if (!XF86DGAQueryExtension(monitor->display, &dummy, &dummy)) {
    BUG("The XF86DGA extension is missing from display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  if (!XF86VidModeQueryExtension(monitor->display, &dummy, &dummy)) {
    BUG("The XF86VidMode extension is missing from display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }
  
  if (!XF86VidModeGetModeLine(monitor->display,
			      DefaultScreen(monitor->display),
			      &monitor->dotclock,
			      &monitor->mode)) {
    BUG("Unable to get current mode line for display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  D(BUG("Pixel clock:  %.1f MHz\n", monitor->dotclock / 1000.0));
  D(BUG("Width:        %d pixels\n", monitor->mode.hdisplay));
  D(BUG("Height:       %d pixels\n", monitor->mode.vdisplay));
  D(BUG("Refresh rate: %.1f Hz\n",
	monitor->dotclock * 1000.0 / (monitor->mode.htotal * monitor->mode.vtotal)));
      
  if (!XF86VidModeGetMonitor(monitor->display,
			     DefaultScreen(monitor->display),
			     &monitor->monitor_info)) {
    BUG("Unable to get current monitor info for display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  D(BUG("Vendor:  %s\n", monitor->monitor_info.vendor));
  D(BUG("Model:   %s\n", monitor->monitor_info.model));

  int i;
  for (i = 0; i < monitor->monitor_info.nhsync; ++i) {
    D(BUG("HSync: %.1f-%.1f kHz\n",
	  monitor->monitor_info.hsync->lo,
	  monitor->monitor_info.hsync->hi));
  }

  for (i = 0; i < monitor->monitor_info.nvsync; ++i) {
    D(BUG("BSync: %.1f-%.1f Hz\n",
	  monitor->monitor_info.vsync->lo,
	  monitor->monitor_info.hsync->hi));
  }

  if (!glXQueryExtension(monitor->display, NULL, NULL)) {
    BUG("The GLX extension is missing from display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }
      
  XSetWindowAttributes swa;

  // Get a truecolor visual, at least a 555 mode.
  monitor->vinfo = glXChooseVisualAttrs(monitor->display,
					DefaultScreen(monitor->display),
					GLX_USE_GL,
					GLX_DOUBLEBUFFER,
					GLX_RGBA,
					GLX_RED_SIZE,     1,
					GLX_GREEN_SIZE,   1,
					GLX_BLUE_SIZE,    1,
					GLX_DEPTH_SIZE,   12,
					GLX_STENCIL_SIZE, 1,
					None);

  if (monitor->vinfo == NULL) {
    BUG("Unable to get a GL visual for display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  struct glgfx_tagitem const tags[] = {
    { glgfx_pixel_attr_rgb,       true                       },
    { glgfx_pixel_attr_redmask,   monitor->vinfo->red_mask   },
    { glgfx_pixel_attr_greenmask, monitor->vinfo->green_mask },
    { glgfx_pixel_attr_bluemask,  monitor->vinfo->blue_mask  },
    { glgfx_tag_end,              0                          }
  };

  monitor->format = glgfx_pixel_getformat_a(tags);
  
  swa.colormap = XCreateColormap(monitor->display,
				 RootWindow(monitor->display,
					    monitor->vinfo->screen),
				 monitor->vinfo->visual,
				 AllocNone);
  swa.background_pixel = BlackPixel(monitor->display,
				    monitor->vinfo->screen);
  swa.border_pixel = BlackPixel(monitor->display,
				monitor->vinfo->screen);

  monitor->window = XCreateWindow(monitor->display,
				  RootWindow(monitor->display,
					     monitor->vinfo->screen),
				  0, 0,
				  monitor->mode.hdisplay,
				  monitor->mode.vdisplay,
				  0, 
				  monitor->vinfo->depth,
				  CopyFromParent,
				  monitor->vinfo->visual,
//				   0,
				  (CWBackPixel |
				   CWBorderPixel |
				   CWColormap),
				  &swa);
      
  if (monitor->window == 0) {
    BUG("Unable to create a window on display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  monitor->main_context = glgfx_monitor_createcontext(monitor);

  if (monitor->main_context == NULL) {
    BUG("Unable to create a GL context for window on display %s!\n",
	display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }
  
  glgfx_monitor_fullscreen(monitor, true);

  XMapRaised(monitor->display, monitor->window);
  XSelectInput(monitor->display, monitor->window,
	       (ButtonPressMask |
		ButtonReleaseMask |
		ButtonMotionMask |
		PointerMotionMask |
		KeyPressMask |
		KeyReleaseMask));
  XFlush(monitor->display);
  
  // Everything is now setup. Check if all the required extensions are present
  if (!check_extensions(monitor)) {
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  errno = 0;
  return monitor;
}


void glgfx_monitor_destroy(struct glgfx_monitor* monitor) {
  if (monitor == NULL) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);

  D(BUG("Destroying monitor %p (%s)\n", monitor, monitor->name));

  glgfx_monitor_fullscreen(monitor, false);

  monitor->dotclock = 0;

  if (monitor->mode.privsize != 0) {
    XFree(monitor->mode.private);
    D(BUG("Freed private modeline data.\n"));
  }

  if (monitor->main_context != NULL) {
    glgfx_monitor_destroycontext(monitor, monitor->main_context);
    monitor->main_context = NULL;
    D(BUG("Destroyed context for window.\n"));
  }
  
  if (monitor->window != 0) {
    XDestroyWindow(monitor->display, monitor->window);
    monitor->window = 0;
    D(BUG("Closed window.\n"));
  }

  if (monitor->display != NULL) {
    XCloseDisplay(monitor->display);
    monitor->display = NULL;
    D(BUG("Closed display.\n"));
  }

  if (monitor->extensions != NULL) {
    void cleanup(gpointer key, gpointer value, gpointer userdata) {
      (void) value;
      (void) userdata;
      free(key);
    }

    g_hash_table_foreach(monitor->extensions, cleanup, NULL);
    g_hash_table_destroy(monitor->extensions);
  }

  free(monitor);
  D(BUG("Freed monitor\n"));

  pthread_mutex_unlock(&glgfx_mutex);
}


void glgfx_monitor_fullscreen(struct glgfx_monitor* monitor, bool fullscreen) {
  long propvalue[3];

  if (monitor == NULL || monitor->display == NULL || monitor->window == 0) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);

  if (monitor->xa_win_state == None) {
    monitor->xa_win_state = XInternAtom(monitor->display, "_NET_WM_STATE", False);
  }

  if (fullscreen) {
    propvalue[0] = XInternAtom(monitor->display, "_NET_WM_STATE_FULLSCREEN",
			       False);
    propvalue[1] = XInternAtom(monitor->display, "_NET_WM_STATE_ABOVE",
			       False);
    propvalue[2] = 0;

    XChangeProperty(monitor->display, monitor->window,
		    monitor->xa_win_state, XA_ATOM,
		    32, PropModeReplace,
		    (unsigned char *) propvalue, 2);
  }
  else {
    XDeleteProperty(monitor->display, monitor->window, monitor->xa_win_state);
  }

  pthread_mutex_unlock(&glgfx_mutex);
}

struct glgfx_context* glgfx_monitor_createcontext(struct glgfx_monitor* monitor) {
  GLXContext context;
  
  if (monitor == NULL) {
    return NULL;
  }

  pthread_mutex_lock(&glgfx_mutex);

  context = glXCreateContext(
    monitor->display,
    monitor->vinfo,
    (GLXContext) (monitor->main_context != NULL ? monitor->main_context :
		  monitor->friend != NULL ? monitor->friend->main_context : NULL),
    True);

  if (!glXMakeCurrent(monitor->display, monitor->window, context)) {
    BUG("Unable to make GL context current!\n");
    glXDestroyContext(monitor->display, context);
    return NULL;
  }

  // Setup a standard integer 2D coordinate system
  glViewport(0, 0, monitor->mode.hdisplay, monitor->mode.vdisplay);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, monitor->mode.hdisplay, monitor->mode.vdisplay, 0, -1, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
//  glTranslatef(0.5, 0.5, 0.0);
//  glTranslatef(0.375, 0.375, 0.0);

  pthread_mutex_unlock(&glgfx_mutex);
  return (struct glgfx_context*) context;
}

void glgfx_monitor_destroycontext(struct glgfx_monitor* monitor,
				  struct glgfx_context* context) {
  pthread_mutex_lock(&glgfx_mutex);
  glXMakeCurrent(monitor->display, None, NULL);
  glXDestroyContext(monitor->display, (GLXContext) context);
  pthread_mutex_unlock(&glgfx_mutex);
}

static __thread struct glgfx_context* current_context  = NULL;

bool glgfx_monitor_selectcontext(struct glgfx_monitor* monitor,
				 struct glgfx_context* context) {
  bool rc;
  
  if (monitor == NULL || context == NULL) {
    return false;
  }

  if (current_context != context) {
    pthread_mutex_lock(&glgfx_mutex);

    if (glXMakeCurrent(monitor->display,
		       monitor->window,
		       (GLXContext) context)) {
      current_context = context;
      rc = true;
    }
    else {
      rc = false;
    }

    pthread_mutex_unlock(&glgfx_mutex);
  }
  else {
    rc = true;
  }

  return rc;
}

bool glgfx_monitor_getattr(struct glgfx_monitor* monitor,
			   enum glgfx_monitor_attr attr,
			   intptr_t* storage) {
  if (monitor == NULL || storage == NULL ||
      attr <= glgfx_monitor_attr_unknown || attr >= glgfx_monitor_attr_max) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  switch (attr) {
    case glgfx_monitor_attr_width:
      *storage = monitor->mode.hdisplay;
      break;

    case glgfx_monitor_attr_height:
      *storage = monitor->mode.vdisplay;
      break;

    case glgfx_monitor_attr_format: 
      *storage = monitor->format;
      break;

    case glgfx_monitor_attr_vsync:
      *storage = monitor->dotclock * 1000.0 / (monitor->mode.htotal * monitor->mode.vtotal);
      break;

    case glgfx_monitor_attr_hsync:
      *storage = monitor->dotclock * 1000.0 / monitor->mode.htotal;
      break;
      
    case glgfx_monitor_attr_dotclock:
      *storage = monitor->dotclock * 1000.0;
      break;
      
    default:
      abort();
  }

  pthread_mutex_unlock(&glgfx_mutex);
  
  return true;
}


bool glgfx_monitor_select(struct glgfx_monitor* monitor) {
  return glgfx_monitor_selectcontext(monitor, monitor->main_context);
}


bool glgfx_monitor_waitblit(struct glgfx_monitor* monitor) {
  if (!glgfx_monitor_select(monitor)) {
    return false;
  }

  glFinish();
  return true;
}

bool glgfx_monitor_swapbuffers(struct glgfx_monitor* monitor) {
  pthread_mutex_lock(&glgfx_mutex);
  glXSwapBuffers(monitor->display, monitor->window);
  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}

bool glgfx_monitor_waittof(struct glgfx_monitor* monitor) {
  if (monitor->have_GLX_SGI_video_sync) {
    GLuint frame_count;
  
    if (!glgfx_monitor_select(monitor)) {
      return false;
    }

    // Don't lock mutex here, since it will sleep!
    if (glXGetVideoSyncSGI(&frame_count) != 0) {
      return false;
    }

    if (glXWaitVideoSyncSGI(1, 0, &frame_count) != 0) {
      return false;
    }

    return true;
  }
  else {
    D(BUG("Don't know how to wait for vertical blank interrupt!\n"));
    return false;
  }
}
