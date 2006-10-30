
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <glgfx.h>
#include <glgfx_monitor.h>
#include <popt.h>
#include <stdio.h>
#include <stdlib.h>

int composite_request;

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
	  int    screen  = DefaultScreen(d);
	  Window root    = XRootWindow(d, screen);
	  Window overlay = XCompositeGetOverlayWindow(d, root);

	  if (overlay == None) {
	    fprintf(stderr, "Unable to create XComposite overlay window.\n");
	    rc = 20;
	  }
	  else {
	    struct glgfx_monitor* monitor = glgfx_monitor_create(
	      display,
	      glgfx_monitor_attr_xparent, overlay,
	      glgfx_monitor_attr_fullscreen, false,
	      glgfx_tag_end);

	    if (monitor == NULL) {
	      fprintf(stderr, "Unable to create glgfx monitor.\n");
	      rc = 20;
	    }
	    else {
	      XCompositeRedirectSubwindows (d, root, CompositeRedirectManual);
	      XSelectInput (d, root, SubstructureNotifyMask);

	      

	      sleep(1);
	      glgfx_monitor_destroy(monitor);
	    }

	    XCompositeReleaseOverlayWindow(d, overlay);
	  }
	}
      }

      XCloseDisplay(d);
    }

    glgfx_cleanup();
  }

  poptFreeContext(pc);
  
  return rc;
}
