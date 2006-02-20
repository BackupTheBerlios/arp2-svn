#ifndef arp2_glgfx_glgfx_h
#define arp2_glgfx_glgfx_h

#include <stdbool.h>
#include <inttypes.h>


/*** Tag/attribute handling **************************************************/

enum glgfx_tag {
  glgfx_tag_end    = 0,
  glgfx_tag_ignore = 1,
  glgfx_tag_more   = 2,
  glgfx_tag_skip   = 3,
  glgfx_tag_user   = 0x80000000
} __attribute__((mode(__pointer__)));

struct glgfx_tagitem {
    enum glgfx_tag tag;
    intptr_t data;
};

struct glgfx_tagitem const* glgfx_nexttagitem(struct glgfx_tagitem const** taglist_ptr);

typedef bool glgfx_getattr_proto(void* object, 
				 enum glgfx_tag attr, 
				 intptr_t* storage);

int glgfx_getattrs_a(void* object, 
		     glgfx_getattr_proto* fn, 
		     struct glgfx_tagitem const* tags);


#define glgfx_getattrs(object, fn, tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_getattrs_a((object), (fn), (struct glgfx_tagitem const*) _tags); })


/*** Half-float support ******************************************************/

uint16_t glgfx_float2half(float f);
float glgfx_half2float(uint16_t h);


/*** System setup ************************************************************/

enum glgfx_init_tag {
  glgfx_init_unknown = glgfx_tag_user + 1000,

  glgfx_init_signal,

  glgfx_init_max
} __attribute__((mode(__pointer__)));


bool glgfx_init_a(struct glgfx_tagitem const* tags);
void glgfx_cleanup();


#define glgfx_init(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_init_a((struct glgfx_tagitem const*) _tags); })

#endif /* arp2_glgfx_glgfx_h */
