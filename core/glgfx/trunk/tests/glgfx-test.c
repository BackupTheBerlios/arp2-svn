
#include "glgfx-config.h"
#include <stdlib.h>

#include "glgfx_bitmap.h"
#include "glgfx_monitor.h"
#include "glgfx_pixel.h"
#include "glgfx_view.h"
#include "glgfx_viewport.h"
#include "glgfx_input.h"

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

  char const* d = getenv("DISPLAY");
  if (glgfx_createmonitors(glgfx_create_monitors_tag_display, (intptr_t) d,
			   glgfx_tag_end)) {
    int width = 512;
    int height = 512;
    PIXEL_TYPE* data;

#if UPLOAD_MODE == 0
    data = calloc(sizeof (*data), width * height);
#endif
    
    struct glgfx_bitmap* bm = glgfx_bitmap_create(glgfx_bitmap_tag_width,  width,
						  glgfx_bitmap_tag_height, height, 
						  glgfx_bitmap_tag_bits,   24,
						  glgfx_bitmap_tag_friend, NULL,
						  glgfx_bitmap_tag_format, PIXEL_FORMAT,
						  glgfx_tag_end);
    
    if (bm != NULL) {
#if UPLOAD_MODE == 1
      if (glgfx_bitmap_lock(bm, false, true)) {
#endif
	int x, y;
      
#if UPLOAD_MODE == 1
	glgfx_bitmap_getattr(bm, glgfx_bitmap_attr_mapaddr, (intptr_t*) &data);
#endif
	for (y = 0; y < height; y+=1) {
	  for (x = 0; x < width; x+=1) {
	    data[x+y*width] = (255 << 24) | ((255*x/width) << 16) | ((255*y/height) << 8) | 0;
//	    data[x+y*width] = 0x8ff8;
	  }
	}
#if UPLOAD_MODE == 0
	glgfx_bitmap_update(bm, 
			    glgfx_bitmap_tag_width,       width,
			    glgfx_bitmap_tag_height,      height,
			    glgfx_bitmap_tag_data,        data, 
			    glgfx_bitmap_tag_format,      PIXEL_FORMAT, 
			    glgfx_tag_end);
#endif
#if UPLOAD_MODE == 1
	glgfx_bitmap_unlock(bm, 0, 0, width, height);
      }
#endif

      struct glgfx_viewport* vp = glgfx_viewport_create(glgfx_viewport_tag_width,   320,
							glgfx_viewport_tag_height,  256,
							glgfx_viewport_tag_xoffset, 100,
							glgfx_viewport_tag_yoffset, 200,
							glgfx_tag_end);
      struct glgfx_rasinfo*  ri = glgfx_viewport_addbitmap(vp, bm, 
							   glgfx_viewport_tag_width,   320,
							   glgfx_viewport_tag_height,  256,
							   glgfx_viewport_tag_xoffset, 0,
							   glgfx_viewport_tag_yoffset, 0,
							   glgfx_tag_end);
      struct glgfx_view*     v  = glgfx_view_create(glgfx_monitors[0]);

      glgfx_view_addviewport(v, vp);

      glgfx_input_acquire(false);
      int i;
      struct timeval s, e;

      gettimeofday(&s, NULL);
      for (i = 0; i < 256; i+=1) {
#if UPLOAD_MODE == 1
	if (glgfx_bitmap_lock(bm, false, true)) {
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
	  glgfx_bitmap_update(bm,
			      glgfx_bitmap_tag_width,       width,
			      glgfx_bitmap_tag_height,      height,
			      glgfx_bitmap_tag_data,        data,
			      glgfx_bitmap_tag_format,      PIXEL_FORMAT,
			      glgfx_tag_end);
#endif
#if UPLOAD_MODE == 1
	  glgfx_bitmap_unlock(bm, 0, 0, width, height);
	}
#endif

	glgfx_viewport_move(vp, 
			    glgfx_viewport_tag_width,   100+i*3,
			    glgfx_viewport_tag_yoffset, i*4-100,
			    glgfx_tag_end);
	
	glgfx_monitor_waittof(glgfx_monitors[0]);
	glgfx_view_render(v);
	glgfx_monitor_swapbuffers(glgfx_monitors[0]);

/* 	enum glgfx_input_code code; */
/* 	while ((code = glgfx_input_getcode()) != glgfx_input_none) { */
/* 	  printf("%08lx\n", code); */
/* 	} */
      }
      gettimeofday(&e, NULL);
      double sec = (e.tv_sec + e.tv_usec * 1e-6) - (s.tv_sec + s.tv_usec * 1e-6);
      
      printf("uploaded %d images in %g seconds -> %g fps/%d MB/s\n",
	     i, sec, i / sec, (int) (i * width * height * sizeof (*data) / sec / 1e6));
      glgfx_input_release();
	   
      glgfx_viewport_destroy(vp);
    }
    glgfx_bitmap_destroy(bm);
      
    glgfx_destroymonitors();
  }
  
  glgfx_cleanup();

  return 0;
}

