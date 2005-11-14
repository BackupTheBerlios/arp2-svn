#ifndef arp2_glgfx_glgfx_monitor_h
#define arp2_glgfx_glgfx_monitor_h

#include <glgfx.h>
#include <glgfx_view.h>
#include <inttypes.h>

struct glgfx_monitor;
struct glgfx_context;

enum glgfx_monitor_attr {
  glgfx_monitor_attr_unknown = glgfx_tag_user + 3000,

  glgfx_monitor_attr_friend,
  glgfx_monitor_attr_fullscreen,

  glgfx_monitor_attr_width,
  glgfx_monitor_attr_height,
  glgfx_monitor_attr_format,
  glgfx_monitor_attr_vsync,
  glgfx_monitor_attr_hsync,
  glgfx_monitor_attr_dotclock,

  glgfx_monitor_attr_max
} __attribute__((mode(__pointer__)));

struct glgfx_monitor* glgfx_monitor_create(char const* display_name,
					   struct glgfx_tagitem const* tags);
void glgfx_monitor_destroy(struct glgfx_monitor* monitor);

bool glgfx_monitor_setattrs_a(struct glgfx_monitor* monitor, 
			      struct glgfx_tagitem const* tags);

bool glgfx_monitor_getattr(struct glgfx_monitor* bm,
			   enum glgfx_monitor_attr attr,
			   intptr_t* storage);


#define glgfx_monitor_setattrs(monitor, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_monitor_setattrs_a((viewport), (struct glgfx_tagitem const*) _tags); })


struct glgfx_context* glgfx_monitor_createcontext(struct glgfx_monitor* monitor);
struct glgfx_context* glgfx_monitor_getcontext(struct glgfx_monitor* monitor);

bool glgfx_monitor_addview(struct glgfx_monitor* monitor,
			   struct glgfx_view* view);
bool glgfx_monitor_loadview(struct glgfx_monitor* monitor,
			    struct glgfx_view* view);
bool glgfx_monitor_remview(struct glgfx_monitor* monitor,
			   struct glgfx_view* view);


bool glgfx_monitor_render(struct glgfx_monitor* monitor);

#endif /* arp2_glgfx_glgfx_monitor_h */

