
#define _GNU_SOURCE  // for PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#include "glgfx-config.h"
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include "glgfx.h"
#include "glgfx_monitor.h"
#include "glgfx_intern.h"

pthread_mutex_t glgfx_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

int                   glgfx_num_monitors;
struct glgfx_monitor* glgfx_monitors[max_monitors];

struct glgfx_tagitem* glgfx_nexttagitem(struct glgfx_tagitem** taglist_ptr) {
  if (taglist_ptr == NULL || *taglist_ptr == NULL) {
    return NULL;
  }

  while (true) {
    switch ((*taglist_ptr)->tag) {
      case glgfx_tag_done:
	*taglist_ptr = NULL;
	return NULL;

      case glgfx_tag_more:
	*taglist_ptr = (struct glgfx_tagitem*) (*taglist_ptr)->data;
	break;

      case glgfx_tag_ignore:
	++(*taglist_ptr);
	break;

      case glgfx_tag_skip:
	*taglist_ptr += (*taglist_ptr)->data + 1;
	break;

      default: {
	struct glgfx_tagitem* res = *taglist_ptr;

	++(*taglist_ptr);
	return res;
      }
    }
  }
}

bool glgfx_create_monitors(void) {
  bool rc;
  char name[8];
  int display;
  int screen;
  struct glgfx_monitor* friend = NULL;

  pthread_mutex_lock(&glgfx_mutex);
  
  glgfx_destroy_monitors();

  glgfx_num_monitors = 0;
  
  for (display = 0; glgfx_num_monitors < max_monitors; ++display) {
    for (screen = 0; glgfx_num_monitors < max_monitors; ++screen) {
      sprintf(name, ":%d.%d", display, screen);

      glgfx_monitors[glgfx_num_monitors] = glgfx_monitor_create(name, friend);

      if (glgfx_monitors[glgfx_num_monitors] == NULL) {
	if (errno == ENXIO) {
	  break;
	}
	else {
	  continue;
	}
      }

      friend = glgfx_monitors[glgfx_num_monitors];
      
      ++glgfx_num_monitors;
    }

    if (screen == 0) {
      // Failed to open display ":x.0"
      break;
    }
  }
 
  rc = glgfx_num_monitors != 0;

  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


void glgfx_destroy_monitors(void) {
  int i;

  pthread_mutex_lock(&glgfx_mutex);

  for (i = 0; i < glgfx_num_monitors; ++i) {
    glgfx_monitor_destroy(glgfx_monitors[i]);
    glgfx_monitors[i] = NULL;
  }

  glgfx_num_monitors = 0;

  pthread_mutex_unlock(&glgfx_mutex);
}


bool glgfx_waitblit(void) {
  bool rc = true;
  int i;

  for (i = 0; i < glgfx_num_monitors; ++i) {
    if (!glgfx_monitor_waitblit(glgfx_monitors[i])) {
      rc = false;
    }
  }

  return rc;
}

bool glgfx_swapbuffers(void) {
  bool rc = true;
  int i;

  for (i = 0; i < glgfx_num_monitors; ++i) {
    if (!glgfx_monitor_swapbuffers(glgfx_monitors[i])) {
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

