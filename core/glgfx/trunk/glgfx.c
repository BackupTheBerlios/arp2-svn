
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "glgfx.h"
#include "glgfx_monitor.h"
#include "glgfx_bitmap.h"

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

/*       int i; */
/*       for (i = 0; i < 160; ++i) { */
/* 	glgfx_monitor_select(monitors[num_monitors]); */
/* 	glDrawBuffer(GL_FRONT); */
/* 	glClearColor( i/160.0, 0, 0, 1); */
/* 	glClear(GL_COLOR_BUFFER_BIT); */
/* 	glFlush(); */
/* 	glgfx_monitor_waittof(monitors[num_monitors]); */
/*       } */
      
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
void glDeleteFramebuffersEXT(GLsizei n, GLuint *framebuffers);
void glGenFramebuffersEXT(GLsizei n, GLuint *ids);

int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
  // If unset, sync to vblank as default (nvidia driver)
  setenv("__GL_SYNC_TO_VBLANK", "1", False);
/*   if (glopen()) { */
    if (glgfx_create_monitors()) {
      struct glgfx_bitmap* bm = glgfx_bitmap_create(100, 200, 24, 0, NULL,
						    glgfx_pixel_r8g8b8a8, monitors[0]);

      glgfx_bitmap_destroy(bm);
      
      glgfx_destroy_monitors();
    }
/*   } */
  
  return 0;
}
