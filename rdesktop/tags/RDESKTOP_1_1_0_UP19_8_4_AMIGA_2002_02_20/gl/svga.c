/*
   rdesktop: A Remote Desktop Protocol client.
   User interface services - SVGAlib
   Copyright (C) Donald Gordon 2001

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <vga.h>
#include <vgagl.h>
#include <vgamouse.h>
#include <vgakeyboard.h>
#include <linux/keyboard.h>

#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include <sys/time.h>

#include <time.h>
#include <errno.h>
#include "rdesktop.h"

extern int width;
extern int height;
extern BOOL sendmotion;
extern BOOL fullscreen;

static uint32 ev_time;

static int mousex = 0, mousey = 0, mouseb = 0;	// mouse state

static uint32 *colmap = 0;

static char keyb[NR_KEYS];
static int keywait[NR_KEYS];
static int repeatable[NR_KEYS];
static int init_repeatable[] = {
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
	0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x47, 0x48, 0x49,
	0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x60,
	0x62, 0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6d, -1
};
#define REPEATRATE 100000

static struct timezone tz_none;

#define MAX_BUTTONS 3
static int mouseb_svga[] = {
	MOUSE_LEFTBUTTON,
	MOUSE_RIGHTBUTTON,
	MOUSE_MIDDLEBUTTON
};
static int mouseb_rdp[] = {
	MOUSE_FLAG_BUTTON1,
	MOUSE_FLAG_BUTTON2,
	MOUSE_FLAG_BUTTON3
};

/* endianness */
static BOOL host_be;
static BOOL xserver_be;

typedef struct {
	int width;
	int height;
	uint8 *data;
} bitmap;

typedef struct {
	int x1, y1;
	int x2, y2;
	int valid;
} rect;

#define VALID(r) (((r).x1<=(r).x2) && ((r).y1<=(r).y2))
#define MIN(x,y) (((x)>(y))?(y):(x))
#define MAX(x,y) (((x)<(y))?(y):(x))
#define NOT(x) (255-(x))

//vgagl already defines these for other purposes
#undef WIDTH
#undef HEIGHT

#define WIDTH(d) ((d).x2-(d).x1+1)
#define HEIGHT(d) ((d).y2-(d).y1+1)

#define MAX_X 799
#define MAX_Y 599

static rect curclip;
static rect mouseclip;
static rect screenclip;

typedef struct {
	uint8 *andmask;
	uint8 *xormask;
	int x;
	int y;
	int w, h;
} cursor;

static cursor mcursor;
static uint8 *mback = NULL;

static GraphicsContext *screen;
static GraphicsContext *backing;

static int fd_vga_evt[2];
static int vga_kb_tick;

void svga_process_mouse (void);
void svga_process_keyboard (int tick);

#define STACKSIZE 16384

/*
 * This is a "kind-of" thr_create() as in pthreads, but not really.
 * It needs some fleshing out to work like pthreads thr_create().
 */
int
start_thread (void (*fn) (void *), void *data)
{
	long retval;
	void **newstack;

	/*
	 * allocate new stack for subthread
	 */
	newstack = (void **) malloc (STACKSIZE);
	if (!newstack)
		return -1;

	/*
	 * Set up the stack for child function, put the (void *)
	 * argument on the stack.
	 */
	newstack = (void **) (STACKSIZE + (char *) newstack);
	*--newstack = data;

	/*
	 * Do clone() system call. We need to do the low-level stuff
	 * entirely in assembly as we're returning with a different
	 * stack in the child process and we couldn't otherwise guarantee
	 * that the program doesn't use the old stack incorrectly.
	 *
	 * Parameters to clone() system call:
	 *      %eax - __NR_clone, clone system call number
	 *      %ebx - clone_flags, bitmap of cloned data
	 *      %ecx - new stack pointer for cloned child
	 *
	 * In this example %ebx is CLONE_VM | CLONE_FS | CLONE_FILES |
	 * CLONE_SIGHAND which shares as much as possible between parent
	 * and child. (We or in the signal to be sent on child termination
	 * into clone_flags: SIGCHLD makes the cloned process work like
	 * a "normal" unix child process)
	 *
	 * The clone() system call returns (in %eax) the pid of the newly
	 * cloned process to the parent, and 0 to the cloned process. If
	 * an error occurs, the return value will be the negative errno.
	 *
	 * In the child process, we will do a "jsr" to the requested function
	 * and then do a "exit()" system call which will terminate the child.
	 */
	__asm__ __volatile__ ("int $0x80\n\t"	/* Linux/i386 system call */
			      "testl %0,%0\n\t"	/* check return value */
			      "jne 1f\n\t"	/* jump if parent */
			      "call *%3\n\t"	/* start subthread function */
			      "movl %2,%0\n\t" "int $0x80\n"	/* exit system call: exit subthread */
			      "1:\t":"=a" (retval)
			      :"0" (__NR_clone), "i" (__NR_exit),
			      "r" (fn),
			      "b" (CLONE_VM | CLONE_FILES | CLONE_FS |
				   CLONE_SIGHAND), "c" (newstack));

	if (retval < 0) {
		errno = -retval;
		retval = -1;
	}
	return retval;
}

void
svga_thread_wait (void *fd)
{
	/* if void *fd came from a local var it would have to be static, or the data it contains will be corrupted. */

	/* all global vars are volatile by definition, so the fact that this is a thread and that it changes global vars
	 * shouldn't be a problem.
	 */

	int evt = 0;
	int ret = 0;
	struct timeval tv, tm, tn, td;

	gettimeofday(&tm, &tz_none);

	vga_kb_tick = 0;
	while (1) { /* repeat until pipe error....  ( ret == -1 ) or process stops. :)*/
		tv.tv_sec = 0;
		tv.tv_usec = REPEATRATE;

		if( (evt = vga_waitevent (VGA_MOUSEEVENT | VGA_KEYEVENT, NULL, NULL, NULL, &tv)) ){
			ret = write (*((int *) fd), &evt, sizeof (evt));
			if (ret < 1 ) {
				if( ret == 0 ){
					STATUS("svga_thread_wait(): pipe closed while sending events, exit...\n");
				}else{
					STATUS("svga_thread_wait(): error while sending events, exit...\n");
				}

				close (*((int *) fd));
				*((int *) fd) = 0;
				return;
			}
		}else{ /* tv timeout */
			gettimeofday(&tn, &tz_none);
			td.tv_sec = tn.tv_sec - tm.tv_sec;
			td.tv_usec = tn.tv_usec - tm.tv_usec;
			while (td.tv_usec < 0) {
				td.tv_sec--;
				td.tv_usec += 1000000;
			}
			if ((td.tv_sec * 1000000 + td.tv_usec) > REPEATRATE) {
				memcpy(&tm, &tn, sizeof (struct timeval));
				vga_kb_tick = 1;
			} else {
				vga_kb_tick = 0;
			}
		}
	}
}

int
ui_select_fd ()
{
	if (pipe (fd_vga_evt) == -1) {
		STATUS("ui_select_fd(): error while creating pipe\n");
		return -1;
	}

	if (start_thread (svga_thread_wait, (void *) &fd_vga_evt[1]) == -1) {
		STATUS("ui_select_fd(): error while creating thread\n");
		close (fd_vga_evt[0]);
		close (fd_vga_evt[1]);
		fd_vga_evt[0] = 0;
		fd_vga_evt[1] = 0;
		return -1;
	}

	return fd_vga_evt[0];
}

inline static unsigned char
rotate(unsigned char i)
{
	i = (i << 4) | (i >> 4);
	i = ((i << 2) & 0xCC) | ((i >> 2) & 0x33);
	i = ((i << 1) & 0xAA) | ((i >> 1) & 0x55);
	return (i);
}


void
init_keycodes (const int id)
{
	STATUS("init_keycodes() is not finished\n");
}

void
ui_process_events ()
{
	int evt = 0;
	if (fd_vga_evt[0] != 0) {
		int ret = read (fd_vga_evt[0], &evt, sizeof (evt));
		if (ret < 1) {
			STATUS("ui_process_events(): event queue terminated\n");
			close (fd_vga_evt[0]);
			fd_vga_evt[0] = 0;
			return;
		}

		if (evt & VGA_MOUSEEVENT)
			svga_process_mouse ();
		if ((evt & VGA_KEYEVENT))
			svga_process_keyboard (vga_kb_tick);
	}
}

void
ui_sync ()
{
/*	STATUS ("ui_sync() is not finished\n"); */
}

BOOL
ui_pending ()
{
	int ret = 0;
	BOOL retval = False;

	struct timeval tv;
	fd_set set;

	FD_ZERO (&set);
	FD_SET (fd_vga_evt[1], &set);

	tv.tv_sec = 0;
	tv.tv_usec = 100;

	if ((ret = select (fd_vga_evt[1] + 1, &set, NULL, NULL, &tv)) < 0) {
		if (errno != EAGAIN && errno != EINTR) {
			STATUS ("ui_pending: select: %s\n", strerror (errno));
			exit (1);
		}
	} else if (ret == 1)
		retval = True;

	return retval;
}

uint16
ui_get_toggle_keys ()
{
	uint16 keys = 0;
	STATUS("ui_get_toggle_keys() is not finished\n");
	return keys;
}

int
svga_intersect (rect * a, rect * b, rect * c)
{
	c->valid = False;
	if (!(a->valid && b->valid))
		return False;

	if (a->x1 > b->x2)
		return False;
	if (b->x1 > a->x2)
		return False;
	if (a->y1 > b->y2)
		return False;
	if (b->y1 > a->y2)
		return False;
	// else

	c->x1 = MAX (a->x1, b->x1);
	c->x2 = MIN (a->x2, b->x2);
	c->y1 = MAX (a->y1, b->y1);
	c->y2 = MIN (a->y2, b->y2);

	c->valid = VALID (*c);
	return c->valid;
}

void svga_draw_mouse (void);
void svga_update_mouse (int ox, int oy);

uint8
svga_rop (int rop, uint8 src, uint8 dst)
{

	switch (rop) {
	case 0x0:
		return 0;
	case 0x1:
		return NOT (src | dst);
	case 0x2:
		return NOT (src) & dst;
	case 0x3:
		return NOT (src);
	case 0x4:
		return src & NOT (dst);
	case 0x5:
		return NOT (dst);
	case 0x6:
		return src ^ dst;
	case 0x7:
		return NOT (src & dst);	// nand
	case 0x8:
		return src & dst;
	case 0x9:
		return NOT (src) ^ dst;
	case 0xa:
		return dst;	// noop
	case 0xb:
		return NOT (src) | dst;
	case 0xc:
		return src;
	case 0xd:
		return src | NOT (dst);
	case 0xe:
		return src | dst;
	case 0xf:
		return NOT (0);
	}
	return dst;		// NOOP

}

void
svga_buildrect (rect * r, int x, int y, int w, int h)
{
	r->x1 = x;
	r->y1 = y;
	r->x2 = x + w - 1;
	r->y2 = y + h - 1;
	r->valid = VALID (*r);
}

void
svga_setclip (rect * r)
{
	gl_setclippingwindow (r->x1, r->y1, r->x2, r->y2);
}

void
svga_copyrect (rect * r)
{
	int w, h;
	rect d;
	rect clip;
	if (!(r->valid))
		return;

	if (!svga_intersect (&screenclip, r, &d))
		return;

	memcpy (&clip, &curclip, sizeof (rect));

	w = WIDTH (d);
	h = HEIGHT (d);

	gl_setcontext (backing);
	//  svga_setclip(&scr);
	gl_copyboxtocontext (d.x1, d.y1, w, h, screen, d.x1, d.y1);
	//  svga_setclip(&clip);
}

void
svga_commitrect (rect * update)
{
	int valid = VALID (*update);
	rect upd, tmp;
	update->valid = valid;
	valid = svga_intersect (&curclip, update, &upd);
	if (!valid)
		return;
	valid = svga_intersect (&upd, &mouseclip, &tmp);

	svga_copyrect (&upd);
	if (valid)
		svga_draw_mouse ();
}

/*
void svga_draw_glyph(int rop, int trans, uint8 fg, uint8 bg, bitmap*
		     b, int x, int y) {
  int i,j;
  uint8* d=b->data;
  int s;

  if (rop!=-1 && rop!=0xa) {

    for (i=0; i<(b->height); i++)
      for (j=0; j<(b->width); j++) {
	if (-1==(s=gl_getpixel(x+j,y+i))) continue;
	if (trans && ! *d) continue;
	gl_setpixel(x+j,y+i,svga_rop(rop,(*d)?fg:bg,s));
      }
  } else {
    for (i=0; i<(b->height); i++)
      for (j=0; j<(b->width); j++, d++) {
	if (trans && ! (*d)) continue;
	gl_setpixel(x+j,y+i,(*d)?fg:bg);
      }
  }

}
*/
void
svga_fill_glyph (int rop, int trans, uint8 fg, uint8 bg, bitmap *
		 b, int x, int y, int w, int h)
{
	int i, j;
	uint8 *d;
	uint8 *r;
	int s;
	int xd;
	int sd;

	for (i = 0; i < h; i++) {
		if ((i + y) < curclip.y1 || (i + y) > curclip.y2)
			continue;
		r = b->data + (b->width * (i % b->height));
		for (xd = 0; xd < w;) {
			d = r;
			for (j = 0; j < (b->width) && xd < w; j++, xd++, d++) {
				if ((x + xd) < curclip.x1
				    || (x + xd) > curclip.x2)
					continue;
				sd = *d;
				s = gl_getpixel (x + xd, y + i);
				if (s == -1) ;	// continue;
				if (!(trans && !sd))
					gl_setpixel (x + xd, y + i,
						     svga_rop (rop,
							       sd ? fg : bg,
							       s));
			}
		}
	}
}

void
svga_fill_rect (int rop, int x, int y, int w, int h, int c)
{

	int i, j;
	int s;
	rect r, d;
	svga_buildrect (&r, x, y, w, h);
	if (!svga_intersect (&r, &curclip, &d))
		return;

	for (i = d.y1; i <= d.y2; i++)
		for (j = d.x1; j <= d.x2; j++) {
			s = gl_getpixel (j, i);
			gl_setpixel (j, i, svga_rop (rop, c, s));
		}
}

void
svga_strip_rop (int rop, int x, int y, int w, uint8 * b)
{
	int i;
	w += x;
	for (i = x; i < w; i++)
		gl_setpixel (i, y, svga_rop (rop, *(b++), gl_getpixel (i, y)));
}

void
svga_draw_cursor (int x, int y, rect * old)
{
	int i, j;
	uint8 *and;
	uint8 *xor;
	uint8 *img;
	int x1, y1, x2, y2;

	x -= mcursor.x;
	y -= mcursor.y;

	x1 = x;
	y1 = y;
	x2 = x + mcursor.w - 1;
	y2 = y + mcursor.h - 1;

	if (!mcursor.andmask)
		return;

	if (x2 < 0 || x1 > MAX_X || y2 < 0 || y1 > MAX_Y)
		return;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > MAX_X)
		x2 = MAX_X;
	if (y2 > MAX_Y)
		y2 = MAX_Y;

	gl_setcontext (backing);

	gl_getbox (x1, y1, x2 - x1 + 1, y2 - y1 + 1, mback);

	img = mback;

	for (i = y1 - y; (i + y) <= y2; i++) {
		and = mcursor.andmask + (x1 - x) + (mcursor.w * i);
		xor = mcursor.xormask + (x1 - x) + (mcursor.w * i);

		for (j = x1 - x; (j + x) <= x2; j++, and++, xor++, img++) {

			if (*and) {
				if (*xor) {
					*img = 255 ^ *img;
				}
			} else
				*img = (*xor) ? 255 : 0;
		}
	}

	// to reduce flicker:
	if (old)
		svga_copyrect (old);

	gl_setcontext (screen);

	gl_putbox (x1, y1, x2 - x1 + 1, y2 - y1 + 1, mback);

	gl_setcontext (backing);

}

void
svga_draw_mouse (void)
{
	//  svga_copyrect(&mouseclip);

	//  gl_setcontext(screen);

	svga_draw_cursor (mousex, mousey, NULL);

	//  gl_setcontext(backing);
	// do nothing
}

void
svga_update_mouse (int x, int y)
{
	rect r;
	if (mcursor.andmask) {
		memcpy (&r, &mouseclip, sizeof (rect));
		svga_buildrect (&mouseclip, x - mcursor.x, y - mcursor.y,
				mcursor.w, mcursor.h);
		svga_draw_cursor (x, y, &r);
	}
}

void
svga_hide_mouse ()
{
	svga_copyrect (&mouseclip);
}

BOOL
ui_create_window (char *title)
{
	int vgamode = G800x600x256;
	int i;
	char *k;

	vga_init ();

	if (!vga_hasmode (vgamode)) {
		error ("Graphics unavailable");
		return False;
	}

	vga_setmousesupport (1);

	vga_setmode (vgamode);

	gl_setcontextvga (vgamode);
	gl_enableclipping ();

	width = 800;
	height = 600;
	svga_buildrect (&screenclip, 0, 0, width, height);

	if (keyboard_init ())
		return False;

	// note DONT_CATCH_ALT_Fx requires a patched svgalib (to ignore Alt-Fx)
#ifndef DONT_CATCH_ALT_Fx
#define DONT_CATCH_ALT_Fx 0
#endif

	keyboard_translatekeys (DONT_CATCH_CTRLC | DONT_CATCH_ALT_Fx);

	k = keyboard_getstate ();

	tz_none.tz_minuteswest = 0;
	tz_none.tz_dsttime = 0;

	for (i = 0; i < NR_KEYS; i++) {
		keyb[i] = k[i];
		keywait[i] = 0;
		repeatable[i] = 0;
	}
	for (i = 0; init_repeatable[i] != -1; i++) {
		repeatable[init_repeatable[i]] = 1;
	}

	//  ui_reset_clip();

	mouseclip.valid = False;
	mcursor.andmask = NULL;
	mcursor.x = 0;
	mcursor.y = 0;
	screen = gl_allocatecontext ();
	gl_getcontext (screen);

	gl_setcontextvgavirtual (vgamode);

	backing = gl_allocatecontext ();
	gl_getcontext (backing);
	ui_reset_clip ();
	return True;
}

void
ui_destroy_window ()
{
	keyboard_close ();
	mouse_close ();
	vga_setmode (TEXT);
}

void
svga_process_mouse (void)
{
	int ox = mousex, oy = mousey, ob = mouseb;
	int i, j;

	mouse_update ();

	ev_time = time (NULL);	// magic incantation issued by xwin.c

	mousex = mouse_getx ();
	mousey = mouse_gety ();
	mouseb = mouse_getbutton ();

	for (i = 0; i < MAX_BUTTONS; i++) {
		j = mouseb & mouseb_svga[i];
		if ((ob & mouseb_svga[i]) != j) {
			rdp_send_input (ev_time, RDP_INPUT_MOUSE,
					mouseb_rdp[i] |
					(j ? MOUSE_FLAG_DOWN : 0), mousex,
					mousey);
		}
	}

	if ((mousex != ox || mousey != oy)) {	// movement
		if (sendmotion)
			rdp_send_input (ev_time, RDP_INPUT_MOUSE,
					MOUSE_FLAG_MOVE, mousex, mousey);
		svga_update_mouse (mousex, mousey);
	}
}

int
svga_keyboard_translate (int scan)
{
	// problems with extended scancodes; this is based on playing with
	// svgalib's keyboard_ routines and the xwin.c source :)
	switch (scan) {
	case 0x60:
		return 0x1c | 0x80;	// enter
	case 0x62:
		return 0x35 | 0x80;	// /

		// note: for this set of keys, if numlock is on, odd things will happen
	case 0x66:
		return 0x47 | 0x80;	// home
	case 0x67:
		return 0x48 | 0x80;	// up
	case 0x68:
		return 0x49 | 0x80;	// pgup
	case 0x69:
		return 0x4b | 0x80;	// left
	case 0x6a:
		return 0x4d | 0x80;	// right
	case 0x6b:
		return 0x4f | 0x80;	// end
	case 0x6c:
		return 0x50 | 0x80;	// down
	case 0x6d:
		return 0x51 | 0x80;	// pgdn
	case 0x6e:
		return 0x52 | 0x80;	// ins
	case 0x6f:
		return 0x53 | 0x80;	// del

	case 0x61:
		return 0x1d | 0x80;	// right-ctrl
	case 0x64:
		return 0x38 | 0x80;	// right-alt
	case 0x7d:
	case 0x7e:
		return 0x7f | 0x80;	// left,right "windows" keys
	case 0x7f:
		return 0x5d | 0x80;	// "menu" key
	}
	return scan;
};

void
svga_process_keyboard (int tick)
{
	int i, l;
	char *k = keyboard_getstate ();

	keyboard_update ();

	ev_time = time (NULL);

	for (i = 0; i < NR_KEYS; i++) {
		if (k[i] && tick)
			keywait[i]++;
		if ((k[i] != keyb[i])
		    || (k[i] && (keywait[i] > 3) && repeatable[i] && tick)) {

			l = svga_keyboard_translate (i);
			if (k[i] != keyb[i])
				keywait[i] = 0;
			else {
				// repeat
				rdp_send_input (ev_time, RDP_INPUT_SCANCODE,
						KBD_FLAG_DOWN | KBD_FLAG_UP, l,
						0);
			}
			rdp_send_input (ev_time, RDP_INPUT_SCANCODE,
					(k[i] ? 0 : KBD_FLAG_DOWN |
					 KBD_FLAG_UP), l, 0);
			keyb[i] = k[i];
		}
	}
}

void
ui_move_pointer (int x, int y)
{
	mouse_setposition (x, y);
	svga_process_mouse ();
}

HBITMAP
ui_create_bitmap (int width, int height, uint8 * data)
{
	bitmap *bmp;
	uint8 *ndata;
	ndata = xmalloc (width * height);
	memcpy (ndata, data, width * height);
	bmp = xmalloc (sizeof (bitmap));
	bmp->width = width;
	bmp->height = height;
	bmp->data = ndata;
	return (HBITMAP) bmp;
}

void
ui_paint_bitmap (int x, int y, int cx, int cy,
		 int width, int height, uint8 * data)
{
	int i;
	rect r;
	rect d;
	svga_buildrect (&r, x, y, cx, cy);
	if (!svga_intersect (&r, &curclip, &d))
		return;

	for (i = d.y1 - y; i <= d.y2 - y; i++)
		gl_putbox (d.x1, y + i, d.x2 - d.x1 + 1, 1,
			   data + (width * i) + (d.x1 - x));

	svga_commitrect (&d);

}

void
ui_destroy_bitmap (HBITMAP bmp)
{
	bitmap *b = (bitmap *) bmp;
	xfree (b->data);
	xfree (b);
}

HGLYPH
ui_create_glyph (int width, int height, uint8 * data)
{
	int x, y;
	bitmap *b = xmalloc (sizeof (bitmap));
	uint8 *d = xmalloc (width * height);
	uint8 *c = data;
	int a = 0;
	b->data = d;
	b->width = width;
	b->height = height;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if ((x & 7) == 0) {
				a = rotate(*c);
				c++;
			}
			*d = (a & 128) ? 255 : 0;
			a = (a & 127) << 1;
			d++;
		}
	}

	return (HGLYPH) b;
}

void
ui_destroy_glyph (HGLYPH glyph)
{
	ui_destroy_bitmap ((bitmap *) glyph);
}

HCURSOR
ui_create_cursor (unsigned int x, unsigned int y, int width,
		  int height, uint8 * andmask, uint8 * xormask)
{
	cursor *c;
	int i, j;

	int iand, ixor;
	uint8 *sand, *sxor, *dand, *dxor;

	c = xmalloc (sizeof (cursor));
	c->x = x;
	c->y = y;
	c->w = width;
	c->h = height;

	dand = c->andmask = xmalloc (width * height);
	dxor = c->xormask = xmalloc (width * height);

	for (i = 0; i < height; i++) {
		sand = andmask + (height - i - 1) * ((width + 7) / 8);
		sxor = xormask + (height - i - 1) * (width * 3);
		for (j = 0; j < width; j++) {
			if ((j & 7) == 0) {
				iand = *(sand++);
				//ixor=*(sxor++);
			}
			*(dand++) = (iand & 0x80) ? 255 : 0;
			*(dxor++) = sxor[0] | sxor[1] | sxor[2];	//(ixor&0x80)?255:0;
			sxor += 3;
			iand = (iand & 0x7f) << 1;
			//      ixor=(ixor & 0x7f) << 1;
		}
		sand++;
		//    sxor++;
	}

	return (HCURSOR) c;
}

void
ui_set_cursor (HCURSOR c_)
{
	cursor *c = (cursor *) c_;

	svga_hide_mouse ();

	memcpy (&mcursor, c, sizeof (cursor));

	if (mback)
		xfree (mback);

	mback = xmalloc (mcursor.w * mcursor.h);

	svga_update_mouse (mousex, mousey);
	// do nothing
}

void
ui_destroy_cursor (HCURSOR cursor_)
{
	cursor *c = (cursor *) cursor_;
	xfree (c->andmask);
	xfree (c->xormask);
	xfree (c);

}

HCOLOURMAP
ui_create_colourmap (COLOURMAP * colours)
{
	int i = 0;
	int n = colours->ncolours;
	COLOURENTRY *c = colours->colours;
	int *cmap = xmalloc (3 * 256 * sizeof (int));
	if (n > 256)
		n = 256;
	bzero (cmap, 256 * 3 * sizeof (int));
	for (i = 0; i < (3 * n); c++) {
		cmap[i++] = (c->red) >> 2;
		cmap[i++] = (c->green) >> 2;
		cmap[i++] = (c->blue) >> 2;
	}
	return cmap;
}

void
ui_destroy_colourmap (HCOLOURMAP map)
{
	if (colmap == map)
		colmap = 0;

	xfree (map);
}

void
ui_set_colourmap (HCOLOURMAP map)
{
	vga_setpalvec (0, 256, (int *) map);
	colmap = map;
}

void
ui_set_clip (int x, int y, int cx, int cy)
{
	rect r;
	//  fprintf(stderr,"setclip(%d,%d,%d,%d)\n",x,y,cx,cy);
	gl_enableclipping ();
	svga_buildrect (&curclip, x, y, cx, cy);
	r.x1 = 0;
	r.y1 = 0;
	r.valid = 1;
	r.x2 = MAX_X;
	r.y2 = MAX_Y;
	svga_setclip (&r);
	//svga_setclip(&curclip);

}

void
ui_reset_clip ()
{
	ui_set_clip (0, 0, width, height);
}

void
ui_bell ()
{
	// unimplemented
}

/* blitting functions:
0000 clear
0001 nor
0010 !and
0011 !copy

0100 and-reverse
0101 !
0110 xor
0111 nand

1000 and
1001 equiv
1010 noop
1011 !or

1100 copy
1101 or-reverse
1110 or
1111 set
*/

void
ui_destblt (uint8 opcode,
	    /* dest */ int x, int y, int cx, int cy)
{
	// this seems a little odd with no given colours, this is a guess...

	rect r;
	rect d;
	svga_buildrect (&r, x, y, cx, cy);
	if (!svga_intersect (&r, &curclip, &d))
		return;

	svga_fill_rect (opcode, d.x1, d.y1, d.x2 - d.x1 + 1, d.y2 - d.y1 + 1.,
			255);

	svga_commitrect (&d);
}

void
ui_patblt (uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy,
	   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	rect r;
	rect d;
	bitmap *fill;

	svga_buildrect (&r, x, y, cx, cy);
	if (!svga_intersect (&r, &curclip, &d))
		return;

	switch (brush->style) {
	case 0:		/* Solid */
		svga_fill_rect (opcode, d.x1, d.y1, d.x2 - d.x1 + 1,
				d.y2 - d.y1 + 1, fgcolour);
		break;

	case 3:		/* Pattern */
        
		fill = (bitmap *) ui_create_glyph (8, 8, brush->pattern);

		svga_fill_glyph (opcode, 0, bgcolour, fgcolour, fill, x, y, cx,
				 cy);

		ui_destroy_glyph ((HGLYPH) fill);
		break;

	default:
		unimpl ("brush %d\n", brush->style);
	}

	svga_commitrect (&d);
}

void
ui_screenblt (uint8 opcode,
	      /* dest */ int x, int y, int cx, int cy,
	      /* src */ int srcx, int srcy)
{
	// FIXME: opcode ignored
	rect r;
	rect d;
	uint8 *data;
	bitmap b;
	svga_buildrect (&r, x, y, cx, cy);
	if (!svga_intersect (&r, &curclip, &d))
		return;

	srcx += d.x1 - x;
	srcy += d.y1 - y;

	if (opcode == ROP2_COPY) {
		gl_copybox (srcx, srcy, WIDTH (d), HEIGHT (d), d.x1, d.y1);
		svga_commitrect (&d);
	} else {
		data = xmalloc (WIDTH (d) * HEIGHT (d));
		b.width = WIDTH (d);
		b.height = HEIGHT (d);
		gl_getbox (srcx, srcy, WIDTH (d), HEIGHT (d), data);
		ui_memblt (opcode, d.x1, d.y1, b.width, b.height, &b, colmap,
			   srcx, srcy);
	}
}

void
ui_memblt (uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy,
	   /* src */ HBITMAP src, HCOLOURMAP map, int srcx, int srcy)
{
	// FIXME: opcode ignored

	bitmap *b = (bitmap *) src;
	int i;
	rect r;
	rect d;

	if( colmap != map )
		ui_set_colourmap( map );

	svga_buildrect (&r, x, y, cx, cy);
	if (!svga_intersect (&r, &curclip, &d))
		return;

	srcx += d.x1 - x;
	srcy += d.y1 - y;
	cx = WIDTH (d);
	cy = HEIGHT (d);
	x = d.x1;
	y = d.y1;

	if (opcode == ROP2_COPY) {
		if (cx == b->width)
			gl_putbox (x, y, cx, cy, b->data + (srcy * b->width));
		else
			for (i = 0; i < cy; i++)
				gl_putbox (x, y + i, cx, 1,
					   b->data + ((srcy + i) * b->width));
	} else
		for (i = 0; i < cy; i++)
			svga_strip_rop (opcode, x, y + i, cx,
					b->data + ((srcy + i) * b->width));

	svga_commitrect (&d);

}

void
ui_triblt (uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy,
	   /* src */ HBITMAP src, HCOLOURMAP map, int srcx, int srcy,
	   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{

	if (colmap != map)
		ui_set_colourmap (map);

	/* This is potentially difficult to do in general. Until someone
	   comes up with a more efficient way of doing it I am using cases. */

	switch (opcode) {
	case 0x69:		/* PDSxxn */
		ui_memblt (ROP2_XOR, x, y, cx, cy, src, map, srcx, srcy);
		ui_patblt (ROP2_NXOR, x, y, cx, cy, brush, bgcolour, fgcolour);
		break;

	case 0xb8:		/* PSDPxax */
		ui_patblt (ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
		ui_memblt (ROP2_AND, x, y, cx, cy, src, map, srcx, srcy);
		ui_patblt (ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
		break;

	case 0xc0:		/* PSa */
		ui_memblt (ROP2_COPY, x, y, cx, cy, src, map, srcx, srcy);
		ui_patblt (ROP2_AND, x, y, cx, cy, brush, bgcolour, fgcolour);
		break;

	default:
		unimpl ("triblt 0x%x\n", opcode);
		ui_memblt (ROP2_COPY, x, y, cx, cy, src, map, srcx, srcy);
	}

}


void
ui_line(uint8 opcode,
       /* dest */ int startx, int starty, int endx, int endy,
       /* pen */ PEN *pen)
{

  long rx,ry,x,y,dx,dy,l,i;
  int col=pen->colour;
  rect r;
  rect d;
  r.x1=MIN(startx,endx);
  r.x2=MAX(startx,endx);
  r.y1=MIN(starty,endy);
  r.y2=MAX(starty,endy);
  r.valid=VALID(r);
  if (!svga_intersect(&r,&curclip,&d)) return;

  dx = endx - startx;
  dy = endy - starty;

#define ABS(x) (((x)<0)?-(x):(x))

  l=MAX(ABS(dx),ABS(dy));

#undef ABS

  x=startx<<16; y=starty<<16;

  dx=(dx<<16)/l;
  dy=(dy<<16)/l;

  for (i=0; i<=l; i++) {
   rx=x>>16; ry=y>>16;
   x+=dx; y+=dy;
   if (rx<curclip.x1 || rx>curclip.x2 || ry<curclip.y1 ||
     ry>curclip.y2) continue;
   gl_setpixel(rx,ry,svga_rop(opcode,col,gl_getpixel(rx,ry)));
  }

  svga_commitrect(&d);
}


void
ui_rect (
		/* dest */ int x, int y, int cx, int cy,
		/* brush */ int colour)
{
	rect r;
	rect d;
	svga_buildrect (&r, x, y, cx, cy);
	if (!svga_intersect (&r, &curclip, &d))
		return;
	gl_fillbox (d.x1, d.y1, WIDTH (d), HEIGHT (d), colour);
	svga_commitrect (&d);
}

void
ui_draw_glyph (int mixmode,
	       /* dest */ int x, int y, int cx, int cy,
	       /* src */ HGLYPH glyph, int srcx, int srcy, int bgcolour,
	       int fgcolour, HBITMAP dst)
{
	// more of a fill_glyph, really

	// FIXME: the dst bitmap isn't used currently.
	// I doubt it is needed, except for compatibility over the interfaces.

	rect r;
	rect d;
	svga_buildrect (&r, x, y, cx, cy);
	if (!svga_intersect (&r, &curclip, &d))
		return;

	// xwin ignores srcx, srcy; so shall I.

	svga_fill_glyph (ROP2_COPY, mixmode == MIX_TRANSPARENT, fgcolour,
			 bgcolour, (bitmap *) glyph, x, y, cx, cy);

	svga_commitrect (&d);

}


#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
    {\
      xyoffset = ttext[++idx];\
      if ((xyoffset & 0x80))\
        {\
          if (flags & TEXT2_VERTICAL) \
            y += ttext[++idx] | (ttext[++idx] << 8);\
          else\
            x += ttext[++idx] | (ttext[++idx] << 8);\
        }\
      else\
        {\
          if (flags & TEXT2_VERTICAL) \
            y += xyoffset;\
          else\
            x += xyoffset;\
        }\
    }\
  if (glyph != NULL)\
    {\
      ui_draw_glyph (mixmode, x + (short) glyph->offset,\
                     y + (short) glyph->baseline,\
                     glyph->width, glyph->height,\
                     glyph->pixmap, 0, 0, bgcolour, fgcolour,\
                     0);\
      if (flags & TEXT2_IMPLICIT_X)\
        x += glyph->width;\
    }\
}

void
ui_draw_text(uint8 font, uint8 flags, int mixmode, int x, int y,
	     int clipx, int clipy, int clipcx, int clipcy, int boxx,
	     int boxy, int boxcx, int boxcy, int bgcolour,
	     int fgcolour, uint8 * text, uint8 length)
{
	FONTGLYPH *glyph;
	int i, j, xyoffset;

	DATABLOB *entry;

	if (boxcx > 1)
		ui_rect (boxx, boxy, boxcx, boxcy, bgcolour);
	else if (mixmode == MIX_OPAQUE)
		ui_rect (clipx, clipy, clipcx, clipcy, bgcolour);

	/* Paint text, character by character */
	for (i = 0; i < length;) {
		switch (text[i]) {
		case 0xff:
			if (i + 2 < length)
				cache_put_text(text[i + 1], text, text[i + 2]);
			else {
				error("this shouldn't be happening\n");
				break;
			}
			/* this will move pointer from start to first character after FF command */
			length -= i + 3;
			text = &(text[i + 3]);
			i = 0;
			break;

		case 0xfe:
			entry = cache_get_text(text[i + 1]);
			if (entry != NULL) {
				if ((((uint8 *) (entry->data))[1] == 0)
				    && (!(flags & TEXT2_IMPLICIT_X))) {
					if (flags & 0x04)	/* vertical text */
						y += text[i + 2];
					else
						x += text[i + 2];
				}
				if (i + 2 < length)
					i += 3;
				else
					i += 2;
				length -= i;
				/* this will move pointer from start to first character after FE command */
				text = &(text[i]);
				i = 0;
				for (j = 0; j < entry->size; j++)
					DO_GLYPH(((uint8 *) (entry->data)), j);
			}
			break;

		default:
			DO_GLYPH(text, i);
			i++;
			break;
		}
	}
}

void
ui_desktop_save (uint32 offset, int x, int y, int cx, int cy)
{
	uint8 *d;
	d = xmalloc (cx * cy);
	gl_getbox (x, y, cx, cy, d);
	cache_put_desktop (offset, cx, cy, cx, 1, d);
	xfree (d);
}

void
ui_desktop_restore (uint32 offset, int x, int y, int cx, int cy)
{
	uint8 *d = cache_get_desktop (offset, cx, cy, 1);
	if (!d)
		return;
	// else

	ui_paint_bitmap (x, y, cx, cy, cx, cy, d);

}
