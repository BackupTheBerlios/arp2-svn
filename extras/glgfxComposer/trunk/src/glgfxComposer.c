
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <assert.h>
#include <glgfx.h>
#include <glgfx_bitmap.h>
#include <glgfx_monitor.h>
#include <glgfx_view.h>
#include <glgfx_viewport.h>
#include <glib.h>
#include <popt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int composite_request;


struct screen {
    Display* display;
    int screen;
    Window root;
    Window overlay;
    GHashTable* windows;
    struct glgfx_monitor* monitor;
    struct glgfx_view* view;
    struct glgfx_viewport* viewport;
};

struct window {
    struct screen* screen;
    XWindowAttributes attrs;
    Window window;
    Pixmap pixmap;
    struct glgfx_rasinfo* rasinfo;
    struct glgfx_bitmap* bitmap;
};

struct window* create_window(struct screen* screen, Window id) {
  

/*   printf("*** %d, %d %dx%dx%d b:%d %s\n",  */
/* 	 attrs.x, attrs.y,  */
/* 	 attrs.width, attrs.height, attrs.depth, */
/* 	 attrs.border_width,  */
/* 	 (attrs.map_state == IsViewable ? "IsViewable" :  */
/* 	  attrs.map_state == IsUnviewable ? "IsUnviewable" : */
/* 	  attrs.map_state == IsUnmapped ? "IsUnmapped" : "Unknown")); */

  struct window* window = calloc(sizeof *window, 1);

  if (window != NULL) {
    window->screen = screen;
    window->window = id;

    XGetWindowAttributes(screen->display, id, &window->attrs);

    if (window->attrs.map_state != IsViewable || window->attrs.depth == 0) {
      free(window);
      return NULL;
    }

    window->pixmap = XCompositeNameWindowPixmap(screen->display, id);
/*     window->pixmap = XCreatePixmap(screen->display, window->window, 1000, 1000, 24); */

    printf("creating bitmap from pixmap %lx (window %lx)\n", (long) id, (long) window->pixmap);
    window->bitmap = glgfx_bitmap_create(
      glgfx_bitmap_attr_width,    window->attrs.width,
      glgfx_bitmap_attr_height,   window->attrs.height,
/*       glgfx_bitmap_attr_bits,     attrs.depth, */
/*       glgfx_bitmap_attr_format,   glgfx_pixel_format_a8r8g8b8, */
      glgfx_bitmap_attr_visualid, XVisualIDFromVisual(window->attrs.visual),
      glgfx_bitmap_attr_pixmap,   window->pixmap,
      glgfx_tag_end);
/*     glgfx_bitmap_destroy(window->bitmap); */
/*     window->bitmap = NULL; */
    
    printf("created window for id %x\n", (unsigned int) id);
  }

  return window;
}

void destroy_window(struct window* window) {
  if (window == NULL) {
    return;
  }

  glgfx_bitmap_destroy(window->bitmap);

  if (window->pixmap != None) {
    XFreePixmap(window->screen->display, window->pixmap);
  }

  printf("destroyed window for id %x\n", (unsigned int) window->window);

  free(window);
}

static void do_destroy_window(gpointer key, gpointer value, gpointer user_data) {
  (void) key;
  (void) user_data;

  destroy_window((struct window*) value);
}

void add_window(struct screen* screen, struct window* window) {
  assert (window->rasinfo == NULL);

  window->rasinfo = glgfx_viewport_addbitmap(screen->viewport, window->bitmap,
					     glgfx_rasinfo_attr_x, window->attrs.x,
					     glgfx_rasinfo_attr_y, window->attrs.y,
					     glgfx_tag_end);

  if (window->rasinfo != NULL) {
    g_hash_table_insert(screen->windows, (void*) window->window, window);
  }
  else {
    fprintf(stderr, "Unable to add bitmap to viewport\n");
  }
}

void remove_window(struct screen* screen, struct window* window) {  
  assert (window->rasinfo != NULL);

  glgfx_viewport_rembitmap(screen->viewport, window->rasinfo);
  window->rasinfo = NULL;

  g_hash_table_remove(screen->windows, (void*) window->window);
}

struct window* find_window(struct screen* screen, Window id) {
  return g_hash_table_lookup(screen->windows, (void*) id);
}

void destroy_screen(struct screen* screen) {
  if (screen == NULL) {
    return;
  }

  if (screen->windows != NULL) {
    g_hash_table_foreach(screen->windows, do_destroy_window, NULL);
    g_hash_table_destroy(screen->windows);
  }

  glgfx_view_remviewport(screen->view, screen->viewport);
  glgfx_viewport_destroy(screen->viewport);

  glgfx_monitor_setattrs(screen->monitor, 
			 glgfx_monitor_attr_view, (intptr_t) NULL, 
			 glgfx_tag_end);
  glgfx_view_destroy(screen->view);
  glgfx_monitor_destroy(screen->monitor);

  if (screen->overlay != None) {
    XCompositeUnredirectSubwindows (screen->display, screen->root, CompositeRedirectManual);
    XCompositeReleaseOverlayWindow(screen->display, screen->overlay);
  }
  
  free(screen);
}

struct screen* create_screen(char const* display, Display* d, int s) {
  intptr_t width, height;

  struct screen* screen = calloc(sizeof *screen, 1);

  if (screen == NULL) { 
    fprintf(stderr, "Unable to allocate screen structure.\n");
    goto failed;
  }
   
  screen->display = d;
  screen->screen  = s;

  screen->windows = g_hash_table_new(g_direct_hash, g_direct_equal);

  if (screen->windows == NULL) {
    fprintf(stderr, "Unable to allocate hash table.\n");
    goto failed;
  }

  screen->root    = XRootWindow(d, s);

#if 1
  screen->overlay = XCompositeGetOverlayWindow(d, screen->root);
#else
  screen->overlay = XCreateWindow(d, screen->root, 100, 100, 100, 100,
				  CopyFromParent, CopyFromParent, CopyFromParent, CopyFromParent,
				  0, NULL);
  XMapWindow(d, screen->overlay);
  XFlush(d);
#endif

  if (screen->overlay == None) {
    fprintf(stderr, "Unable to create XComposite overlay window.\n");
    goto failed;
  }


  screen->monitor = glgfx_monitor_create(d,
					 glgfx_monitor_attr_xparent, screen->overlay,
					 glgfx_monitor_attr_fullscreen, false,
					 glgfx_tag_end);

  if (screen->monitor == NULL) {
    fprintf(stderr, "Unable to create glgfx monitor.\n");
    goto failed;
  }

  if (glgfx_getattrs(screen->monitor,
		     (glgfx_getattr_proto*) glgfx_monitor_getattr,
		     glgfx_monitor_attr_width,  (intptr_t) &width,
		     glgfx_monitor_attr_height, (intptr_t) &height,
		     glgfx_tag_end) != 2) {
    fprintf(stderr, "Unable to get display dimensions\n");
    goto failed;
  }

  screen->view = glgfx_view_create();

  if (screen->view == NULL) {
    fprintf(stderr, "Unable to create glgfx view.\n");
    goto failed;
  }

  if (!glgfx_monitor_setattrs(screen->monitor, 
			      glgfx_monitor_attr_view, (intptr_t) screen->view, 
			      glgfx_tag_end)) {
    fprintf(stderr, "Unable to load glgfx view.\n");
    goto failed;
  }
  
  screen->viewport = glgfx_viewport_create(glgfx_viewport_attr_width,  width,
					   glgfx_viewport_attr_height, height,
					   glgfx_tag_end);
  
  if (screen->viewport == NULL) {
    fprintf(stderr, "Unable to create glgfx viewport.\n");
    goto failed;
  }

  if (!glgfx_view_addviewport(screen->view, screen->viewport)) {
    fprintf(stderr, "Unable to add glgfx viewport to view.\n");
    goto failed;
  }
  
  XGrabServer(d);
  
  XCompositeRedirectSubwindows (d, screen->root, CompositeRedirectManual);
  XSelectInput (d, screen->root, SubstructureNotifyMask);

  Window root_return;
  Window parent_return;
  Window* children;
  unsigned int num_children;
  int i;
	      
  XQueryTree(d, screen->root, &root_return, &parent_return, &children, &num_children);
  
  for (i = 0; i < num_children; ++i) {
    struct window* window = create_window(screen, children[i]);

    if (window != NULL) { 
      add_window(screen, window);
    }
  }

  XFree(children);

  XUngrabServer(d);


  return screen;

  failed:
  destroy_screen(screen);
  return NULL;
}


int handle_screen(struct screen* screen) {
  bool quit = false;
  int counter = 75*5;

  while (!quit) {
    while (XPending(screen->display)) {
      XEvent xevent;

      XNextEvent(screen->display, &xevent);

      printf("got event type %d \n", xevent.type);

      switch (xevent.type) {
	case CreateNotify:
	  add_window(screen, create_window(screen, xevent.xcreatewindow.window));
	  break;

	case DestroyNotify: {
	  struct window* window = find_window(screen, xevent.xdestroywindow.window);

	  if (window != NULL) {
	    remove_window(window->screen, window);
	    destroy_window(window);
	  }
	  break;
	}
      }
    }

    glgfx_monitor_render(screen->monitor);

    if (--counter == 0) {
      quit = true;
    }
  }

  return 0;
}




static void usage(poptContext pc, int exitcode, char const *error) {
  poptPrintUsage(pc, stderr, 0);
  if (error) fprintf(stderr, "\nError: %s\n", error);

  poptFreeContext(pc);
  exit(exitcode);
}

int main(int argc, char const** argv) {
  poptContext  pc;
  char* display = getenv("DISPLAY");
  int c;
  int rc = 0;

  struct poptOption const opts[] = {
    POPT_AUTOHELP
    { "display",     'd', POPT_ARG_STRING,    &display,     0, "Optional display argument",       "<display>" },
    { NULL,          0,   0,                  NULL,         0, NULL,                                              NULL        }
  };

  pc = poptGetContext(NULL, argc, argv, opts, 0);
//  poptSetOtherOptionHelp(pc, "[OPTION...] <PAR file>");

  if ((c = poptGetNextOpt(pc)) != -1) {
    char buf[100];
    snprintf(buf, sizeof (buf), "%s: %s",
             poptBadOption(pc, POPT_BADOPTION_NOALIAS),
             poptStrerror(c));
    usage(pc, 10, buf);
  }

  if (poptGetArg(pc) != NULL) {
    usage(pc, 10, "Unrecognized extra argument.");
  }

  if (!glgfx_init(glgfx_tag_end)) {
    fprintf(stderr, "Unable to initialize glgfx.\n");
    rc = 20;
  }
  else {
    Display* d = XOpenDisplay(display);

    if (d == NULL) {
      fprintf(stderr, "Unable to open display.\n");
      rc = 20;
    }
    else {
      int error;
      int event;

      if (!XQueryExtension(d, COMPOSITE_NAME, &composite_request, &event, &error)) {
	fprintf(stderr, "Unable to find XComposite X11 extension.\n");
	rc = 20;
      }
      else {
	int major = 0;
	int minor = 0;

	XCompositeQueryVersion (d, &major, &minor);

	if (major == 0 && minor < 3) {
	  fprintf(stderr, "XComposite X11 extension version >= 0.3 is required.\n");
	  rc = 20;
	}
	else {
	  struct screen* s = create_screen(display, d, DefaultScreen(d));

	  if (s == NULL) {
	    rc = 20;
	  }
	  else {
	    rc = handle_screen(s);
	  }

	  destroy_screen(s);
	}
      }

      XCloseDisplay(d);
    }

    glgfx_cleanup();
  }

  poptFreeContext(pc);
  
  return rc;
}
