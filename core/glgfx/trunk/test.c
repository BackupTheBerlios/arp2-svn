
#include "glgfx-config.h"
#include <stdlib.h>

#include "glgfx_bitmap.h"
#include "glgfx_monitor.h"
#include "glgfx_view.h"
#include "glgfx_viewport.h"
#include "glgfx_input.h"

#define UPLOAD_MODE 2

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
  // If unset, sync to vblank as default (nvidia driver)
  setenv("__GL_SYNC_TO_VBLANK", "0", False);
  setenv("__GL_NV30_EMULATE", "0", False);

/* Section "Device" */
/*     Identifier "NV AGP" */
/*     Driver     "nvidia" */
/*     VendorName "nvidia" */
/*     BusID      "PCI:2:0:0" */
/*     Option     "NvEmulate" "30" */
/* EndSection */

  if (glgfx_create_monitors()) {
    int width = 1024;
    int height = 768;
    unsigned short* data;

#if UPLOAD_MODE < 2
    data = calloc(sizeof (*data), width * height);
#endif
    
    struct glgfx_bitmap* bm = glgfx_bitmap_create(width, height, 24, 0, NULL,
						  glgfx_pixel_a4r4g4b4, glgfx_monitors[0]);

    if (bm != NULL) {
#if UPLOAD_MODE > 0
      if (glgfx_bitmap_lock(bm, false, true)) {
#endif
	int x, y;
      
#if UPLOAD_MODE > 1
	data = glgfx_bitmap_map(bm);
#endif
	for (y = 0; y < height; y+=1) {
	  for (x = 0; x < width; x+=1) {
//	    data[x+y*width] = (255 << 24) | ((255*x/width) << 16) | ((255*y/height) << 8) | 0;
	    data[x+y*width] = 0x8ff8;
	  }
	}
#if UPLOAD_MODE < 2
	  glgfx_bitmap_update(bm, data, width*height*sizeof (*data));
#endif
#if UPLOAD_MODE > 0
	glgfx_bitmap_unlock(bm);
      }
#endif

      glgfx_monitor_select(glgfx_monitors[0]);

      struct glgfx_viewport* vp = glgfx_viewport_create(320, 256, 100, 200);
      struct glgfx_rasinfo*  ri = glgfx_viewport_addbitmap(vp, bm, 0, 0, 320, 256);
      struct glgfx_view*     v  = glgfx_view_create(glgfx_monitors[0]);

      glgfx_view_addviewport(v, vp);

//      glgfx_input_acquire();
      int i;
      struct timeval s, e;

      gettimeofday(&s, NULL);
      for (i = 0; i < 256; i+=1) {
#if UPLOAD_MODE > 0
	if (glgfx_bitmap_lock(bm, false, true)) {
#endif
	  int x, y;

#if UPLOAD_MODE > 1
	  data = glgfx_bitmap_map(bm);
#endif
	  for (y = 0; y < height; y+=10) {
	    for (x = 0; x < width; x+=10) {
	      data[x+y*width] = -1;
	    }
	  }
#if UPLOAD_MODE < 2
	  glgfx_bitmap_update(bm, data, width*height*sizeof (*data));
#endif
#if UPLOAD_MODE > 0
	  glgfx_bitmap_unlock(bm);
	}
#endif

	glgfx_viewport_move(vp, 320 + i, 256, 100, 200+i);
//	glgfx_viewport_setbitmap(vp, ri, bm, 0, 0, 320, 256);
	
	glgfx_view_render(v);
	glgfx_monitor_waittof(glgfx_monitors[0]);

/* 	enum glgfx_input_code code; */
/* 	while ((code = glgfx_input_getcode()) != glgfx_input_none) { */
/* 	  printf("%08lx\n", code); */
/* 	} */
      }
      gettimeofday(&e, NULL);
      double sec = (e.tv_sec + e.tv_usec * 1e-6) - (s.tv_sec + s.tv_usec * 1e-6);
      
      printf("uploaded %d images in %g seconds -> %g fps/%d MB/s\n",
	     i, sec, i / sec, (int) (i * width * height * sizeof (*data) / sec / 1e6));
//      glgfx_input_release();

      glgfx_viewport_destroy(vp);
    }
    glgfx_bitmap_destroy(bm);
      
    glgfx_destroy_monitors();
  }
  
  return 0;
}

