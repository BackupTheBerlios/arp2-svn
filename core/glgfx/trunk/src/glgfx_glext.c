
#include "glgfx_glext.h"
#include "glgfx_intern.h"

#define GLGFX_GLEXT(type, name) type name = 0;
# include "glgfx_glext_extensions.h"
#undef GLGFX_GLEXT

bool glgfx_glext_init() {
  void missing(char const* name) {
    D(BUG("Warning: extension %s is missing!\n", name));
  }

  bool rc = true;

#define GLGFX_GLEXT(type, name) \
  if((name = (type) glXGetProcAddressARB((GLubyte const*) #name)) == NULL) { \
    missing(#name); \
    rc = false; \
  }
# include "glgfx_glext_extensions.h"
#undef GLGFX_GLEXT

  return rc;
}
