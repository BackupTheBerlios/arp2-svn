#ifndef arp2_glgfx_glgfx_viewport_h
#define arp2_glgfx_glgfx_viewport_h

#include "glgfx.h"
#include "glgfx_bitmap.h"
#include "glgfx_monitor.h"

#include <glib.h>
#include <GL/gl.h>

struct glgfx_rasinfo {
    struct glgfx_bitmap* bitmap;
    int                  xoffset;
    int                  yoffset;
};

struct glgfx_viewport {
    int                     width;
    int                     height;
    int                     xoffset;
    int                     yoffset;
    GList*                  rasinfos;
};


struct glgfx_viewport* glgfx_viewport_create(int width, int height,
					     int xoffset, int yoffset);

void glgfx_viewport_destroy(struct glgfx_viewport* viewport);

bool glgfx_viewport_move(struct glgfx_viewport* viewport,
			 int width, int height,
			 int xoffset, int yoffset);

struct glgfx_rasinfo* glgfx_viewport_addbitmap(struct glgfx_viewport* viewport,
					       struct glgfx_bitmap* bitmap,
					       int xoffset, int yoffset);

bool glgfx_viewport_rembitmap(struct glgfx_viewport* viewport,
			      struct glgfx_rasinfo* rasinfo);

bool glgfx_viewport_setbitmap(struct glgfx_viewport* viewport,
			      struct glgfx_rasinfo* rasinfo,
			      struct glgfx_bitmap* bitmap,
			      int xoffset, int yoffset);

int glgfx_viewport_numbitmaps(struct glgfx_viewport* viewport);

bool glgfx_viewport_render(struct glgfx_viewport* viewport);

#endif /* arp2_glgfx_glgfx_viewport_h */

