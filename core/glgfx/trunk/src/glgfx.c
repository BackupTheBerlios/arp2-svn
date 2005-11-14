
#define _GNU_SOURCE  // for PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#include "glgfx-config.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include <GL/glu.h>

#include "glgfx.h"
#include "glgfx_intern.h"

/** Signal used for various glgfx timers, like the vsync emulation in
    glgfx_monitor */
static int signum = 0;

/** A copy of the old signal handler */
static struct sigaction old_sa;

pthread_mutex_t glgfx_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;


/** The global signal handler, used for vsync emulation */
static void glgfx_sighandler(int sig, siginfo_t * si, void* extra) {
  printf("vblank\n");
}


bool glgfx_init_a(struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;
  bool rc = false;

  signum = SIGRTMIN + 0;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_init_tag) tag->tag) {
      case glgfx_init_signal:
	signum = tag->data;
	break;

      case glgfx_init_unknown:
      case glgfx_init_max:
	/* Make compiler happy */
	break;
    }
  }

  // Install our signal handler
  struct sigaction sa;
  sa.sa_sigaction = glgfx_sighandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;

  if (sigaction(signum, &sa, &old_sa) == 0) {
    rc = true;
  }

  return rc;
}


void glgfx_cleanup() {
  // Restore signal handler to default
  sigaction(signum, &old_sa, NULL);
}


int glgfx_getattrs_a(void* object, 
		     glgfx_getattr_proto* fn, 
		     struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;
  int count = 0;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    if (fn(object, tag->tag, (intptr_t*) tag->data)) {
      ++count;
    }
  }

  return count;
}


void glgfx_checkerror(char const* func, char const* file, int line) {
  GLenum error = glGetError();

  if (error != 0) {
    char const* msg = (char const*) gluErrorString(error);
    
    BUG("OpenGL error %d %s:%d (%s): %s\n", error, file, line, func, msg);
    abort();
  }
}

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
	if (*taglist_ptr == NULL) {
	  return NULL;
	}
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
