#ifndef arp2_glgfx_glgfx_intern_h
#define arp2_glgfx_glgfx_intern_h

#include <stdbool.h>
#include <inttypes.h>
#include <glib.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include <stdio.h>
#define BUG(...) printf(__VA_ARGS__)

#if defined(DEBUG)
# define D(x) (x)
#else
# define D(x)
#endif

#include "glgfx_pixel.h"

struct glgfx_monitor {
    char const*             name;
    Display*                display;
    Window                  window;
    GLXContext              context;

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
    int                     flags;
    enum glgfx_pixel_format format;
    GLuint                  texture;
    GLuint                  pbo;
    size_t                  size;
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
