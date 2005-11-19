
#include "glgfx-config.h"

#include <GL/gl.h>
#include <GL/glx.h>

#define DRAWABLES 256
#define CONTEXTS  256

static XID drawables[DRAWABLES];

static int find_drawable() {
  int i;

  for (i = DRAWABLES-1; i > 0 && drawables[i] != 0; --i);
  return i;
}

GLXWindow glgfxCreateWindow(Display *dpy,
			    GLXFBConfig config,
			    Window window,
			    const int *AttributeList) {
  int i = find_drawable();
  
  if (i != 0) {
    drawables[i] = window;
  }

  return i;
}


void glgfxDestroyWindow(Display *dpy,
			GLXWindow Window) {
  if (Window > 0 && 
      Window < DRAWABLES && 
      drawables[Window] != 0) {
    drawables[Window] = 0;
  }
}


GLXPbuffer glgfxCreatePbuffer(Display *dpy,
			      GLXFBConfig config,
			      const int *AttributeList) {
  int i = find_drawable();
  
  if (i != 0) {
    drawables[i] = glXCreatePbuffer(dpy, config, AttributeList);

    if (drawables[i] == 0) {
      i = 0;
    }
  }

  return i;
}


void glgfxDestroyPbuffer(Display *dpy,
			 GLXPbuffer Pbuffer) {
  if (Pbuffer > 0 && 
      Pbuffer < DRAWABLES && 
      drawables[Pbuffer] != 0) {
    glXDestroyPbuffer(dpy, drawables[Pbuffer]);
    drawables[Pbuffer] = 0;
  }
}


/* GLXContext glgfxCreateNewContext(Display *dpy, */
/* 				 GLXFBConfig config, */
/* 				 int renderType, */
/* 				 GLXContext ShareList, */
/* 				 Bool Direct) { */
/*   GLXContext ctx; */

/*   XVisualInfo* vis = glXGetVisualFromFBConfig(dpy, config); */

/*   if (vis == 0) { */
/*     return 0; */
/*   } */

/*   ctx = glXCreateContext(dpy, vis, ShareList, Direct); */
/*   XFree(vis); */

/*   return ctx; */
/* } */

#include <stdio.h>
#include <pthread.h>

Bool glgfxMakeContextCurrent(Display *dpy,
			     GLXDrawable draw,
			     GLXDrawable read,
			     GLXContext context) {
  if (draw != read ||
      draw <= 0 || 
      draw >= DRAWABLES) {
    return False;
  }

  printf("%ld: making %d (drawable %d/%d) current\n", 
	 pthread_self(), context, draw, drawables[draw]);
  return glXMakeCurrent(dpy, drawables[draw], context);
}

void glgfxSwapBuffers(Display *dpy,
		      GLXDrawable drawable) {
  if (drawable > 0 &&
      drawable < DRAWABLES) {
    return glXSwapBuffers(dpy, drawables[drawable]);
  }
}
