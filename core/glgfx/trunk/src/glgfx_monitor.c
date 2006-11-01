
#include "glgfx-config.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "glgfx_context.h"
#include "glgfx_glext.h"
#include "glgfx_glxemul.h"
#include "glgfx_monitor.h"
#include "glgfx_intern.h"

#include <GL/glxext.h>

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
  monitor->is_ati = strstr((char*) glGetString(GL_VENDOR), "ATI ") != NULL;
  monitor->miss_pixel_ops = monitor->is_ati;

  // Ugly hack for Geforce FX 5xxx driver bugs. FIXME!
  monitor->is_geforce_fx = strstr((char*) glGetString(GL_RENDERER), "GeForce FX 5") != NULL;

  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &monitor->max_mrt);

  if (monitor->max_mrt > GLGFX_MAX_RENDER_TARGETS) {
    monitor->max_mrt = GLGFX_MAX_RENDER_TARGETS;
  }

  // Check for framebuffer_object support
  if (g_hash_table_lookup(monitor->gl_extensions, "GL_EXT_framebuffer_object") != NULL) {
    monitor->have_GL_EXT_framebuffer_object = true;
  }
  else {
    BUG("Required extension GL_EXT_framebuffer_object missing from display %s!\n",
	monitor->name);
  }

  // Check for texture_rectangle extension
  if (! monitor->is_ati &&
      g_hash_table_lookup(monitor->gl_extensions, "GL_ARB_texture_rectangle") != NULL) {
    monitor->have_GL_ARB_texture_rectangle = true;
  }
  else {
    BUG("Warning: GL_ARB_texture_rectangle not supported; "
	"will use NPOT 2D workaround.\n");
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

  // Check for blend_square
  if (g_hash_table_lookup(monitor->gl_extensions, "GL_NV_blend_square") != NULL) {
    monitor->have_GL_NV_blend_square = true;
  }

  if (!monitor->have_GL_NV_blend_square) {
    BUG("Warning: GL_NV_blend_square not supported; some blend modes will not work.\n");
  }

  // Check for texture_from_pixmap
  if (g_hash_table_lookup(monitor->gl_extensions, "GLX_EXT_texture_from_pixmap") != NULL) {
    monitor->have_GLX_EXT_texture_from_pixmap = true;
  }
  else {
    BUG("Warning: GLX_EXT_texture_from_pixmap not supported; reduced bitmap functionality.\n");
  }

  // Return true if all required extensions are present
  return (monitor->have_GL_EXT_framebuffer_object);
}


static void go_fullscreen(struct glgfx_monitor* monitor, bool fullscreen) {
  long propvalue[3];

  if (monitor == NULL || monitor->display == NULL || monitor->window == 0) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);

  Window window = monitor->xparent != None ? monitor->xparent : monitor->window;

  if (monitor->xa_win_state == None) {
    monitor->xa_win_state = XInternAtom(monitor->display, "_NET_WM_STATE", False);
  }

  if (fullscreen) {
    propvalue[0] = XInternAtom(monitor->display, "_NET_WM_STATE_FULLSCREEN",
			       False);
    propvalue[1] = XInternAtom(monitor->display, "_NET_WM_STATE_ABOVE",
			       False);
    propvalue[2] = 0;

    XChangeProperty(monitor->display, window,
		    monitor->xa_win_state, XA_ATOM,
		    32, PropModeReplace,
		    (unsigned char *) propvalue, 2);
  }
  else {
    XDeleteProperty(monitor->display, monitor->window, monitor->xa_win_state);
  }

  pthread_mutex_unlock(&glgfx_mutex);
}




struct glgfx_monitor* glgfx_monitor_create_a(Display* display,
					     struct glgfx_tagitem const* tags) {
  struct glgfx_monitor* monitor;
  struct glgfx_tagitem const* tag;
  Window dummy_win;
  int dummy;

  monitor = calloc(1, sizeof (*monitor));

  if (monitor == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  if (display == NULL) {
    display = XOpenDisplay(NULL);

    if (display == NULL) {
      BUG("Unable to open display '%s'.\n", XDisplayName(NULL));
      free(monitor);
      errno = EINVAL;
      return NULL;
    }

    monitor->display_opened = true;
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

      case glgfx_monitor_attr_xparent:
	monitor->xparent = tag->data;
	break;

      case glgfx_monitor_attr_view:
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

  // Default error code
  errno = ENOTSUP;

  monitor->display = display;
  monitor->name = strdup(DisplayString(monitor->display));
  monitor->xa_win_state = None;

  monitor->gl_extensions = g_hash_table_new(g_str_hash, g_str_equal);

  if (monitor->gl_extensions == NULL) {
    free(monitor);
    errno = ENOMEM;
    return NULL;
  }

  if (!XF86VidModeQueryExtension(monitor->display, &dummy, &dummy)) {
    BUG("The XF86VidMode extension is missing from display %s!\n", monitor->name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }
  
  if (!XF86VidModeGetModeLine(monitor->display,
			      DefaultScreen(monitor->display),
			      &monitor->dotclock,
			      &monitor->mode)) {
    BUG("Unable to get current mode line for display %s!\n", monitor->name);
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
    BUG("Unable to get current monitor info for display %s!\n", monitor->name);
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
    BUG("The GLX extension is missing from display %s!\n", monitor->name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  // Get a list of all available FBConfigs (used for GLX_EXT_texture_from_pixmap)

  monitor->fbconfigs = glXGetFBConfigs(monitor->display, DefaultScreen(monitor->display), 
				       &monitor->num_fbconfigs);


  // Select one suitable FBConfig for our framebuffer

  static int const fb_attribs[] = { 
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PBUFFER_BIT,
    GLX_DOUBLEBUFFER,  True,
    GLX_RED_SIZE,      4,
    GLX_GREEN_SIZE,    4,
    GLX_BLUE_SIZE,     4,
    GLX_DEPTH_SIZE,    16,
    GLX_STENCIL_SIZE,  8,
    GLX_CONFIG_CAVEAT, GLX_NONE,
    None
  };

  int num_configs;
  GLXFBConfig* configs = glXChooseFBConfig(monitor->display, 
					   DefaultScreen(monitor->display), 
					   fb_attribs, 
					   &num_configs);

  D(BUG("Found %d suitable FBConfigs\n", num_configs));

  if (num_configs == 0) {
    BUG("Unable to get a sane FBConfig for display %s!\n", monitor->name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  monitor->fbconfig = configs[0];
  XFree(configs);

  monitor->vinfo = glXGetVisualFromFBConfig(monitor->display, monitor->fbconfig);

  if (monitor->vinfo == NULL) {
    BUG("Unable to get a GL visual for display %s!\n", monitor->name);
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
    BUG("Visual for display %s is not supported. Sorry.\n", monitor->name);
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
    BUG("Unable to create a window on display %s!\n", monitor->name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  monitor->glx_window = glXCreateWindow(monitor->display, 
					monitor->fbconfig,
					monitor->window,
					NULL);
  
  if (monitor->glx_window == 0) {
    BUG("Unable to create a GLX window on display %s!\n", monitor->name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  monitor->main_context = glgfx_monitor_createcontext(monitor);

  if (monitor->main_context == NULL) {
    BUG("Unable to create a GL context for window on display %s!\n",
	monitor->name);
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

  if (monitor->xparent != None) {
    XReparentWindow(monitor->display, monitor->window, monitor->xparent, 0, 0);
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

  XQueryPointer(monitor->display, monitor->window, &dummy_win, &dummy_win,
		&monitor->mouse_x, &monitor->mouse_y, &dummy, &dummy, (unsigned int*) &dummy);

  glgfx_context_select(monitor->main_context);

  errno = 0;
  return monitor;
}


static void cleanup(gpointer key, gpointer value, gpointer userdata) {
  (void) value;
  (void) userdata;
  free(key);
}

void glgfx_monitor_destroy(struct glgfx_monitor* monitor) {
  if (monitor == NULL) {
    return;
  }

  pthread_mutex_lock(&glgfx_mutex);

  D(BUG("Destroying monitor %p (%s)\n", monitor, monitor->name));


  if (monitor->fbconfigs != NULL) {
    XFree(monitor->fbconfigs);
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

  if (monitor->gl_extensions != NULL) {
    g_hash_table_foreach(monitor->gl_extensions, cleanup, NULL);
    g_hash_table_destroy(monitor->gl_extensions);
  }

  if (monitor->display_opened) {
    XCloseDisplay(monitor->display);
  }

  free(monitor->name);
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

      case glgfx_monitor_attr_xparent:
	monitor->xparent = tag->data;
	XReparentWindow(monitor->display, monitor->window, monitor->xparent, 0, 0);
	break;

      case glgfx_monitor_attr_view:
	monitor->view = (struct glgfx_view*) tag->data;
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
    case glgfx_monitor_attr_fullscreen:
      *storage = monitor->fullscreen;
      break;

    case glgfx_monitor_attr_xparent:
      *storage = monitor->xparent;
      break;

    case glgfx_monitor_attr_view:
      *storage = (intptr_t) monitor->view;
      break;

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

extern struct glgfx_list contexts;

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
    monitor->display, monitor->fbconfig, GLX_RGBA_TYPE,
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

	if (!glgfx_shader_init(monitor)) {
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

	    // Remember context
	    glgfx_list_addtail(&contexts, &context->node);
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

static unsigned long depth_func(struct glgfx_hook* hook, 
				struct glgfx_rasinfo* rasinfo, 
				struct glgfx_viewport_rendermsg* msg) {
  struct glgfx_context* context = (struct glgfx_context*) hook->data;

  if (rasinfo->blend_eq == glgfx_blend_equation_disabled &&
      rasinfo->blend_eq_alpha == glgfx_blend_equation_disabled) {
    // No magic required (even alpha != 1.0 are rendered opaque)
    glgfx_context_bindprogram(context, &plain_texture_blitter);
  }
  else {
    // Render only fully opaque pixels
    glgfx_context_bindprogram(context, &depth_renderer);
  }
  
  glgfx_context_checkstate(context);
  glgfx_callhook(msg->geometry_hook, rasinfo, msg);

  return 0;
}

static unsigned long stencil_func(struct glgfx_hook* hook, 
				  struct glgfx_rasinfo* rasinfo, 
				  struct glgfx_viewport_rendermsg* msg) {
  struct glgfx_context* context = (struct glgfx_context*) hook->data;

  glgfx_context_unbindprogram(context);
  glgfx_context_checkstate(context);
  glgfx_callhook(msg->geometry_hook, rasinfo, msg);

  return 0;
}


static unsigned long blur_func(struct glgfx_hook* hook, 
			       struct glgfx_rasinfo* rasinfo, 
			       struct glgfx_viewport_rendermsg* msg) {
  struct glgfx_context* context = (struct glgfx_context*) hook->data;

  if (rasinfo->blend_eq != glgfx_blend_equation_disabled) {
    // TODO: Don't change state if it's the same as the current state
    glEnable(GL_BLEND);
    glBlendEquationSeparate(glgfx_blend_equations[rasinfo->blend_eq],
			    glgfx_blend_equations[rasinfo->blend_eq_alpha]);
    glBlendFuncSeparate(glgfx_blend_funcs[rasinfo->blend_func_src],
			glgfx_blend_funcs[rasinfo->blend_func_dst],
			glgfx_blend_funcs[rasinfo->blend_func_src_alpha],
			glgfx_blend_funcs[rasinfo->blend_func_dst_alpha]);
  }
  else {
    glDisable(GL_BLEND);
  }

  glgfx_context_bindprogram(context, &blur_renderer);
  msg->z -= 0.01;
  glgfx_context_checkstate(context);
  glgfx_callhook(msg->geometry_hook, rasinfo, msg);

  return 0;
}

static unsigned long render_func(struct glgfx_hook* hook, 
				 struct glgfx_rasinfo* rasinfo, 
				 struct glgfx_viewport_rendermsg* msg) {
  struct glgfx_context* context = (struct glgfx_context*) hook->data;

  if (rasinfo->blend_eq != glgfx_blend_equation_disabled) {
    // TODO: Don't change state if it's the same as the current state
    glEnable(GL_BLEND);
    glBlendEquationSeparate(glgfx_blend_equations[rasinfo->blend_eq],
			    glgfx_blend_equations[rasinfo->blend_eq_alpha]);
    glBlendFuncSeparate(glgfx_blend_funcs[rasinfo->blend_func_src],
			glgfx_blend_funcs[rasinfo->blend_func_dst],
			glgfx_blend_funcs[rasinfo->blend_func_src_alpha],
			glgfx_blend_funcs[rasinfo->blend_func_dst_alpha]);
  }
  else {
    glDisable(GL_BLEND);
  }

  glgfx_context_bindprogram(context, &plain_texture_blitter);
  glgfx_context_checkstate(context);
  glgfx_callhook(msg->geometry_hook, rasinfo, msg);

  return 0;
}

bool glgfx_monitor_render(struct glgfx_monitor* monitor) {
  static const bool late_sprites = false;

  if (monitor == NULL || monitor->view == NULL) {
    errno = EINVAL;
    return false;
  }

  glgfx_context_unbindfbo(glgfx_context_getcurrent());

  pthread_mutex_lock(&glgfx_mutex);

  if (monitor->fps_counter == 0) {
    struct timeval start = monitor->fps_time;
    gettimeofday(&monitor->fps_time, NULL);

    if (start.tv_sec != 0 && start.tv_usec != 0) {
      double time = ((monitor->fps_time.tv_sec + monitor->fps_time.tv_usec * 1e-6) -
		     (start.tv_sec + start.tv_usec * 1e-6));
    
      monitor->fps_mean = 100 / time;
      printf("%f frames/second\n", monitor->fps_mean);
    }
  }
  
  monitor->fps_counter = (monitor->fps_counter + 1) % 100;
  

  bool has_changed = glgfx_view_haschanged(monitor->view);

  pthread_mutex_unlock(&glgfx_mutex);

  if (has_changed) {
    struct glgfx_context* context = glgfx_context_getcurrent();
    struct glgfx_hook depth_hook   = { (glgfx_hookfunc) depth_func,   context };
    struct glgfx_hook stencil_hook = { (glgfx_hookfunc) stencil_func, context };
    struct glgfx_hook blur_hook    = { (glgfx_hookfunc) blur_func,    context };
    struct glgfx_hook render_hook  = { (glgfx_hookfunc) render_func,  context };

    // Clear complete drawing area
    glDrawBuffer(GL_BACK);
    glClearColor(0, 0, 0, 0);
    glDepthRange(1.0, 0.0); // i don't get it but whatever ...
    glClearDepth(1.0);
    glStencilMask(~0);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pthread_mutex_lock(&glgfx_mutex);

    // Pre-passes do not touch the framebuffer, only depth and stencil
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_BLEND);

    // "Draw" opaque pixels front-to-back, only updating depth buffer.
    // The shader will 'discard' all fragments that are not fully
    // opaque. For pixels that are not discarded, the stencil buffer
    // will be set to one and the Z-buffer to the depth.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glgfx_view_render(monitor->view, &depth_hook, true);


    // "Render" visible transparent pixels back-to-front to the
    // stencil buffer.  The depth buffer is left as-is.
    //
    // Opaque pixels or transparent pixels below opaque pixels will
    // not be drawn, since they will be z-rejected
    // (glDepthFunc(GL_LESS)).
    //
    // Transparent pixels with no opaque pixels below will not be
    // drawn either, since stencil must be != 0
    // (glStencilFunc(GL_NOTEQUAL, 0, ...)).
    //
    // For each pixel that passes all tests, increment stencil by one.
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_NOTEQUAL, 0, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glgfx_view_render(monitor->view, &stencil_hook, false);

    // Enable framebuffer rendering
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Render blurred areas back-to-front. The stencil buffer holds
    // the number of blurred layes per pixel minus one. We render if
    // stencil > 1, and decrement the stencil buffer for every layer
    // drawn.  
    //
    // This also completes the Z-buffer, which will now be fully
    // filled, even in transparent areas.
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glStencilFunc(GL_LESS, 1, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    glgfx_view_render(monitor->view, &blur_hook, false);

    // Finally, disable the stencil test and draw the real image over
    // the background and blurred areas, back-to-front. (Only top-most
    // pixels will be drawn because of the Z-buffer.)
    glDisable(GL_STENCIL_TEST);
    glgfx_view_render(monitor->view, &render_hook, false);

    // Sprites are always transparent
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!late_sprites) {
      glgfx_view_rendersprites(monitor->view);
    }

    pthread_mutex_unlock(&glgfx_mutex);
  }

  glgfx_monitor_waittof(monitor);

  if (has_changed) {
    glgfx_monitor_swapbuffers(monitor);

    if (late_sprites) {
      glDrawBuffer(GL_FRONT);

      pthread_mutex_lock(&glgfx_mutex);
      glgfx_view_rendersprites(monitor->view);
      pthread_mutex_unlock(&glgfx_mutex);
    }

    glDisable(GL_BLEND);
  }

  return true;
}


bool glgfx_monitor_swapbuffers(struct glgfx_monitor* monitor) {
  glXSwapBuffers(monitor->display, monitor->glx_window);
  return true;
}
