
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include "glgfx.h"
#include "glgfx_bitmap.h"
#include "glgfx_monitor.h"
#include "glgfx_viewport.h"

#define max_monitors  8               // Four cards, two outputs/card max
static int            num_monitors;
struct glgfx_monitor* monitors[max_monitors];

bool glgfx_create_monitors(void) {
  char name[8];
  int display;
  int screen;
  struct glgfx_monitor* friend = NULL;
  
  glgfx_destroy_monitors();

  num_monitors = 0;
  
  for (display = 0; num_monitors < max_monitors; ++display) {
    for (screen = 0; num_monitors < max_monitors; ++screen) {
      sprintf(name, ":%d.%d", display, screen);

      monitors[num_monitors] = glgfx_monitor_create(name, friend);

      if (monitors[num_monitors] == NULL) {
	if (errno == ENXIO) {
	  break;
	}
	else {
	  continue;
	}
      }

      friend = monitors[num_monitors];
      
      ++num_monitors;
    }

    if (screen == 0) {
      // Failed to open display ":x.0"
      break;
    }
  }

  return num_monitors != 0;
}


void glgfx_destroy_monitors(void) {
  int i;

  for (i = 0; i < num_monitors; ++i) {
    glgfx_monitor_destroy(monitors[i]);
    monitors[i] = NULL;
  }

  num_monitors = 0;
}


bool glgfx_waitblit(void) {
  bool rc = true;
  int i;

  for (i = 0; i < num_monitors; ++i) {
    if (!glgfx_monitor_waitblit(monitors[i])) {
      rc = false;
    }
  }

  return rc;
}

bool glgfx_waittof(void) {
  bool rc = true;
  int i;

  for (i = 0; i < num_monitors; ++i) {
    if (!glgfx_monitor_waittof(monitors[i])) {
      rc = false;
    }
  }

  return rc;
}

void glgfx_check_error(char const* func, char const* file, int line) {
  GLenum error = glGetError();

  if (error != 0) {
    char const* msg = gluErrorString(error);
    
    BUG("OpenGL error %d %s:%d (%s): %s\n", error, file, line, func, msg);
    abort();
  }
}

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
    int width = 1280;
    int height = 1024;
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
      struct glgfx_rasinfo*  ri = glgfx_viewport_addbitmap(vp, bm, 0, 0);
      
      int i;
      for (i = 0; i < 255; i+=2) {
	if (glgfx_bitmap_lock(bm, false, true)) {
	  int* data = (int*) bm->locked_memory;
	  int x, y;
      
	  for (y = 0; y < height; y+=10) {
	    for (x = 0; x < width; x+=10) {
//	      data[x+y*width+i] = -1;
	    }
	  }
	  glgfx_bitmap_unlock(bm);
	}

	glgfx_viewport_move(vp, 320 + i, 256, 100, 200+i);
//	glgfx_viewport_setbitmap(vp, ri, bm, i*5, i*4);
	
	glDrawBuffer(GL_BACK);
//	glClearColor( i/255.0, 0, 0, 1.0);
	glClearColor( 0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glgfx_viewport_render(vp);
	glgfx_monitor_waittof(monitors[0]);
      }

      glgfx_viewport_destroy(vp);
    }
    glgfx_bitmap_destroy(bm);
      
    glgfx_destroy_monitors();
  }
  
  return 0;
}

