
#include "glgfx-config.h"

#include <GL/gl.h>
#include <GL/glx.h>
/* #include <GL/glext.h> */
/* #include <GL/glxext.h> */
/* #if HAVE_GL_GLATI_H */
/* # include <GL/glATI.h> */
/* #endif */
#if HAVE_GL_GLXATI_H
# include <GL/glxATI.h>
#endif

static struct {
  XID drawable;
} drawables[128];

static int find_drawable() {
  int i;

  for (i = 127; i > 0 && drawables[i].drawable != 0; --i);
  return i;
}

GLXWindow glgfxCreateWindow(Display *dpy,
			    GLXFBConfig config,
			    Window window,
			    const int *AttributeList) {
  int i = find_drawable();
  
  if (i != 0) {
    drawables[i].drawable = window;
  }

  return i;
}


void glgfxDestroyWindow(Display *dpy,
			GLXWindow Window) {
  if (Window > 0 && 
      Window < 128 && 
      drawables[Window].drawable != 0) {
    drawables[Window].drawable = 0;
  }
}


GLXPbuffer glgfxCreatePbuffer(Display *dpy,
			      GLXFBConfig config,
			      const int *AttributeList) {
  int i = find_drawable();
  
  if (i != 0) {
    drawables[i].drawable = glXCreatePbuffer(dpy, config, AttributeList);

    if (drawables[i].drawable == 0) {
      i = 0;
    }
  }

  return i;
}


void glgfxDestroyPbuffer(Display *dpy,
			 GLXPbuffer Pbuffer) {
  if (Pbuffer > 0 && 
      Pbuffer < 128 && 
      drawables[Pbuffer].drawable != 0) {
    glXDestroyPbuffer(dpy, drawables[Pbuffer].drawable);
    drawables[Pbuffer].drawable = 0;
  }
}


GLXContext glgfxCreateNewContext(Display *dpy,
				 GLXFBConfig config,
				 int renderType,
				 GLXContext ShareList,
				 Bool Direct) {

  GLXContext ctx;
  XVisualInfo* vis = glXGetVisualFromFBConfig(dpy, config);

  if (vis == 0) {
    return 0;
  }

  ctx = glXCreateContext(dpy, vis, ShareList, Direct);
  XFree(vis);

  return ctx;
}


Bool glgfxMakeContextCurrent(Display *dpy,
			     GLXDrawable draw,
			     GLXDrawable read,
			     GLXContext context) {
  if (draw != read ||
      draw <= 0 || 
      draw >= 128) {
    return False;
  }

  return glXMakeCurrent(dpy, drawables[draw].drawable, context);
}

void glgfxSwapBuffers(Display *dpy,
		      GLXDrawable drawable) {
  if (drawable > 0 &&
      drawable < 128) {
    return glXSwapBuffers(dpy, drawables[drawable].drawable);
  }
}
