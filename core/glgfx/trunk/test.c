
#include <stdlib.h>

#include "glgfx_bitmap.h"
#include "glgfx_monitor.h"
#include "glgfx_view.h"
#include "glgfx_viewport.h"

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
  // If unset, sync to vblank as default (nvidia driver)
  setenv("__GL_SYNC_TO_VBLANK", "1", False);
  setenv("__GL_NV30_EMULATE", "1", False);

/* Section "Device" */
/*     Identifier "NV AGP" */
/*     Driver     "nvidia" */
/*     VendorName "nvidia" */
/*     BusID      "PCI:2:0:0" */
/*     Option     "NvEmulate" "30" */
/* EndSection */

  if (glgfx_create_monitors()) {
    int width = 640;
    int height = 480;
    struct glgfx_bitmap* bm = glgfx_bitmap_create(width, height, 24, 0, NULL,
						  glgfx_pixel_b8g8r8a8, monitors[0]);

    if (bm != NULL) {
	if (glgfx_bitmap_lock(bm, false, true)) {
	  int* data = (int*) bm->locked_memory;
	  int x, y;

	  for (y = 0; y < height; y+=1) {
	    for (x = 0; x < width; x+=1) {
	      data[x+y*width] = (255 << 24) | ((255*x/width) << 16) | ((255*y/height) << 8) | 0;
	    }
	  }
	  glgfx_bitmap_unlock(bm);
	}

      glgfx_monitor_select(monitors[0]);

      struct glgfx_viewport* vp = glgfx_viewport_create(320, 256, 100, 200);
      struct glgfx_rasinfo*  ri = glgfx_viewport_addbitmap(vp, bm, 0, 0, 320, 256);
      struct glgfx_view*     v  = glgfx_view_create(monitors[0]);

      glgfx_view_addviewport(v, vp);
      
      int i;
      for (i = 0; i < 255; i+=1) {
/* 	if (glgfx_bitmap_lock(bm, false, true)) { */
/* 	  int* data = (int*) bm->locked_memory; */
/* 	  int x, y; */
      
/* 	  for (y = 0; y < height; y+=10) { */
/* 	    for (x = 0; x < width; x+=10) { */
/* 	      data[x+y*width+i*13] = -1; */
/* 	    } */
/* 	  } */
/* 	  glgfx_bitmap_unlock(bm); */
/* 	} */

	glgfx_viewport_move(vp, 320 + i, 256, 100, 200+i);
//	glgfx_viewport_setbitmap(vp, ri, bm, 0, 0, 320, 256);
	
	glgfx_view_render(v);
	glgfx_monitor_waittof(monitors[0]);
      }

      glgfx_viewport_destroy(vp);
    }
    glgfx_bitmap_destroy(bm);
      
    glgfx_destroy_monitors();
  }
  
  return 0;
}

