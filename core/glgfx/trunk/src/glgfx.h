#ifndef arp2_glgfx_glgfx_h
#define arp2_glgfx_glgfx_h

#include <stdbool.h>
#include <inttypes.h>


/*** Tag handling ************************************************************/

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


/*** Montor handling *********************************************************/

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
void glgfx_cleanup();
bool glgfx_create_monitors_a(struct glgfx_tagitem const* tags);
void glgfx_destroy_monitors(void);
bool glgfx_waitblit(void);
bool glgfx_swapbuffers(void);


#define glgfx_init(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_init_a((struct glgfx_tagitem const*) _tags); })

#define glgfx_create_monitors(tag1, ...) \
  ({ intptr_t const _tags[] = { tag1, ## __VA_ARGS__ }; \
    glgfx_create_monitors_a((struct glgfx_tagitem const*) _tags); })


/*** Private stuff ***********************************************************/

struct glgfx_monitor;
#define max_monitors  8               // Four cards, two outputs/card max
extern int                   glgfx_num_monitors;
extern struct glgfx_monitor* glgfx_monitors[max_monitors];


#define check_error() glgfx_check_error(__PRETTY_FUNCTION__, __FILE__, __LINE__);
void glgfx_check_error(char const* func, char const* file, int line);

#endif /* arp2_glgfx_glgfx_h */
