#ifndef arp2_glgfx_glgfx_monitor_h
#define arp2_glgfx_glgfx_monitor_h

#include <glgfx.h>
#include <inttypes.h>

struct glgfx_monitor;

enum glgfx_monitor_attr {
  glgfx_monitor_width = 0,
  glgfx_monitor_height,
  glgfx_monitor_format,
  glgfx_monitor_vsync,
  glgfx_monitor_hsync,
  glgfx_monitor_dotclock,

  glgfx_monitor_max
};

struct glgfx_monitor* glgfx_monitor_create(char const* display_name,
					   struct glgfx_monitor const* friend);
void glgfx_monitor_destroy(struct glgfx_monitor* monitor);
void glgfx_monitor_fullscreen(struct glgfx_monitor* monitor, bool fullscreen);
bool glgfx_monitor_select(struct glgfx_monitor* monitor);
bool glgfx_monitor_waitblit(struct glgfx_monitor* monitor);
bool glgfx_monitor_waittof(struct glgfx_monitor* monitor);

bool glgfx_monitor_getattr(struct glgfx_monitor* bm,
			   enum glgfx_monitor_attr attr,
			   uint32_t* storage);



#endif /* arp2_glgfx_glgfx_monitor_h */

