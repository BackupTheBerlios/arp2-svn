#ifndef arp2_glgfx_glgfx_viewport_h
#define arp2_glgfx_glgfx_viewport_h

#include <glgfx.h>
#include <glgfx_bitmap.h>

struct glgfx_rasinfo;
struct glgfx_viewport;

enum glgfx_viewport_attr {
  glgfx_viewport_attr_unknown = glgfx_tag_user,
  
  glgfx_viewport_attr_width,
  glgfx_viewport_attr_height,
  glgfx_viewport_attr_x,
  glgfx_viewport_attr_y,
  
  glgfx_viewport_attr_max
} __attribute__((mode(__pointer__)));

enum glgfx_rasinfo_attr {
  glgfx_rasinfo_attr_unknown = glgfx_tag_user,
  
  glgfx_rasinfo_attr_width,
  glgfx_rasinfo_attr_height,
  glgfx_rasinfo_attr_x,
  glgfx_rasinfo_attr_y,
  glgfx_rasinfo_attr_bitmap,
  
  glgfx_rasinfo_attr_max
} __attribute__((mode(__pointer__)));


struct glgfx_viewport* glgfx_viewport_create_a(struct glgfx_tagitem const* tags);
void glgfx_viewport_destroy(struct glgfx_viewport* viewport);
bool glgfx_viewport_setattrs_a(struct glgfx_viewport* viewport, 
			       struct glgfx_tagitem const* tags);

bool glgfx_viewport_getattr(struct glgfx_viewport* viewport,
			    enum glgfx_viewport_attr attr,
			    intptr_t* storage);


struct glgfx_rasinfo* glgfx_viewport_addbitmap_a(struct glgfx_viewport* viewport,
						 struct glgfx_bitmap* bitmap,
						 struct glgfx_tagitem const* tags);
bool glgfx_viewport_rembitmap(struct glgfx_viewport* viewport,
			      struct glgfx_rasinfo* rasinfo);

int glgfx_viewport_numbitmaps(struct glgfx_viewport* viewport);


bool glgfx_rasinfo_setattrs_a(struct glgfx_rasinfo* rasinfo,
			      struct glgfx_tagitem const* tags);

bool glgfx_rasinfo_getattr(struct glgfx_rasinfo* rasinfo,
			   enum glgfx_rasinfo_attr attr,
			   intptr_t* storage);


#define glgfx_viewport_create(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_viewport_create_a((struct glgfx_tagitem const*) _tags); })

#define glgfx_viewport_setattrs(viewport, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_viewport_setattrs_a((viewport), (struct glgfx_tagitem const*) _tags); })

#define glgfx_viewport_addbitmap(viewport, bitmap, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_viewport_addbitmap_a((viewport), (bitmap), (struct glgfx_tagitem const*) _tags); })

#define glgfx_rasinfo_setattrs(rasinfo, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_rasinfo_setattr_a((rasinfo),(struct glgfx_tagitem const*) _tags); })


#endif /* arp2_glgfx_glgfx_viewport_h */

