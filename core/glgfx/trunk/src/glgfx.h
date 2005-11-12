
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


/*** Context handling ********************************************************/

struct glgfx_context;
struct glgfx_monitor;
struct glgfx_bitmap;

struct glgfx_context* glgfx_context_create(struct glgfx_monitor* monitor);
bool glgfx_context_select(struct glgfx_context* context);
struct glgfx_context* glgfx_context_getcurrent(void);
bool glgfx_context_bindfbo(struct glgfx_context* context, struct glgfx_bitmap* bitmap);
bool glgfx_context_unbindfbo(struct glgfx_context* context);
bool glgfx_context_destroy(struct glgfx_context* context);


/*** System setup ************************************************************/

enum glgfx_init_tag {
  glgfx_init_tag_unknown = glgfx_tag_user,

  /* No tags defined yet */

  glgfx_init_tag_max
} __attribute__((mode(__pointer__)));

enum glgfx_create_monitors_tag {
  glgfx_create_monitors_tag_unknown = glgfx_tag_user,
  
  glgfx_create_monitors_tag_display, /* data is display name string */
  
  glgfx_create_monitors_tag_max
} __attribute__((mode(__pointer__)));


bool glgfx_init_a(struct glgfx_tagitem const* tags);
int glgfx_createmonitors_a(struct glgfx_tagitem const* tags);
void glgfx_destroymonitors(void);
void glgfx_cleanup();


#define glgfx_init(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_init_a((struct glgfx_tagitem const*) _tags); })

#define glgfx_createmonitors(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_createmonitors_a((struct glgfx_tagitem const*) _tags); })



/*** Private stuff ***********************************************************/

#define max_monitors  8               // Four cards, two outputs/card max
extern int                   glgfx_num_monitors;
extern struct glgfx_monitor* glgfx_monitors[max_monitors];

#endif /* arp2_glgfx_glgfx_h */
