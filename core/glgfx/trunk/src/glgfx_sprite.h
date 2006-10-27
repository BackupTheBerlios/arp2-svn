#ifndef arp2_glgfx_glgfx_sprite_h
#define arp2_glgfx_glgfx_sprite_h

#include <glgfx.h>
#include <glgfx_bitmap.h>

#ifdef __cplusplus
extern "C" {
#endif

struct glgfx_sprite;

enum glgfx_sprite_attr {
  glgfx_sprite_attr_unknown = glgfx_tag_user + 5000,
  
  glgfx_sprite_attr_width,
  glgfx_sprite_attr_height,
  glgfx_sprite_attr_x,
  glgfx_sprite_attr_y,
  glgfx_sprite_attr_bitmap,
  glgfx_sprite_attr_interpolated,		/* Default is true */
  
  glgfx_sprite_attr_max
} __attribute__((mode(__pointer__)));


struct glgfx_sprite* glgfx_sprite_create_a(struct glgfx_tagitem const* tags);
void glgfx_sprite_destroy(struct glgfx_sprite* sprite);
bool glgfx_sprite_setattrs_a(struct glgfx_sprite* sprite, 
			     struct glgfx_tagitem const* tags);

bool glgfx_sprite_getattr(struct glgfx_sprite* sprite,
			  enum glgfx_sprite_attr attr,
			  intptr_t* storage);


#define glgfx_sprite_create(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_sprite_create_a((struct glgfx_tagitem const*) (void*) _tags); })

#define glgfx_sprite_setattrs(sprite, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ##__VA_ARGS__ }; \
    glgfx_sprite_setattrs_a((sprite), (struct glgfx_tagitem const*) (void*) _tags); })


#ifdef __cplusplus
}
#endif

#endif /* arp2_glgfx_glgfx_sprite_h */
