#ifndef arp2_glgfx_glgfx_glext_h
#define arp2_glgfx_glgfx_glext_h

#include "glgfx.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/glxext.h>

#define GLGFX_GLEXT(type, name) extern type name;
# include "glgfx_glext_extensions.h"
#undef GLGFX_GLEXT

bool glgfx_glext_init();

#endif /* arp2_glgfx_glgfx_glext_h */
