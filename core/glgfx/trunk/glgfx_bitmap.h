#ifndef arp2_glgfx_glgfx_bitmap_h
#define arp2_glgfx_glgfx_bitmap_h

#include <glgfx.h>
#include <glgfx_monitor.h>

#include <GL/gl.h>

struct glgfx_bitmap {
    struct glgfx_monitor*   monitor;
    int                     width;
    int                     height;
    int                     bits;
    int                     flags;
    enum glgfx_pixel_format format;
    GLuint                  texture;
    GLuint                  pbo;
    size_t                  size;
    bool                    locked;
    GLenum                  locked_usage;
    GLenum                  locked_access;
    void*                   locked_memory;
};


struct glgfx_bitmap* glgfx_bitmap_create(int width, int height, int bits,
					 int flags, struct glgfx_bitmap* friend,
					 int format, struct glgfx_monitor* monitor);
void glgfx_bitmap_destroy(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_lock(struct glgfx_bitmap* bitmap, bool read, bool write);
bool glgfx_bitmap_update(struct glgfx_bitmap* bitmap, void* data, size_t size);
void* glgfx_bitmap_map(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_unmap(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_unlock(struct glgfx_bitmap* bitmap);

bool glgfx_bitmap_select(struct glgfx_bitmap* bitmap);
bool glgfx_bitmap_waitblit(struct glgfx_bitmap* bitmap);

#endif /* arp2_glgfx_glgfx_bitmap_h */

