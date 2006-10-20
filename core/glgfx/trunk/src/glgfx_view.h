#ifndef arp2_glgfx_glgfx_view_h
#define arp2_glgfx_glgfx_view_h

#include <glgfx.h>
#include <glgfx_sprite.h>
#include <glgfx_viewport.h>

#ifdef __cplusplus
extern "C" {
#endif

struct glgfx_view;

struct glgfx_view* glgfx_view_create(void);

void glgfx_view_destroy(struct glgfx_view* view);


bool glgfx_view_addviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport);

bool glgfx_view_remviewport(struct glgfx_view* view,
			    struct glgfx_viewport* viewport);

int glgfx_view_numviewports(struct glgfx_view* view);


bool glgfx_view_addsprite(struct glgfx_view* view,
			    struct glgfx_sprite* viewport);

bool glgfx_view_remsprite(struct glgfx_view* view,
			    struct glgfx_sprite* viewport);

int glgfx_view_numsprites(struct glgfx_view* view);


#ifdef __cplusplus
}
#endif

#endif /* arp2_glgfx_glgfx_view_h */
