
#include "glgfx-config.h"
#include <pthread.h>
#include <stdlib.h>

#include "glgfx.h"
#include "glgfx_bitmap.h"
#include "glgfx_monitor.h"
#include "glgfx_pixel.h"
#include "glgfx_sprite.h"
#include "glgfx_view.h"
#include "glgfx_viewport.h"
#include "glgfx_input.h"

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

bool volatile renderer_quit = false;


void* renderer(void* _m) {
  struct glgfx_monitor* monitor = _m;
  
//  glgfx_context_select(glgfx_monitor_getcontext(monitor));
  glgfx_context_create(glgfx_monitors[0]);
  
  while (!renderer_quit) {
    glgfx_monitor_render(monitor);
  }

  return NULL;
}

bool blit(struct glgfx_bitmap* bitmap, int width, int height) {
  uint16_t* buffer;
  
  struct glgfx_context* ctx = glgfx_context_create(glgfx_monitors[0]);

  // Fill texture with data

  if ((buffer = glgfx_bitmap_lock(bitmap, false, true)) != NULL) {
    int x, y;

    for (y = 0; y < height; y += 1) {
      for (x = 0; x < width; x += 1) {
	buffer[x+y*width] = glgfx_pixel_create_r5g6b5(y*31/height, 0, x*31/width);
      }
    }

    if (glgfx_bitmap_unlock(bitmap, 0, 0, width, height)) {
      printf("updated\n");
    }
  }

  // Clear the PBO buffer
  if ((buffer = glgfx_bitmap_lock(bitmap, true, false)) != NULL) {
    bzero(buffer, width*height*2);
    glgfx_bitmap_unlock(bitmap, 0, 0, width, height);
  }

  // Read texture, modify 100x100 pixels of it and upload 200x200 pixels
  if ((buffer = glgfx_bitmap_lock(bitmap, true, true)) != NULL) {
    int x, y;

    for (y = 0; y < 100; y += 1) {
      for (x = 0; x < 100; x += 1) {
	buffer[x+y*width] = glgfx_pixel_create_r5g6b5((100-y)*31/100, 0, x*31/100);
      }
    }

    if (glgfx_bitmap_unlock(bitmap, 0, 0, 200, 200)) {
      printf("updated 2\n");
    }
  }

  int i;
  for (i = 0; i < 100000; ++i) {
    glgfx_bitmap_blit(bitmap,
		      glgfx_bitmap_blit_x,       0,
		      glgfx_bitmap_blit_y,       0,
		      glgfx_bitmap_blit_width,   100,
		      glgfx_bitmap_blit_height,  100,

		      glgfx_bitmap_blit_dst_x,   100+i/1000*10,
		      glgfx_bitmap_blit_dst_y,   100+i/1000,
		      glgfx_bitmap_blit_minterm, 0x30, // inverted source
		      glgfx_tag_end);
  }

  sleep(3);
  printf("going home\n");
  glgfx_context_destroy(ctx);

  //  while(true);
  return true;
}


int main(int argc, char** argv) {
  int rc = 0;
  (void) argc;
  (void) argv;

  if (!glgfx_init(glgfx_tag_end)) {
    printf("Unable to initialize glgfx\n");
    return 20;
  }

  char const* d = getenv("DISPLAY");
  if (!glgfx_createmonitors(glgfx_create_monitors_tag_display, (intptr_t) d,
			    glgfx_tag_end)) {
    printf("Unable to open display\n");
  }
  else {
    intptr_t width, height;

    if (glgfx_getattrs(glgfx_monitors[0],
		       (glgfx_getattr_proto*) glgfx_monitor_getattr,
		       glgfx_monitor_attr_width,  (intptr_t) &width,
		       glgfx_monitor_attr_height, (intptr_t) &height,
		       glgfx_tag_end) != 2) {
      printf("Unable to get display dimensions\n");
      rc = 20;
    }
    else{
      printf("Display width: %" PRIdPTR "x%" PRIdPTR " pixels\n", width, height);

      struct glgfx_bitmap* bm = 
	glgfx_bitmap_create(glgfx_bitmap_tag_width,  width,
			    glgfx_bitmap_tag_height, height/3, 
			    glgfx_bitmap_tag_format, glgfx_pixel_format_r5g6b5,
//			    glgfx_bitmap_tag_format, glgfx_pixel_format_a8b8g8r8,
			    glgfx_tag_end);
    
      if (bm == NULL) {
	printf("Unable to allocate bitmap\n");
	rc = 20;
      }
      else {
	struct glgfx_viewport* vp1 =
	  glgfx_viewport_create(glgfx_viewport_attr_width,  width,
				glgfx_viewport_attr_height, height / 3,
				glgfx_tag_end);

	struct glgfx_viewport* vp2 =
	  glgfx_viewport_create(glgfx_viewport_attr_y,      height / 3 + 1,
				glgfx_viewport_attr_width,  width,
				glgfx_viewport_attr_height, height * 2 / 3,
				glgfx_tag_end);

	struct glgfx_rasinfo* ri1 =
	  glgfx_viewport_addbitmap(vp1, bm,
				   glgfx_rasinfo_attr_width,  width,
				   glgfx_rasinfo_attr_height, height / 3,
				   glgfx_tag_end);

	struct glgfx_rasinfo* ri2 =
	  glgfx_viewport_addbitmap(vp2, bm,
				   glgfx_rasinfo_attr_width,  width,
				   glgfx_rasinfo_attr_height, height / 3,
				   glgfx_tag_end);
	
	struct glgfx_view* v = glgfx_view_create();

	if (vp1 == NULL || vp2 == NULL || ri1 == NULL || ri2 == NULL ||
	    v == NULL ||
	    !glgfx_view_addviewport(v, vp1) ||
	    !glgfx_view_addviewport(v, vp2) ||
	    !glgfx_monitor_addview(glgfx_monitors[0], v)) {
	  printf("Unable to create view/viewport\n");
	  rc = 20;
	}
	else {
	  pthread_t pid = -1;

	  if (pthread_create(&pid, NULL, renderer, glgfx_monitors[0]) != 0) {
	    printf("Unable to start render thread\n");
	    rc = 20;
	  }
	  else {
	    if (!blit(bm, width, height / 3)) {
	      rc = 5;
	    }
	  }

	  renderer_quit = true;
	  pthread_join(pid, NULL);

	  glgfx_context_select(glgfx_monitor_getcontext(glgfx_monitors[0]));
	}

	glgfx_viewport_destroy(vp1);
	glgfx_viewport_destroy(vp2);
	glgfx_view_destroy(v);
	glgfx_bitmap_destroy(bm);
      }
    }

    glgfx_destroymonitors();
  }
  
  glgfx_cleanup();

  return rc;
}
