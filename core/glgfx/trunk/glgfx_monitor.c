
#include "glgfx-config.h"

#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glgfx_monitor.h"
#include "glgfx_intern.h"

#include <X11/extensions/xf86dga.h>

#define glXChooseVisualAttrs(d, s, tag1 ...) \
  ({ int _attrs[] = { tag1 }; glXChooseVisual((d), (s), _attrs); })

struct glgfx_monitor* glgfx_monitor_create(char const*  display_name,
					   struct glgfx_monitor const* friend) {
  struct glgfx_monitor* monitor;
  sigjmp_buf jmpbuf;
  struct sigaction sa, old_sa;

  errno = EAGAIN;

  void handler() {
    siglongjmp(jmpbuf, 1);
  }

  monitor = calloc(1, sizeof (*monitor));

  if (monitor == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  monitor->name = strdup(display_name);
  monitor->xa_win_state = None;
  
  sa.sa_handler = (__sighandler_t) handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGALRM, &sa, &old_sa);

  // Don't spend more than one second trying to find a display
  // Yeah ... it's ugly and might cause resource leaks.
  alarm(1);

  if (sigsetjmp(jmpbuf, 1) == 0) {
    monitor->display = XOpenDisplay(display_name);
  }
  else {
    monitor->display = 0;
  }

  alarm(0);
  sigaction(SIGALRM, &old_sa, NULL);

  if (monitor->display == NULL) {
    glgfx_monitor_destroy(monitor);
    errno = ENXIO;
    return NULL;
  }

  D(BUG("Opened display %s. Extensions: %s\n", display_name,
	glXQueryExtensionsString(monitor->display, DefaultScreen(monitor->display))));

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
      
  XVisualInfo* vinfo;
  XSetWindowAttributes swa;

  // Get a truecolor visual, at least a 555 mode.
  vinfo = glXChooseVisualAttrs(monitor->display,
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

  if (vinfo == NULL) {
    BUG("Unable to get a GL visual for display %s!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

  struct glgfx_tagitem tags[] = {
    { glgfx_pixel_attr_rgb,       true              },
    { glgfx_pixel_attr_redmask,   vinfo->red_mask   },
    { glgfx_pixel_attr_greenmask, vinfo->green_mask },
    { glgfx_pixel_attr_bluemask,  vinfo->blue_mask  },
    { glgfx_tag_done,             0                 }
  };

  monitor->format = glgfx_pixel_getformat(tags);
  
  swa.colormap = XCreateColormap(monitor->display,
				 RootWindow(monitor->display,
					    vinfo->screen),
				 vinfo->visual,
				 AllocNone);
  swa.background_pixel = BlackPixel(monitor->display,
				    vinfo->screen);
  swa.border_pixel = BlackPixel(monitor->display,
				vinfo->screen);

  monitor->window = XCreateWindow(monitor->display,
				  RootWindow(monitor->display,
					     vinfo->screen),
				  0, 0,
				  monitor->mode.hdisplay,
				  monitor->mode.vdisplay,
				  0, 
				  vinfo->depth,
				  CopyFromParent,
				  vinfo->visual,
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

  monitor->context = glXCreateContext(monitor->display,
				      vinfo,
				      friend != NULL ? friend->context : NULL,
				      True);

  if (monitor->context == NULL) {
    BUG("Unable to create a GL context for window on display %s!\n",
	display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }
      
  if (!glXMakeCurrent(monitor->display, monitor->window, monitor->context)) {
    BUG("Unable to make GL context display %s current!\n", display_name);
    glgfx_monitor_destroy(monitor);
    return NULL;
  }

//  D(BUG("Supported extensions: %s\n", glGetString(GL_EXTENSIONS)));

  // Setup a standard integer 2D coordinate system
  glViewport(0, 0, monitor->mode.hdisplay, monitor->mode.vdisplay);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, monitor->mode.hdisplay, monitor->mode.vdisplay, 0, -1, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.375, 0.375, 0.0);
  
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
  
  errno = 0;
  return monitor;
}


void glgfx_monitor_destroy(struct glgfx_monitor* monitor) {
  if (monitor == NULL) {
    return;
  }

  D(BUG("Destroying monitor %p (%s)\n", monitor, monitor->name));

  glgfx_monitor_fullscreen(monitor, false);

  monitor->dotclock = 0;

  if (monitor->mode.privsize != 0) {
    XFree(monitor->mode.private);
    D(BUG("Freed private modeline data.\n"));
  }

  if (monitor->context != NULL) {
    glXMakeCurrent(monitor->display, None, NULL);
    glXDestroyContext(monitor->display, monitor->context);
    monitor->context = NULL;
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

  free(monitor);
  D(BUG("Freed monitor\n"));
}


void glgfx_monitor_fullscreen(struct glgfx_monitor* monitor, bool fullscreen) {
  long propvalue[3];

  if (monitor == NULL || monitor->display == NULL) {
    return;
  }

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
}


bool glgfx_monitor_getattr(struct glgfx_monitor* monitor,
			   enum glgfx_monitor_attr attr,
			   uint32_t* storage) {
  if (monitor == NULL || storage == NULL ||
      attr <= glgfx_monitor_attr_unknown || attr >= glgfx_monitor_attr_max) {
    return false;
  }

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

  return true;
}


bool glgfx_monitor_select(struct glgfx_monitor* monitor) {
  static struct glgfx_monitor* current_monitor = NULL;

  if (monitor == NULL) {
    return false;
  }

  if (current_monitor != monitor) {
    if (glXMakeCurrent(monitor->display,
		       monitor->window,
		       monitor->context)) {
      current_monitor = monitor;
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return true;
  }
}


bool glgfx_monitor_waitblit(struct glgfx_monitor* monitor) {
  if (!glgfx_monitor_select(monitor)) {
    return false;
  }

  glFinish();
  return true;
}

bool glgfx_monitor_waittof(struct glgfx_monitor* monitor) {
  glXSwapBuffers(monitor->display, monitor->window);
  return true;
}


