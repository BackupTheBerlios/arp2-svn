
#include "glgfx-config.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "glgfx.h"
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
#include <errno.h>
#include <unistd.h>

struct glgfx_bitmap* bitmap;
intptr_t width, height;

bool volatile renderer_quit = false;

// A non-broken sleep()
unsigned int sleep(unsigned int seconds) {
  struct timespec ts = { seconds, 0 };

  while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
  return 0;
}


void* renderer(struct glgfx_monitor* monitor) {
  printf("Rendering on thread %ld\n", pthread_self());

  while (!renderer_quit) {
    glgfx_monitor_render(monitor);
  }

  return NULL;
}

void* blit(struct glgfx_monitor* monitor) {
  int w = width, h = height / 3;
  
//  uint16_t* buffer;
  uint32_t* buffer;
  
  printf("Blitting on thread %ld\n", (void*) pthread_self());

#if 1
  // Fill texture with data

  if ((buffer = glgfx_bitmap_lock(bitmap, false, true, glgfx_tag_end)) != NULL) {
    int x, y;

    for (y = 0; y < h; y += 1) {
      for (x = 0; x < w; x += 1) {
//	buffer[x+y*w] = glgfx_pixel_create_r5g6b5(y*31/h, 0, x*31/w);
	buffer[x+y*w] = glgfx_pixel_create_a8r8g8b8(y*255/h, 0, x*255/w, 0);
      }
    }

    if (glgfx_bitmap_unlock(bitmap, glgfx_tag_end)) {
      printf("updated\n");
    }
  }

  // Clear the PBO buffer, but don't update bitmap
  if ((buffer = glgfx_bitmap_lock(bitmap, true, false, glgfx_tag_end)) != NULL) {
    memset(buffer, 0, w*h*sizeof(*buffer));
    glgfx_bitmap_unlock(bitmap, glgfx_tag_end);
  }

  // Read texture (but only 180x180!), modify 100x100 pixels of it and
  // upload 200x200 pixels. The 20 pixel wide border that was not
  // read will have undefined content (but probably zero).
  if ((buffer = glgfx_bitmap_lock(bitmap, true, true,
				  glgfx_bitmap_copy_width, 180,
				  glgfx_bitmap_copy_height, 180,
				  glgfx_tag_end)) != NULL) {
    int x, y;
    
    for (y = 0; y < 100; y += 1) {
      for (x = 0; x < 100; x += 1) {
	buffer[x+y*w] = glgfx_pixel_create_a8r8g8b8(y*255/100, 0, x*255/100, 0);
      }
    }

    if (glgfx_bitmap_unlock(bitmap, 
			    glgfx_bitmap_copy_width, 200,
			    glgfx_bitmap_copy_height, 200,
			    glgfx_tag_end)) {
      printf("updated 2\n");
    }
  }

  int i;
  for (i = 0; i < 100; ++i) {
    // Blit upper left 100x100 pixels in a stripe down-right
    glgfx_bitmap_blit(bitmap,
		      glgfx_bitmap_blit_x,       100+i*10,
		      glgfx_bitmap_blit_y,       100+i,
		      glgfx_bitmap_blit_width,   100,
		      glgfx_bitmap_blit_height,  100,

		      glgfx_bitmap_blit_src_x,   0,
		      glgfx_bitmap_blit_src_y,   0,
		      glgfx_bitmap_blit_minterm, 0x30, // inverted source
		      glgfx_tag_end);
  }

  // Scaled blit test, src == dst: blit center 100x100 pixels to upper
  // right corner, 200x200 pixels
  glgfx_bitmap_blit(bitmap,
		    glgfx_bitmap_blit_x,          w-200,
		    glgfx_bitmap_blit_y,          0,
		    glgfx_bitmap_blit_width,      200,
		    glgfx_bitmap_blit_height,     200,

		    glgfx_bitmap_blit_src_x,      w/2-50,
		    glgfx_bitmap_blit_src_y,      h/2-50,
		    glgfx_bitmap_blit_src_width,  100,
		    glgfx_bitmap_blit_src_height, 100,
		    glgfx_tag_end);


  struct glgfx_bitmap* bm2 = 
    glgfx_bitmap_create(glgfx_bitmap_attr_width,  100,
			glgfx_bitmap_attr_height, 100, 
			glgfx_bitmap_attr_format, glgfx_pixel_format_r5g6b5,
			glgfx_tag_end);

  if (bm2 != NULL) {
    uint16_t* buffer;

    if ((buffer = glgfx_bitmap_lock(bm2, false, true, glgfx_tag_end)) != NULL) {
      int x, y;

      for (y = 0; y < 100; y += 1) {
	for (x = 0; x < 100; x += 1) {
	  buffer[x+y*100] = glgfx_pixel_create_r5g6b5(y*31/100, x*63/100, 0);
	}
      }

      if (glgfx_bitmap_unlock(bm2, glgfx_tag_end)) {
	printf("updated bm2\n");
      }
    }

    // Scaled blit test, src != dst: blit bm2 (inverted) to middle
    // left bottom, 50x50 pixels. Also reduce the source's red
    // component to 50% (before inverting)
    glgfx_bitmap_blit(bitmap,
		      glgfx_bitmap_blit_x,          200,
		      glgfx_bitmap_blit_y,          h-50,
		      glgfx_bitmap_blit_width,      50,
		      glgfx_bitmap_blit_height,     50,

		      glgfx_bitmap_blit_src_x,      0,
		      glgfx_bitmap_blit_src_y,      0,
		      glgfx_bitmap_blit_src_width,  100,
		      glgfx_bitmap_blit_src_height, 100,
		      glgfx_bitmap_blit_src_bitmap, (intptr_t) bm2,

		      glgfx_bitmap_blit_mod_r,      0x7fffffff,
		      
		      glgfx_bitmap_blit_minterm,    0x30, // inverted source


		      glgfx_tag_end);

    glgfx_bitmap_destroy(bm2);
  }
#endif  

  sleep(3);

  printf("going home\n");

  return NULL;
}


void* thread(void* _m) {
  struct glgfx_monitor* monitor = _m;
  
  struct glgfx_context* ctx = glgfx_context_create(monitor);

  void* res = blit(monitor);

  glgfx_context_destroy(ctx);

  renderer_quit = true;
  return res;
}


int main(int argc, char** argv) {
  int rc = 0;
  (void) argc;
  (void) argv;

  if (!glgfx_init(glgfx_tag_end)) {
    printf("Unable to initialize glgfx\n");
    return 20;
  }
  
  struct glgfx_monitor* monitor = 
    glgfx_monitor_create(NULL,
			 glgfx_monitor_attr_fullscreen, true,
			 glgfx_tag_end);

  if (monitor == NULL) {
    printf("Unable to open display\n");
  }
  else {
    if (glgfx_getattrs(monitor,
		       (glgfx_getattr_proto*) glgfx_monitor_getattr,
		       glgfx_monitor_attr_width,  (intptr_t) &width,
		       glgfx_monitor_attr_height, (intptr_t) &height,
		       glgfx_tag_end) != 2) {
      printf("Unable to get display dimensions\n");
      rc = 20;
    }
    else{
      printf("Display width: %" PRIdPTR "x%" PRIdPTR " pixels\n", width, height);

      bitmap = glgfx_bitmap_create(glgfx_bitmap_attr_width,  width,
				   glgfx_bitmap_attr_height, height/3, 
				   //glgfx_bitmap_attr_format, glgfx_pixel_format_r5g6b5,
				   glgfx_bitmap_attr_format, glgfx_pixel_format_a8b8g8r8,
				   glgfx_tag_end);
    
      if (bitmap == NULL) {
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
	  glgfx_viewport_addbitmap(vp1, bitmap,
				   glgfx_rasinfo_attr_width,  width,
				   glgfx_rasinfo_attr_height, height / 3,
				   glgfx_tag_end);

	struct glgfx_rasinfo* ri2 =
	  glgfx_viewport_addbitmap(vp2, bitmap,
				   glgfx_rasinfo_attr_width,  width,
				   glgfx_rasinfo_attr_height, height / 3,
				   glgfx_tag_end);
	
	struct glgfx_view* v = glgfx_view_create();

	if (vp1 == NULL || vp2 == NULL || ri1 == NULL || ri2 == NULL ||
	    v == NULL ||
	    !glgfx_view_addviewport(v, vp1) ||
	    !glgfx_view_addviewport(v, vp2) ||
	    !glgfx_monitor_setattrs(monitor, glgfx_monitor_attr_view, v, glgfx_tag_end)) {
	  printf("Unable to create view/viewport\n");
	  rc = 20;
	}
	else {
	  pthread_t pid = -1;

	  printf("Main thread is %ld\n", pthread_self());

	  if (pthread_create(&pid, NULL, thread, monitor) != 0) {
	    printf("Unable to start blit thread\n");
	    rc = 20;
	  }
	  else {
	    renderer(monitor);
	  }

	  pthread_join(pid, NULL);

	  printf("blit thread joined\n");
	}

	glgfx_viewport_destroy(vp1);
	glgfx_viewport_destroy(vp2);
	glgfx_view_destroy(v);
	glgfx_bitmap_destroy(bitmap);
      }
    }

    glgfx_monitor_destroy(monitor);
  }
  
  glgfx_cleanup();

  return rc;
}
