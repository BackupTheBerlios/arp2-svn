
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

bool volatile renderer_quit = false;


void* renderer(void* _m) {
  struct glgfx_monitor* monitor = _m;

  while (!renderer_quit) {
    glgfx_monitor_waittof(monitor);
    glgfx_monitor_render(monitor);
    glgfx_monitor_swapbuffers(monitor);
  }

  return NULL;
}

bool blit(struct glgfx_bitmap* bitmap) {
}


int main(int argc, char** argv) {
  int rc = 0;
  (void) argc;
  (void) argv;

  if (!glgfx_init(glgfx_tag_end)) {
    printf("Unable to initialize glgfx\n");
    return 10;
  }

  char const* d = getenv("DISPLAY");
  if (!glgfx_createmonitors(glgfx_create_monitors_tag_display, (intptr_t) d,
			    glgfx_tag_end)) {
    printf("Unable to open display\n");
  }
  else {
    intptr_t width, height;

    if (glgfx_getattrs(glgfx_monitors[0], /*glgfx_getattr_proto */
		       (glgfx_getattr_proto*) glgfx_monitor_getattr,
		       glgfx_monitor_attr_width,  (intptr_t) &width,
		       glgfx_monitor_attr_height, (intptr_t) &height,
		       glgfx_tag_end) != 2) {
      printf("Unable to get display dimensions\n");
      rc = 10;
    }
    else {
      printf("Display width: %dx%d pixels\n", width, height);

      struct glgfx_bitmap* bm = 
	glgfx_bitmap_create(glgfx_bitmap_tag_width,  width,
			    glgfx_bitmap_tag_height, height / 3, 
			    glgfx_bitmap_tag_format, glgfx_pixel_format_r5g6b5,
			    glgfx_tag_end);
    
      if (bm == NULL) {
	printf("Unable to allocate bitmap\n");
	rc = 10;
      }
      else {
	struct glgfx_viewport* vp1 = 
	  glgfx_viewport_create(glgfx_viewport_attr_width,  width,
				glgfx_viewport_attr_height, height / 3,
				glgfx_tag_end);

	struct glgfx_viewport* vp2 = 
	  glgfx_viewport_create(glgfx_viewport_attr_width,  width,
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
				   glgfx_rasinfo_attr_height, height * 2 / 3,
				   glgfx_rasinfo_attr_y,      height / 3,
				   glgfx_tag_end);
	
	struct glgfx_view* v = glgfx_view_create();

	if (vp1 == NULL || vp2 == NULL || ri1 == NULL || ri2 == NULL || 
	    v == NULL || 
	    !glgfx_view_addviewport(v, vp1) || 
	    !glgfx_view_addviewport(v, vp2)) {
	  printf("Unable to create view/viewport\n");
	  rc = 10;
	}
	else {
	  pthread_t pid = -1;

	  if (pthread_create(&pid, NULL, renderer, glgfx_monitors[0]) != 0) {
	    printf("Unable to start render thread\n");
	    rc = 10;
	  }
	  else {
	    if (!blit(bm)) {
	      rc = 5;
	    }

	    renderer_quit = true;
	    pthread_join(pid, NULL);
	  }
	}

	glgfx_viewport_rembitmap(vp1, ri1);
	glgfx_viewport_rembitmap(vp2, ri2);
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
