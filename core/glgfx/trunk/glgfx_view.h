#ifndef arp2_glgfx_glgfx_view_h
#define arp2_glgfx_glgfx_view_h

#include <glgfx-config.h>
#include <glgfx.h>
#include <glgfx_monitor.h>
#include <glgfx_viewport.h>

#include <glib.h>

struct glgfx_view {
    struct glgfx_monitor* monitor;
    GList*                viewports;
};

struct glgfx_view* glgfx_view_create(struct glgfx_monitor* monitor);

void glgfx_view_destroy(struct glgfx_view* view);

bool glgfx_view_addviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport);

bool glgfx_view_remviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport);

int glgfx_view_numviewports(struct glgfx_view* view);

bool glgfx_view_render(struct glgfx_view* view);

#endif /* arp2_glgfx_glgfx_view_h */

