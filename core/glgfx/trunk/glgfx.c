
#include <stdlib.h>
#include <errno.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include "glgfx.h"
#include "glgfx_monitor.h"

int                   num_monitors;
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

