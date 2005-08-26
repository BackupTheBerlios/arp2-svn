#ifndef ARP2_glgfx_glgfx_h
#define ARP2_glgfx_glgfx_h

#include <stdbool.h>
#include <inttypes.h>

bool glgfx_init(void);
void glgfx_cleanup();
bool glgfx_create_monitors(void);
void glgfx_destroy_monitors(void);
bool glgfx_waitblit(void);
bool glgfx_swapbuffers(void);

enum glgfx_tag {
  glgfx_tag_done   = 0,
  glgfx_tag_end    = 0,
  glgfx_tag_ignore = 1,
  glgfx_tag_more   = 2,
  glgfx_tag_skip   = 3,
  glgfx_tag_user   = 0x80000000
};

struct glgfx_tagitem {
    enum glgfx_tag tag;
    uintptr_t data;
};

struct glgfx_tagitem* glgfx_nexttagitem(struct glgfx_tagitem** taglist_ptr);

struct glgfx_monitor;
#define max_monitors  8               // Four cards, two outputs/card max
extern int                   glgfx_num_monitors;
extern struct glgfx_monitor* glgfx_monitors[max_monitors];


#define check_error() glgfx_check_error(__PRETTY_FUNCTION__, __FILE__, __LINE__);
void glgfx_check_error(char const* func, char const* file, int line);

#endif /* ARP2_glgfx_glgfx_h */

