#ifndef arp2_glgfx_glgfx_glxemul_h
#define arp2_glgfx_glgfx_glxemul_h

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

#define glXCreateWindow(d,c,w,a)        glgfxCreateWindow(d,c,w,a)
#define glXDestroyWindow(d,w)           glgfxDestroyWindow(d,w)
#define glXCreatePbuffer(d,c,a)         glgfxCreatePbuffer(d,c,a)
#define glXDestroyPbuffer(d,p)          glgfxDestroyPbuffer(d,p)
#define glXCreateNewContext(d,p,r,s,di) glgfxCreateNewContext(d,p,r,s,di)
#define glXMakeContextCurrent(d,w,r,c)  glgfxMakeContextCurrent(d,w,r,c)
#define glXSwapBuffers(d, dr)           glgfxSwapBuffers(d,dr)


GLXWindow glgfxCreateWindow(Display *dpy,
			    GLXFBConfig config,
			    Window window,
			    const int *AttributeList);

void glgfxDestroyWindow(Display *dpy,
			GLXWindow Window);

GLXPbuffer glgfxCreatePbuffer(Display *dpy,
			      GLXFBConfig config,
			      const int *AttributeList);

void glgfxDestroyPbuffer(Display *dpy,
			 GLXPbuffer Pbuffer);

GLXContext glgfxCreateNewContext(Display *dpy,
				 GLXFBConfig config,
				 int renderType,
				 GLXContext ShareList,
				 Bool Direct);

Bool glgfxMakeContextCurrent(Display *dpy,
			     GLXDrawable draw,
			     GLXDrawable read,
			     GLXContext context);

void glgfxSwapBuffers(Display *dpy,
		      GLXDrawable drawable);

#endif /* arp2_glgfx_glgfx_glxemul_h */
