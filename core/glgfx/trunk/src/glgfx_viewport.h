#ifndef arp2_glgfx_glgfx_viewport_h
#define arp2_glgfx_glgfx_viewport_h

#include <glgfx.h>
#include <glgfx_bitmap.h>
#include <glgfx_monitor.h>

struct glgfx_rasinfo;
struct glgfx_viewport;

enum glgfx_viewport_tag {
  glgfx_viewport_tag_unknown = glgfx_tag_user,
  
  glgfx_viewport_tag_width,
  glgfx_viewport_tag_height,
  glgfx_viewport_tag_xoffset,
  glgfx_viewport_tag_yoffset,
  
  glgfx_viewport_tag_max
} __attribute__((mode(__pointer__)));


struct glgfx_viewport* glgfx_viewport_create_a(struct glgfx_tagitem const* tags);
void glgfx_viewport_destroy(struct glgfx_viewport* viewport);
bool glgfx_viewport_move_a(struct glgfx_viewport* viewport, struct glgfx_tagitem const* tags);

struct glgfx_rasinfo* glgfx_viewport_addbitmap_a(struct glgfx_viewport* viewport,
						 struct glgfx_bitmap* bitmap,
						 struct glgfx_tagitem const* tags);
bool glgfx_viewport_rembitmap(struct glgfx_viewport* viewport,
			      struct glgfx_rasinfo* rasinfo);

bool glgfx_viewport_setbitmap_a(struct glgfx_viewport* viewport,
				struct glgfx_rasinfo* rasinfo,
				struct glgfx_bitmap* bitmap,
				struct glgfx_tagitem const* tags);

int glgfx_viewport_numbitmaps(struct glgfx_viewport* viewport);

bool glgfx_viewport_render(struct glgfx_viewport* viewport);


#define glgfx_viewport_create(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_viewport_create_a((struct glgfx_tagitem const*) _tags); })

#define glgfx_viewport_move(viewport, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_viewport_move_a((viewport), (struct glgfx_tagitem const*) _tags); })

#define glgfx_viewport_addbitmap(viewport, bitmap, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_viewport_addbitmap_a((viewport), (bitmap), (struct glgfx_tagitem const*) _tags); })

#define glgfx_viewport_setbitmap(viewport, rasinfo, bitmap, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_viewport_addbitmap_a((viewport), (rasinfo), (bitmap), (struct glgfx_tagitem const*) _tags); })


#endif /* arp2_glgfx_glgfx_viewport_h */

