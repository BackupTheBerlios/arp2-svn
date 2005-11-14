#ifndef arp2_glgfx_glgfx_context_h
#define arp2_glgfx_glgfx_context_h

#include <glgfx.h>

struct glgfx_context;
struct glgfx_monitor;
struct glgfx_bitmap;

struct glgfx_context* glgfx_context_create(struct glgfx_monitor* monitor);
bool glgfx_context_select(struct glgfx_context* context);
struct glgfx_context* glgfx_context_getcurrent(void);
bool glgfx_context_destroy(struct glgfx_context* context);

#endif /* arp2_glgfx_glgfx_context_h */

