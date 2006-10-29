#ifndef arp2_glgfx_glgfx_intern_h
#define arp2_glgfx_glgfx_intern_h

#include <glib.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include "glgfx_pixel.h"

#define BUG(...) printf(__VA_ARGS__)

#if defined(DEBUG)
# define D(x) (x)
#else
# define D(x)
#endif

#include "glgfx_pixel.h"

extern pthread_mutex_t glgfx_mutex;
extern int glgfx_signum;

extern GLenum const glgfx_blend_equations[glgfx_blend_equation_max];
extern GLenum const glgfx_blend_funcs[glgfx_blend_func_max];


struct glgfx_node {
    struct glgfx_node* succ;
    struct glgfx_node* pred;
};

struct glgfx_list {
    struct glgfx_node* head;
    struct glgfx_node* tail;
    struct glgfx_node* tailpred;
};

void glgfx_list_new(struct glgfx_list* list);
bool glgfx_list_isempty(struct glgfx_list* list);
void glgfx_list_addhead(struct glgfx_list* list, struct glgfx_node* node);
void glgfx_list_addtail(struct glgfx_list* list, struct glgfx_node* node);
void glgfx_list_insert(struct glgfx_list* list, 
		       struct glgfx_node* node, 
		       struct glgfx_node* pred);
void glgfx_list_enqueue(struct glgfx_list* list, 
			struct glgfx_node* node, 
			struct glgfx_hook* cmp_hook);
struct glgfx_node* glgfx_list_remove(struct glgfx_node* node);
struct glgfx_node* glgfx_list_remhead(struct glgfx_list* list);
struct glgfx_node* glgfx_list_remtail(struct glgfx_list* list);

struct glgfx_node* glgfx_list_find(struct glgfx_list* list, struct glgfx_node* node);
size_t glgfx_list_length(struct glgfx_list* list);
void glgfx_list_foreach(struct glgfx_list* list, struct glgfx_hook* node_hook, void* message);

#define GLGFX_LIST_FOR(list, node) for (node = (void*) ((struct glgfx_list*) (list))->head;	\
					((struct glgfx_node*) (node))->succ != NULL;		\
					node = (void*) ((struct glgfx_node*) (node))->succ)


struct glgfx_bitmap;
struct glgfx_shader;

#define GLGFX_MAX_RENDER_TARGETS 4

struct glgfx_context {
    struct glgfx_monitor*   monitor;
    GLXContext              glx_context;
    GLXPbuffer              glx_pbuffer;
    GLuint                  fbo;
    struct glgfx_bitmap*    temp_bitmaps[glgfx_pixel_format_max];

    bool                    fbo_bound;
    struct glgfx_bitmap*    fbo_bitmap[GLGFX_MAX_RENDER_TARGETS];
    int                     fbo_width;
    int                     fbo_height;

    struct glgfx_bitmap*    tex_bitmap[2];

    GLuint                  program;
};


struct glgfx_monitor {
    char const*             name;

    struct glgfx_monitor const*   friend;
    bool                    fullscreen;

    int                     mouse_x;
    int                     mouse_y;

    Display*                display;
    GLXFBConfig*            fb_config;
    int                     fb_configs;
    XVisualInfo*            vinfo;
    Window                  window;
    Window                  xparent;
    GLXWindow               glx_window;
    Atom                    xa_win_state;
    Cursor                  cursor;

    struct glgfx_context*   main_context;

    GHashTable*             gl_extensions;
    bool                    have_GLX_SGI_video_sync;
    bool                    have_GL_ARB_pixel_buffer_object;
    bool                    have_GL_EXT_framebuffer_object;
    bool                    have_GL_NV_blend_square;
    bool                    have_GL_ARB_texture_rectangle;
    bool                    is_ati;
    bool                    is_geforce_fx;
    bool                    miss_pixel_ops;
    GLint                   max_mrt;

    timer_t                 vsync_timer;
    struct itimerspec       vsync_itimerspec;
    bool                    vsync_timer_valid;
    pthread_cond_t          vsync_cond;

    struct timeval	    fps_time;
    double		    fps_mean;
    int			    fps_counter;

    enum glgfx_pixel_format format;
    XF86VidModeModeLine     mode;
    int                     dotclock;
    XF86VidModeMonitor      monitor_info;

    struct glgfx_view*      view;
};

struct glgfx_view {
    GQueue* viewports;
    GQueue* sprites;
    bool    has_changed;
};


struct glgfx_cliprect {
  int x;
  int y;
  int width;
  int height;
};

struct glgfx_bitmap {
//    struct glgfx_monitor*   monitor;
    int                     width;
    int                     height;
    int                     bits;
    enum glgfx_pixel_format format;
    GLuint                  texture;
    GLuint                  texture_target;
    GLint                   texture_filter;
    union {
      GLuint                  pbo;
      uint8_t*                buffer;
    };
    size_t                  pbo_size;
    size_t                  pbo_bytes_per_row;
    bool                    locked;
    GLenum                  locked_usage;
    GLenum                  locked_access;
    void*                   locked_memory;
    int                     locked_x;
    int                     locked_y;
    int                     locked_width;
    int                     locked_height;

    GList*                  cliprects;

    bool                    has_changed;
};


struct glgfx_rasinfo {
    struct glgfx_bitmap* bitmap;
    struct glgfx_shader* shader;
    int                  xoffset;
    int                  yoffset;
    int                  width;
    int                  height;
    bool                 interpolated;

    bool                 has_changed;

    enum glgfx_blend_equation blend_eq;
    enum glgfx_blend_equation blend_eq_alpha;
    enum glgfx_blend_func     blend_func_src;
    enum glgfx_blend_func     blend_func_src_alpha;
    enum glgfx_blend_func     blend_func_dst;
    enum glgfx_blend_func     blend_func_dst_alpha;
};


struct glgfx_viewport {
    int     width;
    int     height;
    int     xoffset;
    int     yoffset;
    GQueue* rasinfos;
    bool    has_changed;
};


struct glgfx_viewport_rendermsg {
    struct glgfx_viewport* viewport;
    struct glgfx_hook*     geometry_hook;
    float z;
};

struct glgfx_sprite {
    struct glgfx_bitmap* bitmap;
    int                  x;
    int                  y;
    int                  width;
    int                  height;
    bool                 interpolated;
    bool                 has_changed;
};


struct pixel_info {
    GLint   internal_format;
    GLenum  format;
    GLenum  type;
    size_t  size;
    bool    is_bigendian;
    bool    is_rgb;
    bool    is_float;
    uint8_t redbits;
    uint8_t redshift;
    uint8_t greenbits;
    uint8_t greenshift;
    uint8_t bluebits;
    uint8_t blueshift;
    uint8_t alphabits;
    uint8_t alphashift;
};

extern struct pixel_info const formats[glgfx_pixel_format_max];


extern struct glgfx_shader color_blitter;

extern struct glgfx_shader raw_texture_blitter;
extern struct glgfx_shader plain_texture_blitter;
extern struct glgfx_shader color_texture_blitter;

extern struct glgfx_shader modulated_texture_blitter;

extern struct glgfx_shader stencil_renderer;
extern struct glgfx_shader depth_renderer;
extern struct glgfx_shader blur_renderer;


#define GLGFX_CHECKERROR() glgfx_checkerror(__PRETTY_FUNCTION__, __FILE__, __LINE__);
void glgfx_checkerror(char const* func, char const* file, int line);

bool glgfx_shader_init(struct glgfx_monitor* monitor);
void glgfx_shader_cleanup();
struct glgfx_shader* glgfx_shader_create(int channels, 
					 char const* vertex, 
					 char const* fragment,
					 struct glgfx_monitor* monitor);
void glgfx_shader_destroy(struct glgfx_shader* shader);
GLuint glgfx_shader_load(struct glgfx_bitmap* src_bm0, 
			 struct glgfx_bitmap* src_bm1,
			 enum glgfx_pixel_format dst,
			 struct glgfx_shader* shader,
			 struct glgfx_monitor* monitor);

bool glgfx_monitor_waittof(struct glgfx_monitor* monitor);
bool glgfx_monitor_swapbuffers(struct glgfx_monitor* monitor);

bool glgfx_view_haschanged(struct glgfx_view* view);
bool glgfx_viewport_haschanged(struct glgfx_viewport* viewport);
bool glgfx_sprite_haschanged(struct glgfx_sprite* sprite);
bool glgfx_bitmap_haschanged(struct glgfx_bitmap* bitmap);

bool glgfx_view_render(struct glgfx_view* view, struct glgfx_hook* mode_hook, bool front_to_back);
bool glgfx_view_rendersprites(struct glgfx_view* view);
bool glgfx_viewport_render(struct glgfx_viewport* viewport, 
			   struct glgfx_hook* mode_hook,
			   bool front_to_back);
bool glgfx_sprite_render(struct glgfx_sprite* sprite);

GLenum glgfx_context_bindtex(struct glgfx_context* context, 
			     int channel, struct glgfx_bitmap* bitmap, bool interpolate);
bool glgfx_context_unbindtex(struct glgfx_context* context, int channel);
bool glgfx_context_bindfbo(struct glgfx_context* context, 
			   int bitmaps, struct glgfx_bitmap* const* bitmap);
bool glgfx_context_unbindfbo(struct glgfx_context* context);
bool glgfx_context_bindprogram(struct glgfx_context* context, struct glgfx_shader* shader);
bool glgfx_context_unbindprogram(struct glgfx_context* context);
bool glgfx_context_checkstate(struct glgfx_context* context);
struct glgfx_bitmap* glgfx_context_gettempbitmap(struct glgfx_context* context,
						 int min_width,
						 int min_height,
						 enum glgfx_pixel_format format);

#endif /* arp2_glgfx_glgfx_intern_h */
