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

struct glgfx_bitmap;

struct glgfx_context {
    struct glgfx_monitor*   monitor;
    GLXContext              glx_context;
    GLuint                  fbo;
    struct glgfx_bitmap*    temp_bitmap;

    bool                    fbo_bound;
    struct glgfx_bitmap*    fbo_bitmap;

    GHashTable*             extensions;
    bool                    have_GL_EXT_framebuffer_object;
    bool                    have_GL_texture_rectangle;
    bool                    have_GLX_SGI_video_sync;
    bool                    have_GL_ARB_vertex_buffer_object;
    bool                    have_GL_ARB_pixel_buffer_object;
    
    bool                    miss_pixel_ops;
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

    GList*                  views;
};

struct glgfx_view {
    GList* viewports;
    GList* sprites;
    bool   has_changed;
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
      uint8_t*                buffer;
    };
    size_t                  pbo_size;
    size_t                  pbo_bytes_per_row;
    bool                    locked;
    GLenum                  locked_usage;
    GLenum                  locked_access;
    void*                   locked_memory;

    bool                    has_changed;
};


struct glgfx_rasinfo {
    struct glgfx_bitmap* bitmap;
    int                  xoffset;
    int                  yoffset;
    int                  width;
    int                  height;
    bool                 has_changed;
};


struct glgfx_viewport {
    int    width;
    int    height;
    int    xoffset;
    int    yoffset;
    GList* rasinfos;
    bool   has_changed;
};


struct glgfx_sprite {
    struct glgfx_bitmap* bitmap;
    int                  x;
    int                  y;
    int                  width;
    int                  height;
    bool                 has_changed;
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


#define GLGFX_CHECKERROR() glgfx_checkerror(__PRETTY_FUNCTION__, __FILE__, __LINE__);
void glgfx_checkerror(char const* func, char const* file, int line);

bool glgfx_monitor_waittof(struct glgfx_monitor* monitor);
bool glgfx_monitor_swapbuffers(struct glgfx_monitor* monitor);

bool glgfx_view_haschanged(struct glgfx_view* view);
bool glgfx_viewport_haschanged(struct glgfx_viewport* viewport);
bool glgfx_sprite_haschanged(struct glgfx_sprite* sprite);
bool glgfx_bitmap_haschanged(struct glgfx_bitmap* bitmap);

bool glgfx_view_render(struct glgfx_view* view);
bool glgfx_view_rendersprites(struct glgfx_view* view);
bool glgfx_viewport_render(struct glgfx_viewport* viewport);
bool glgfx_sprite_render(struct glgfx_sprite* sprite);

#endif /* arp2_glgfx_glgfx_intern_h */
