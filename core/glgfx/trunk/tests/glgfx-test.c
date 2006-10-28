
#include "glgfx-config.h"
#include <stdlib.h>

#include "glgfx_bitmap.h"
#include "glgfx_context.h"
#include "glgfx_input.h"
#include "glgfx_monitor.h"
#include "glgfx_pixel.h"
#include "glgfx_sprite.h"
#include "glgfx_view.h"
#include "glgfx_viewport.h"

#include <stdio.h>
#include <sys/time.h>

#define UPLOAD_MODE 1
#define DATA_TYPE 1

#if DATA_TYPE == 0
# define PIXEL_FORMAT glgfx_pixel_format_r5g6b5
# define PIXEL_TYPE   unsigned short
#elif DATA_TYPE == 1
# define PIXEL_FORMAT glgfx_pixel_format_a8b8g8r8
# define PIXEL_TYPE   unsigned int
#endif

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
  // If unset, sync to vblank as default (nvidia driver)
/*   setenv("__GL_SYNC_TO_VBLANK", "1", 0); */
/*   setenv("__GL_NV30_EMULATE", "1", 0); */

  if (!glgfx_init(glgfx_tag_end)) {
    return 10;
  }

  struct glgfx_monitor* monitor = glgfx_monitor_create(getenv("DISPLAY"),
						       glgfx_tag_end);

  if (monitor != NULL) {
    int width = 512;
    int height = 512;
    PIXEL_TYPE* data;

    int mouse_x = 100;
    int mouse_y = 100;

    glgfx_context_select(glgfx_monitor_getcontext(monitor));
    
#if UPLOAD_MODE == 0
    data = calloc(sizeof (*data), width * height);
#endif
    
    struct glgfx_bitmap* bm = glgfx_bitmap_create(glgfx_bitmap_attr_width,  width,
						  glgfx_bitmap_attr_height, height, 
						  glgfx_bitmap_attr_bits,   24,
						  glgfx_bitmap_attr_friend, 0,
						  glgfx_bitmap_attr_format, PIXEL_FORMAT,
						  glgfx_tag_end);
    
    if (bm != NULL) {
#if UPLOAD_MODE == 1
      if (glgfx_bitmap_lock(bm, false, true, glgfx_tag_end)) {
#endif
	int x, y;
      
#if UPLOAD_MODE == 1
	glgfx_bitmap_getattr(bm, glgfx_bitmap_attr_mapaddr, (intptr_t*) &data);
#endif
	for (y = 0; y < height; y+=1) {
	  for (x = 0; x < width; x+=1) {
	    data[x+y*width] = (128 << 24) | ((255*x/width) << 16) | ((255*y/height) << 8) | 0;
//	    data[x+y*width] = 0x8ff8;
	  }
	}
#if UPLOAD_MODE == 0
	glgfx_bitmap_write(bm, 
			   glgfx_bitmap_copy_width,       width,
			   glgfx_bitmap_copy_height,      height,
			   glgfx_bitmap_copy_data,        (intptr_t) data, 
			   glgfx_bitmap_copy_format,      PIXEL_FORMAT, 
			   glgfx_tag_end);
#endif
#if UPLOAD_MODE == 1
	glgfx_bitmap_unlock(bm, glgfx_tag_end);
      }
#endif

      struct glgfx_viewport* vp = glgfx_viewport_create(glgfx_viewport_attr_width, 320,
							glgfx_viewport_attr_height,256,
							glgfx_viewport_attr_x,     100,
							glgfx_viewport_attr_y,     200,
							glgfx_tag_end);
      struct glgfx_rasinfo*  ri = glgfx_viewport_addbitmap(vp, bm, 
							   glgfx_rasinfo_attr_width, 320,
							   glgfx_rasinfo_attr_height,256,
							   glgfx_rasinfo_attr_x,     0,
							   glgfx_rasinfo_attr_y,     0,
							   glgfx_tag_end);
      struct glgfx_view*     v  = glgfx_view_create();

      glgfx_view_addviewport(v, vp);

      struct glgfx_sprite* sp = glgfx_sprite_create(
	glgfx_sprite_attr_width,  32,
	glgfx_sprite_attr_height, 32,
	glgfx_sprite_attr_bitmap, (intptr_t) bm,
	glgfx_tag_end);

      if (sp != NULL) {
	glgfx_view_addsprite(v, sp);
      }

      glgfx_monitor_setattrs(monitor, glgfx_monitor_attr_view, (intptr_t) v, glgfx_tag_end);

      glgfx_input_acquire(monitor);
      int i;
      struct timeval s, e;

      gettimeofday(&s, NULL);
      for (i = 0; i < 256*4; i+=1) {
#if UPLOAD_MODE == 1
	if (glgfx_bitmap_lock(bm, false, true, glgfx_tag_end)) {
#endif
	  int x, y;

#if UPLOAD_MODE == 1
	  glgfx_bitmap_getattr(bm, glgfx_bitmap_attr_mapaddr, (intptr_t*) &data);
#endif
	  for (y = 0; y < height; y+=10) {
	    for (x = 0; x < width; x+=10) {
	      data[x+y*width] = 0;
	    }
	  }
#if UPLOAD_MODE == 0
	  glgfx_bitmap_write(bm,
			     glgfx_bitmap_copy_width,       width,
			     glgfx_bitmap_copy_height,      height,
			     glgfx_bitmap_copy_data,        (intptr_t) data,
			     glgfx_bitmap_copy_format,      PIXEL_FORMAT,
			     glgfx_tag_end);
#endif
#if UPLOAD_MODE == 1
	  glgfx_bitmap_unlock(bm, glgfx_tag_end);
	}
#endif

	glgfx_viewport_setattrs(vp,
				glgfx_viewport_attr_width, 100+i,
				glgfx_viewport_attr_y,     i-100,
				glgfx_tag_end);
	
	glgfx_sprite_setattrs(sp, 
			      glgfx_sprite_attr_x, mouse_x,
			      glgfx_sprite_attr_y, mouse_y,
			      glgfx_tag_end);

	//	glgfx_context_unbindfbo(glgfx_context_getcurrent());


	struct glgfx_input_event event;
	while ((glgfx_input_getcode(&event))) {
	  if (event.class == glgfx_input_class_mouse) {
	    if (event.code == glgfx_input_x) {
	      mouse_x += event.value;
	    }
	    else if (event.code == glgfx_input_y) {
	      mouse_y += event.value;
	    }
	    else if (event.code == glgfx_input_btn_left) {
	      i += 1000;
	    }
	  }
	}

	glgfx_monitor_render(monitor);
      }
      gettimeofday(&e, NULL);
      double sec = (e.tv_sec + e.tv_usec * 1e-6) - (s.tv_sec + s.tv_usec * 1e-6);
      
      printf("uploaded %d images in %g seconds -> %g fps/%d MB/s\n",
	     i, sec, i / sec, (int) (i * width * height * sizeof (*data) / sec / 1e6));
      glgfx_input_release(monitor);

      glgfx_sprite_destroy(sp);
      glgfx_viewport_destroy(vp);
    }
    glgfx_bitmap_destroy(bm);
      
    glgfx_monitor_destroy(monitor);
  }
  
  glgfx_cleanup();

  return 0;
}

