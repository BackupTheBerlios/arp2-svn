#ifndef arp2_glgfx_glgfx_intern_h
#define arp2_glgfx_glgfx_intern_h

#include <glib.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

struct glgfx_monitor {
    char const*          name;
    Display*             display;
    Window               window;
    GLXContext           context;

    Atom                 xa_win_state;

    XF86VidModeModeLine  mode;
    int                  dotclock;
    XF86VidModeMonitor   monitor_info;
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

#endif /* arp2_glgfx_glgfx_intern_h */
