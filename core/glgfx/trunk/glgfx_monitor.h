#ifndef arp2_glgfx_glgfx_monitor_h
#define arp2_glgfx_glgfx_monitor_h

#include <glgfx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glx.h>

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

struct glgfx_monitor* glgfx_monitor_create(char const* display_name,
					   struct glgfx_monitor const* friend);
void glgfx_monitor_destroy(struct glgfx_monitor* monitor);
void glgfx_monitor_fullscreen(struct glgfx_monitor* monitor, bool fullscreen);
bool glgfx_monitor_select(struct glgfx_monitor* monitor);
bool glgfx_monitor_waitblit(struct glgfx_monitor* monitor);
bool glgfx_monitor_waittof(struct glgfx_monitor* monitor);

#endif /* arp2_glgfx_glgfx_monitor_h */

