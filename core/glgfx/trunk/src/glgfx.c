
#define _GNU_SOURCE  // for PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#include "glgfx-config.h"
#include <stdlib.h>
#include <string.h>
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

struct glgfx_tagitem const* glgfx_nexttagitem(struct glgfx_tagitem const** taglist_ptr) {
  if (taglist_ptr == NULL || *taglist_ptr == NULL) {
    return NULL;
  }

  while (true) {
    switch ((*taglist_ptr)->tag) {
      case glgfx_tag_end:
	*taglist_ptr = NULL;
	return NULL;

      case glgfx_tag_more:
	*taglist_ptr = (struct glgfx_tagitem const*) (*taglist_ptr)->data;
	break;

      case glgfx_tag_ignore:
	++(*taglist_ptr);
	break;

      case glgfx_tag_skip:
	*taglist_ptr += (*taglist_ptr)->data + 1;
	break;

      default: {
	struct glgfx_tagitem const* res = *taglist_ptr;

	++(*taglist_ptr);
	return res;
      }
    }
  }
}


bool glgfx_init_a(struct glgfx_tagitem const* tags) {
  (void) tags;
  return true;
}


void glgfx_cleanup() {
}


bool glgfx_create_monitors_a(struct glgfx_tagitem const* tags) {
  bool rc;
  struct glgfx_monitor* friend = NULL;
  struct glgfx_tagitem const* tag;

  pthread_mutex_lock(&glgfx_mutex);
  
  glgfx_destroy_monitors();

  glgfx_num_monitors = 0;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_create_monitors_tag) tag->tag) {

      case glgfx_create_monitors_tag_display: {
	char const* name = (char const*) tag->data;

	glgfx_monitors[glgfx_num_monitors] = glgfx_monitor_create(name, friend);

	if (glgfx_monitors[glgfx_num_monitors] != NULL) {
	  friend = glgfx_monitors[glgfx_num_monitors];
      
	  ++glgfx_num_monitors;
	}

	break;
      }

      case glgfx_create_monitors_tag_unknown:
      case glgfx_create_monitors_tag_max:
	/* Make compiler happy */
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
    char const* msg = (char const*) gluErrorString(error);
    
    BUG("OpenGL error %d %s:%d (%s): %s\n", error, file, line, func, msg);
    abort();
  }
}

