#ifndef arp2_glgfx_glgfx_intern_h
#define arp2_glgfx_glgfx_intern_h

#include <glib.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#define BUG(...) printf(__VA_ARGS__)

#if defined(DEBUG)
# define D(x) (x)
#else
# define D(x)
#endif

#include "glgfx_pixel.h"

extern pthread_mutex_t glgfx_mutex;

struct glgfx_context {
    struct glgfx_monitor*   monitor;
    GLXContext              glx_context;
    GLuint                  fbo;

    bool                    fbo_bound;

    GHashTable*             extensions;
    bool                    have_GL_EXT_framebuffer_object;
    bool                    have_GL_texture_rectangle;
    bool                    have_GLX_SGI_video_sync;
    bool                    have_GL_ARB_vertex_buffer_object;
    bool                    have_GL_ARB_pixel_buffer_object;
};


struct glgfx_monitor {
    char const*             name;
    Display*                display;
    Window                  window;
    struct glgfx_context*   main_context;
    XVisualInfo*            vinfo;
    struct glgfx_monitor const*   friend;

    Atom                    xa_win_state;

    enum glgfx_pixel_format format;
    XF86VidModeModeLine     mode;
    int                     dotclock;
    XF86VidModeMonitor      monitor_info;
};


struct glgfx_bitmap {
    struct glgfx_monitor*   monitor;
    int                     width;
    int                     height;
    int                     bits;
    enum glgfx_pixel_format format;
    GLuint                  texture;
    union {
      GLuint                  pbo;
      void*                   buffer;
    };
    size_t                  pbo_size;
    size_t                  pbo_bytes_per_row;
    bool                    locked;
    GLenum                  locked_usage;
    GLenum                  locked_access;
    void*                   locked_memory;
};


struct glgfx_rasinfo {
    struct glgfx_bitmap* bitmap;
    int                  xoffset;
    int                  yoffset;
    int                  width;
    int                  height;
};


struct glgfx_viewport {
    int    width;
    int    height;
    int    xoffset;
    int    yoffset;
    GList* rasinfos;
};


struct glgfx_sprite {
    struct glgfx_bitmap* bitmap;
    int                  x;
    int                  y;
    int                  width;
    int                  height;
};


struct pixel_info {
    GLint   internal_format;
    GLenum  format;
    GLenum  type;
    size_t  size;
    bool    is_bigendian;
    bool    is_rgb;
    uint8_t redbits;
    uint8_t redshift;
    uint8_t greenbits;
    uint8_t greenshift;
    uint8_t bluebits;
    uint8_t blueshift;
    uint8_t alphabits;
    uint8_t alphashift;
};

extern struct pixel_info formats[glgfx_pixel_format_max];

#endif /* arp2_glgfx_glgfx_intern_h */
