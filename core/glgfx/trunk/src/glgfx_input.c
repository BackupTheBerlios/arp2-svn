
#include "glgfx-config.h"
#include <stdlib.h>
#include <glib.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "glgfx.h"
#include "glgfx_input.h"
#include "glgfx_intern.h"
#include "glgfx_monitor.h"

static GQueue* pending_queue;
static GQueue* cache_queue;

static enum glgfx_input_code xorg_codes[256] = {
  glgfx_input_none,
  glgfx_input_none,
  glgfx_input_none,
  glgfx_input_none,
  glgfx_input_none,
  glgfx_input_none,
  glgfx_input_none,
  glgfx_input_none,
  glgfx_input_none,

  // Row 1
  glgfx_input_esc,	// 9
  glgfx_input_1,
  glgfx_input_2,
  glgfx_input_3,
  glgfx_input_4,
  glgfx_input_5,
  glgfx_input_6,
  glgfx_input_7,
  glgfx_input_8,
  glgfx_input_9,
  glgfx_input_0,
  glgfx_input_minus,
  glgfx_input_equal,
  glgfx_input_bs,

  // Row 2
  glgfx_input_tab,	// 23
  glgfx_input_q,
  glgfx_input_w,
  glgfx_input_e,
  glgfx_input_r,
  glgfx_input_t,
  glgfx_input_y,
  glgfx_input_u, 
  glgfx_input_i,
  glgfx_input_o,
  glgfx_input_p,
  glgfx_input_lbracket,
  glgfx_input_rbracket,
  glgfx_input_return,
 

  // Row 3
  glgfx_input_lctrl,	// 37
  glgfx_input_a,
  glgfx_input_s,
  glgfx_input_d,
  glgfx_input_f,
  glgfx_input_g,
  glgfx_input_h,
  glgfx_input_j,
  glgfx_input_k,
  glgfx_input_l,
  glgfx_input_semicolon,
  glgfx_input_quote,
  glgfx_input_backquote,

  // Row 4
  glgfx_input_lshift,	// 50
  glgfx_input_backslash,
  glgfx_input_z,
  glgfx_input_x,
  glgfx_input_c,
  glgfx_input_v,
  glgfx_input_b, 
  glgfx_input_n,
  glgfx_input_m,
  glgfx_input_comma,
  glgfx_input_period,
  glgfx_input_slash,
  glgfx_input_rshift,
 
  glgfx_input_npmul,

  // Row 5
  glgfx_input_lalt,	// 64
  glgfx_input_space,
  glgfx_input_capslock,
  glgfx_input_f1,
  glgfx_input_f2,
  glgfx_input_f3,
  glgfx_input_f4,
  glgfx_input_f5,
  glgfx_input_f6,
  glgfx_input_f7,
  glgfx_input_f8,
  glgfx_input_f9,
  glgfx_input_f10,

  glgfx_input_numlock,
  glgfx_input_scrlock,
  glgfx_input_np7,
  glgfx_input_np8,
  glgfx_input_np9,
  glgfx_input_npsub,
  glgfx_input_np4,
  glgfx_input_np5,
  glgfx_input_np6,
  glgfx_input_npadd,
  glgfx_input_np1,
  glgfx_input_np2,
  glgfx_input_np3,
  glgfx_input_np0,
  glgfx_input_npdel,

  glgfx_input_none,	// 92
  glgfx_input_none,	// 93
  glgfx_input_ltgt,
  glgfx_input_f11,
  glgfx_input_f12,
  glgfx_input_home,
  glgfx_input_up,
  glgfx_input_pageup,
  glgfx_input_left,
  glgfx_input_none,	// 101
  glgfx_input_right,
  glgfx_input_end,
  glgfx_input_down,
  glgfx_input_pagedown,
  glgfx_input_insert,
  glgfx_input_del,
  glgfx_input_enter,
  glgfx_input_rctrl,
  glgfx_input_pause,
  glgfx_input_prtscr,
  glgfx_input_npdiv,
  glgfx_input_ralt,
  glgfx_input_none,	// 114
  glgfx_input_lmeta,
  glgfx_input_rmeta,
  glgfx_input_menu,

  glgfx_input_none,	// 118
  glgfx_input_none,	// 119
  glgfx_input_none,	// 120
  glgfx_input_none,	// 121
  glgfx_input_none,	// MM Search
  glgfx_input_none,	// 123
  glgfx_input_none,	// 124
  glgfx_input_none,	// 125
  glgfx_input_none,	// 126
  glgfx_input_none,	// 127
  glgfx_input_none,	// 128
  glgfx_input_none,	// MM Media
  glgfx_input_none,	// MM Home
  glgfx_input_none,	// 131
  glgfx_input_none,	// 132
  glgfx_input_none,	// 133
  glgfx_input_none,	// 134
  glgfx_input_none,	// 135
  glgfx_input_none,	// 136
  glgfx_input_none,	// 137
  glgfx_input_none,	// 138
  glgfx_input_none,	// 139
  glgfx_input_none,	// 140
  glgfx_input_none,	// 141
  glgfx_input_none,	// 142
  glgfx_input_none,	// 143
  glgfx_input_prev,
  glgfx_input_none,	// 145
  glgfx_input_none,	// 146
  glgfx_input_none,	// 147
  glgfx_input_none,	// 148
  glgfx_input_none,	// 149
  glgfx_input_none,	// 150
  glgfx_input_none,	// 151
  glgfx_input_none,	// 152
  glgfx_input_next,
  glgfx_input_none,	// 154
  glgfx_input_none,	// 155
  glgfx_input_none,	// 156
  glgfx_input_none,	// 157
  glgfx_input_none,	// 158
  glgfx_input_none,	// 159
  glgfx_input_none,	// MM Mute
  glgfx_input_none,	// 161
  glgfx_input_play,
  glgfx_input_none,	// 163
  glgfx_input_stop,
  glgfx_input_none,	// 165
  glgfx_input_none,	// 166
  glgfx_input_none,	// 167
  glgfx_input_none,	// 168
  glgfx_input_none,	// 169
  glgfx_input_none,	// 170
  glgfx_input_none,	// 171
  glgfx_input_none,	// 172
  glgfx_input_none,	// 173
  glgfx_input_none,	// MM VolDown
  glgfx_input_none,	// 175
  glgfx_input_none,	// MM VolUp

/*   sleep = 223, */
/*   favourites = 230, */
/*   email = 236, */
  
/*   glgfx_input_nplparen, */
/*   glgfx_input_nprparen, */
/*   glgfx_input_numbersign = 0x2b, */
};

bool glgfx_input_acquire(bool safety_net) {
  static bool safety_net_installed = false;
  bool rc = true;
  int i;

  if (pending_queue == NULL) {
    pending_queue = g_queue_new();
  }

  if (cache_queue == NULL) {
    cache_queue = g_queue_new();
  }
  
  for (i = 0; i < glgfx_num_monitors; ++i) {
    struct glgfx_monitor* monitor = glgfx_monitors[i];
    Window rootwin = XRootWindow(monitor->display, DefaultScreen(monitor->display));
    
    XGrabKeyboard(monitor->display, rootwin, 1, GrabModeAsync,
		  GrabModeAsync,  CurrentTime);
    XGrabPointer(monitor->display, rootwin, 1, PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None,  None, CurrentTime);
    XF86DGADirectVideo(monitor->display, DefaultScreen(monitor->display),
		       XF86DGADirectMouse | XF86DGADirectKeyb);
  }

  if (safety_net && !safety_net_installed) {
    safety_net_installed = true;

    printf("forking ...\n");
    pid_t pid = fork();

    switch (pid)  {
      case -1:
	// Fork failed
	printf("failed\n");
	safety_net_installed = false;
	rc = false;
	break;

      case 0:
	// Child
	printf("child\n");
	break;
	
      default: {
	// Wait for child, then unlock display
	// and exit

	int rc;
	
	printf("ok; sleeping...\n");
	if (waitpid(pid, &rc, 0) == -1) {
	  perror("waitpid");
	  exit(20);
	}

	printf("releasing\n");
	glgfx_input_release();

	if (WIFEXITED(rc)) {
	  printf("exiting\n");
	  exit(WEXITSTATUS(rc));
	}
	else if (WIFSIGNALED(rc)) {
	  printf("raising signal\n");
	  raise(WTERMSIG(rc));
	}
	else {
	  printf("aborting\n");
	  abort();
	}
      }
    }
  }

  return rc;
}

bool glgfx_input_release(void) {
  int i;
  
  for (i = 0; i < glgfx_num_monitors; ++i) {
    struct glgfx_monitor* monitor = glgfx_monitors[i];

    XF86DGADirectVideo(monitor->display, DefaultScreen(monitor->display), 0);
    XUngrabPointer(monitor->display, CurrentTime);
    XUngrabKeyboard(monitor->display, CurrentTime);
  }

  void free_event(gpointer* data, gpointer* userdata) {
    (void) userdata;
    free(data);
  }  
  
  if (pending_queue != NULL) {
    g_queue_foreach(pending_queue, (GFunc) free_event, NULL);
    g_queue_free(pending_queue);
  }

  if (cache_queue != NULL) {
    g_queue_foreach(cache_queue, (GFunc) free_event, NULL);
    g_queue_free(cache_queue);
  }
  
  return true;
}


static void fill_queue(void) {
  int i;

  for (i = 0; i < glgfx_num_monitors; ++i) {
    while (XPending(glgfx_monitors[i]->display)) {
      XEvent* event;

      if (!g_queue_is_empty(cache_queue)) {
	event = g_queue_pop_head(cache_queue);
      }
      else {
	event = calloc(1, sizeof (*event));
      }
      
      XNextEvent(glgfx_monitors[i]->display, event);
      g_queue_push_tail(pending_queue, event);
    }
  }
}

enum glgfx_input_code glgfx_input_getcode(void) {
  enum glgfx_input_code code = glgfx_input_none;
 
  if (g_queue_is_empty(pending_queue)) {
    // For fairness, we scan all displays in order
    fill_queue();
  }
 
  if (!g_queue_is_empty(pending_queue)) {
    XEvent* event = g_queue_pop_head(pending_queue);

    // Return event to cache queue
    g_queue_push_tail(cache_queue, event);

    switch (event->type) {
      case KeyPress:
      case KeyRelease: {
	XKeyEvent* ev = (XKeyEvent*) event;
	
	if (ev->keycode < 256) {
	  enum glgfx_input_code c = xorg_codes[ev->keycode];

//	  if (ev->altmask is true
	  
	  code = (glgfx_input_key | c |
		  (event->type == KeyRelease ? glgfx_input_releasemask : 0));
	}
	else {
	  return glgfx_input_getcode();
	}
	break;
      }

      case ButtonPress:
      case ButtonRelease: {
	XButtonEvent* ev = (XButtonEvent*) event;
	code = (event->type == ButtonRelease ? glgfx_input_releasemask : 0);

	switch (ev->button) {
	  case 1:
	    code |= glgfx_input_mouse_button | 0;
	    break;

	  case 2:
	    code |= glgfx_input_mouse_button | 2;
	    break;

	  case 3:
	    code |= glgfx_input_mouse_button | 1;
	    break;

	  case 4:
	    if (event->type == ButtonPress) {
	      code |= glgfx_input_mouse_vwheel | (+16 & 0xffff);
	    }
	    else {
	      return glgfx_input_getcode();
	    }
	    break;

	  case 5:
	    if (event->type == ButtonPress) {
	      code |= glgfx_input_mouse_vwheel | (-16 & 0xffff);
	    }
	    else {
	      return glgfx_input_getcode();
	    }
	    break;

	  default:
	    return glgfx_input_getcode();
	}

	break;
      }

      case MotionNotify: {
	XMotionEvent* ev = (XMotionEvent*) event;
	int dx = ev->x_root;
	int dy = ev->y_root;

	code = glgfx_input_mouse_xyz | ((dy & 0xff) << 8) | (dx & 0xff);
	break;
      }

      default:
	return glgfx_input_getcode();
    }
  }

  return code;
}
