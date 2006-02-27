
#include "glgfx-config.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_glxemul.h"
#include "glgfx_monitor.h"
#include "glgfx_intern.h"

#include <GL/glxext.h>
#include <X11/extensions/xf86dga.h>

static int __GL_SYNC_TO_VBLANK = -1;

static bool swapbuffer_syncs_to_vblank() {
  if (__GL_SYNC_TO_VBLANK == -1) {
    char const* var = getenv("__GL_SYNC_TO_VBLANK");
      
    __GL_SYNC_TO_VBLANK = var != NULL ? atoi(var) : 0;
  }

  return __GL_SYNC_TO_VBLANK == 1;
}


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
	if (!g_hash_table_lookup(monitor->gl_extensions, ext)) {
	  g_hash_table_insert(monitor->gl_extensions, strdup(ext), (gpointer) 1);
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


  // Ugly hack for ATI's driver. FIXME!
  monitor->miss_pixel_ops = strstr((char*) glGetString(GL_VENDOR), "ATI ") != NULL;

  // Check for framebuffer_object support
  if (g_hash_table_lookup(monitor->gl_extensions, "GL_EXT_framebuffer_object") != NULL) {
    monitor->have_GL_EXT_framebuffer_object = true;
  }
  else {
    BUG("Required extension GL_EXT_framebuffer_object missing from display %s!\n",
	monitor->name);
  }

  // Check for texture_rectangle extension
  if (g_hash_table_lookup(monitor->gl_extensions, "GL_ARB_texture_rectangle") != NULL ||
      g_hash_table_lookup(monitor->gl_extensions, "GL_EXT_texture_rectangle") != NULL) {
    monitor->have_GL_texture_rectangle = true;
  }
  else {
    BUG("Required extension GL_{ARB,EXT}_texture_rectangle missing from display %s!\n",
	monitor->name);
  }

  // Check for video_sync
  if (g_hash_table_lookup(monitor->gl_extensions, "GLX_SGI_video_sync") != NULL) {
    monitor->have_GLX_SGI_video_sync = true;
  }
  else {
    BUG("Warning: GLX_SGI_video_sync not supported; will emulate.\n");
  }

  // Check for pixel_buffer_object
  if (g_hash_table_lookup(monitor->gl_extensions, "GL_ARB_pixel_buffer_object") != NULL) {
    monitor->have_GL_ARB_pixel_buffer_object = true;
  }

  if (!monitor->have_GL_ARB_pixel_buffer_object) {
    BUG("Warning: GL_ARB_pixel_buffer_object not supported; will emulate.\n");
  }

  // Return true if all required extensions are present
  return (monitor->have_GL_EXT_framebuffer_object && 
	  monitor->have_GL_texture_rectangle);
}


static void go_fullscreen(struct glgfx_monitor* monitor, bool fullscreen) {
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




struct glgfx_monitor* glgfx_monitor_create_a(char const* display_name,
					     struct glgfx_tagitem const* tags) {
  struct glgfx_monitor* monitor;
  struct glgfx_tagitem const* tag;

  if (display_name == NULL) {
    errno = EINVAL;
    return NULL;
  }

  monitor = calloc(1, sizeof (*monitor));

  if (monitor == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  monitor->fullscreen = true;
  pthread_cond_init(&monitor->vsync_cond, NULL);

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_monitor_attr) tag->tag) {
      case glgfx_monitor_attr_friend:
	monitor->friend = (struct glgfx_monitor*) tag->data;
	break;

      case glgfx_monitor_attr_fullscreen:
	monitor->fullscreen = tag->data;
	break;

      case glgfx_monitor_attr_width:
      case glgfx_monitor_attr_height:
      case glgfx_monitor_attr_format:
      case glgfx_monitor_attr_vsync:
      case glgfx_monitor_attr_hsync:
      case glgfx_monitor_attr_dotclock:
      case glgfx_monitor_attr_unknown:
      case glgfx_monitor_attr_max:
	/* Make compiler happy */
	break;
    }
  }

  D(BUG("Opening display %s\n", display_name));

  // Default error code
  errno = ENOTSUP;

  monitor->name = strdup(display_name);
  monitor->xa_win_state = None;

  monitor->gl_extensions = g_hash_table_new(g_str_hash, g_str_equal);

  if (monitor->gl_extensions == NULL) {
    free(monitor);
    errno = ENOMEM;
    return NULL;
  }

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
    D(BUG("VSync: %.1f-%.1f Hz\n",
	  monitor->monitor_info.vsync->lo,
	  monitor->monitor_info.hsync->hi));
  }

  if (!glXQueryExtension(monitor->display, NULL, NULL)) {
    BUG("The GLX extension is missing from display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }


  static int const fb_attribs[] = { 
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PBUFFER_BIT,
    GLX_DOUBLEBUFFER,  True,
    GLX_RED_SIZE,      4,
    GLX_GREEN_SIZE,    4,
    GLX_BLUE_SIZE,     4,
    GLX_DEPTH_SIZE,    16,
    GLX_CONFIG_CAVEAT, GLX_NONE,
    None
  };

  monitor->fb_config = glXChooseFBConfig(monitor->display, 
					 DefaultScreen(monitor->display), 
					 fb_attribs, 
					 &monitor->fb_configs);

  D(BUG("Found %d suitable FBConfigs\n", monitor->fb_configs));

  if (monitor->fb_configs == 0) {
    BUG("Unable to get a sane FBConfig for display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }


  monitor->vinfo = glXGetVisualFromFBConfig(monitor->display, monitor->fb_config[0]);

  if (monitor->vinfo == NULL) {
    BUG("Unable to get a GL visual for display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  struct glgfx_tagitem const px_tags[] = {
    { glgfx_pixel_attr_rgb,       true                       },
    { glgfx_pixel_attr_redmask,   monitor->vinfo->red_mask   },
    { glgfx_pixel_attr_greenmask, monitor->vinfo->green_mask },
    { glgfx_pixel_attr_bluemask,  monitor->vinfo->blue_mask  },
    { glgfx_tag_end,              0                          }
  };

  monitor->format = glgfx_pixel_getformat_a(px_tags);

  if (monitor->format == glgfx_pixel_format_unknown) {
    BUG("Visual for display %s is not supported. Sorry.\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }
  
  XSetWindowAttributes swa;

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
				  (CWBackPixel |
				   CWBorderPixel |
				   CWColormap),
				  &swa);
      
  if (monitor->window == 0) {
    BUG("Unable to create a window on display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  monitor->glx_window = glXCreateWindow(monitor->display, 
					monitor->fb_config[0],
					monitor->window,
					NULL);
  
  if (monitor->glx_window == 0) {
    BUG("Unable to create a GLX window on display %s!\n", display_name);
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

  if (!monitor->have_GLX_SGI_video_sync &&
      !swapbuffer_syncs_to_vblank()) {
    // No video sync: Create a timer for vsync emulation

    struct sigevent ev;

    ev.sigev_notify = SIGEV_SIGNAL;
    ev.sigev_signo  = glgfx_signum;
    ev.sigev_value.sival_ptr = monitor;

    if (timer_create(CLOCK_REALTIME, &ev, &monitor->vsync_timer) == -1) {
      BUG("Unable to create vsync emulation timer\n");
      glgfx_monitor_destroy(monitor);
      return NULL;
    }
    else {
      monitor->vsync_timer_valid = true;
      
      long ns = (long) (1e9 / (monitor->dotclock * 1000.0 /
			       (monitor->mode.htotal * monitor->mode.vtotal)));
      struct timespec now;

      if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
	BUG("Unable to get current time\n");
	glgfx_monitor_destroy(monitor);
	return NULL;
      }

      monitor->vsync_itimerspec.it_value.tv_nsec = now.tv_nsec + ns;
      monitor->vsync_itimerspec.it_value.tv_sec  = now.tv_sec;

      while (monitor->vsync_itimerspec.it_value.tv_nsec > 1e9) {
	monitor->vsync_itimerspec.it_value.tv_nsec -= 1e9;
	++monitor->vsync_itimerspec.it_value.tv_sec;
      }

      if (timer_settime(monitor->vsync_timer, TIMER_ABSTIME, 
			&monitor->vsync_itimerspec, NULL) != 0) {
	BUG("Unable to start vsync emulation timer\n");
	glgfx_monitor_destroy(monitor);
	return NULL;
      }
    }
  }

  go_fullscreen(monitor, monitor->fullscreen);

  XMapRaised(monitor->display, monitor->window);
  XSelectInput(monitor->display, monitor->window,
	       (ButtonPressMask |
		ButtonReleaseMask |
		ButtonMotionMask |
		PointerMotionMask |
		KeyPressMask |
		KeyReleaseMask));
  XFlush(monitor->display);

  glgfx_context_select(monitor->main_context);

  errno = 0;
  return monitor;
}


void glgfx_monitor_destroy(struct glgfx_monitor* monitor) {
  if (monitor == NULL) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);

  D(BUG("Destroying monitor %p (%s)\n", monitor, monitor->name));

  if (monitor->fb_config != NULL) {
    XFree(monitor->fb_config);
  }

  if (monitor->vinfo != NULL) {
    XFree(monitor->vinfo);
  }

  if (monitor->vsync_timer_valid) {
    timer_delete(monitor->vsync_timer);
  }

  go_fullscreen(monitor, false);

  if (monitor->mode.privsize != 0) {
    XFree(monitor->mode.private);
    D(BUG("Destroyed private modeline data.\n"));
  }

  if (monitor->main_context != NULL) {
    glgfx_context_destroy(monitor->main_context);
    D(BUG("Destroyed context for window.\n"));
  }

  if (monitor->glx_window != 0) {
    glXDestroyWindow(monitor->display, monitor->glx_window);
    D(BUG("Destroyed GLX window.\n"));
  }
  
  if (monitor->window != 0) {
    XDestroyWindow(monitor->display, monitor->window);
    D(BUG("Closed window.\n"));
  }

  if (monitor->display != NULL) {
    XCloseDisplay(monitor->display);
    D(BUG("Closed display.\n"));
  }

  if (monitor->gl_extensions != NULL) {
    void cleanup(gpointer key, gpointer value, gpointer userdata) {
      (void) value;
      (void) userdata;
      free(key);
    }

    g_hash_table_foreach(monitor->gl_extensions, cleanup, NULL);
    g_hash_table_destroy(monitor->gl_extensions);
  }

  free(monitor);
  D(BUG("Destroyed monitor\n"));

  pthread_mutex_unlock(&glgfx_mutex);
}


bool glgfx_monitor_setattrs_a(struct glgfx_monitor* monitor,
			      struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;
  bool rc = true;

  if (monitor == NULL || tags == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_monitor_attr) tag->tag) {

      case glgfx_monitor_attr_fullscreen:
	monitor->fullscreen = tag->data;
	go_fullscreen(monitor, monitor->fullscreen);
	break;

      case glgfx_monitor_attr_friend:
      case glgfx_monitor_attr_width:
      case glgfx_monitor_attr_height:
      case glgfx_monitor_attr_format:
      case glgfx_monitor_attr_vsync:
      case glgfx_monitor_attr_hsync:
      case glgfx_monitor_attr_dotclock:
      case glgfx_monitor_attr_unknown:
      case glgfx_monitor_attr_max:
	/* Make compiler happy */
	break;
    }
  }

  pthread_mutex_unlock(&glgfx_mutex);
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

#include <time.h>

struct glgfx_context* glgfx_monitor_createcontext(struct glgfx_monitor* monitor) {
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
    monitor->friend != NULL ? monitor->friend->main_context->glx_context : NULL,
    True);

  if (context->glx_context == 0 || 
      !glXIsDirect(monitor->display, context->glx_context)) {
    BUG("Failed to create a direct GL context.\n");
    glgfx_context_destroy(context);
    context = NULL;
    errno = ENOTSUP;
  }
  else {
    if (!glXMakeContextCurrent(monitor->display, 
			       monitor->glx_window, monitor->glx_window,
			       context->glx_context)) {
      BUG("Unable to make GL context current!\n");
      glgfx_context_destroy(context);
      context = NULL;
      errno = ENOTSUP;
    }
    else {
      // Check that all the required extensions are present
      if (check_extensions(monitor)) {
	// Init all extensions we might use, if we haven't done so already
	glgfx_glext_init();

	if (!glgfx_shader_init()) {
	  BUG("Unable to initialize shaders!\n");
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
      else {
	// Required extensions missing
	glgfx_context_destroy(context);
	context = NULL;
	errno = ENOTSUP;
      }
      
    }
  }

  pthread_mutex_unlock(&glgfx_mutex);
  return context;
}


struct glgfx_context* glgfx_monitor_getcontext(struct glgfx_monitor* monitor) {
  if (monitor == NULL) {
    errno = EINVAL;
    return NULL;
  }

  return monitor->main_context;
}


bool glgfx_monitor_addview(struct glgfx_monitor* monitor,
			   struct glgfx_view* view) {
  if (monitor == NULL || view == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  monitor->views = g_list_append(monitor->views, view);
  pthread_mutex_unlock(&glgfx_mutex);

  return true;
}


bool glgfx_monitor_loadview(struct glgfx_monitor* monitor,
			    struct glgfx_view* view) {
  if (monitor == NULL || view == NULL || 
      g_list_find(monitor->views, view) == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  monitor->views = g_list_remove(monitor->views, view);
  monitor->views = g_list_prepend(monitor->views, view);
  pthread_mutex_unlock(&glgfx_mutex);

  return true;
}


bool glgfx_monitor_remview(struct glgfx_monitor* monitor,
			   struct glgfx_view* view) {

  if (monitor == NULL || view == NULL ||
      g_list_find(monitor->views, view) == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);
  monitor->views = g_list_remove(monitor->views, view);
  pthread_mutex_unlock(&glgfx_mutex);
  return true;
}


bool glgfx_monitor_waittof(struct glgfx_monitor* monitor) {
  if (swapbuffer_syncs_to_vblank()) {
    // Return immediately, since glgfx_monitor_swapbuffers() will
    // sleep for us.
    return false;
  }

  if (monitor->have_GLX_SGI_video_sync) {
    GLuint frame_count;

    if (glXGetVideoSyncSGI(&frame_count) != 0) {
      return false;
    }

    if (glXWaitVideoSyncSGI(1, 0, &frame_count) != 0) {
      return false;
    }

    return true;
  }
  else {
    bool rc = true;

    pthread_mutex_lock(&glgfx_mutex);
    if (pthread_cond_wait(&monitor->vsync_cond, &glgfx_mutex) != 0) {
      rc = false;
    }
    pthread_mutex_unlock(&glgfx_mutex);
    
    return true;
  }
}


bool glgfx_monitor_render(struct glgfx_monitor* monitor) {
  static const bool late_sprites = true;

  if (monitor == NULL || monitor->views == NULL) {
    errno = EINVAL;
    return false;
  }

  glgfx_context_unbindfbo(glgfx_context_getcurrent());

  pthread_mutex_lock(&glgfx_mutex);

  bool has_changed = glgfx_view_haschanged(monitor->views->data);

  pthread_mutex_unlock(&glgfx_mutex);

  if (has_changed) {
    glDrawBuffer(GL_BACK);
    glClearColor(0, 0, 0, 0);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    pthread_mutex_lock(&glgfx_mutex);

    glgfx_view_render(monitor->views->data);

    if (!late_sprites) {
      glgfx_view_rendersprites(monitor->views->data);
    }

    pthread_mutex_unlock(&glgfx_mutex);
  }

//  glgfx_monitor_waittof(monitor);

  if (has_changed) {
    if (late_sprites) {
      pthread_mutex_lock(&glgfx_mutex);
      glgfx_view_rendersprites(monitor->views->data);
      pthread_mutex_unlock(&glgfx_mutex);
    }

    glgfx_monitor_swapbuffers(monitor);
  }

  return true;
}


bool glgfx_monitor_swapbuffers(struct glgfx_monitor* monitor) {
  glXSwapBuffers(monitor->display, monitor->glx_window);
  return true;
}
