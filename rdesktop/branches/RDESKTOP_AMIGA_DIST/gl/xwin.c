/*
   rdesktop: A Remote Desktop Protocol client.
   User interface services - X-Windows
   Copyright (C) Matthew Chapman 1999-2001

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


#define XK_MISCELLANY
#define XK_LATIN1
#define XK_CURRENCY
#define XK_XKB_KEYS

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef WITH_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <sys/time.h>
#include <rdesktop.h>
#include <stdlib.h>

#include <keymap_wrap.c>

#define BITSPERBYTES 8
#define TOBYTES(bits) ((bits)/BITSPERBYTES)
extern int width;
extern int height;
extern BOOL sendmotion;
extern int wxpos;
extern int wypos;
extern int bpp;
extern BOOL grab_keyboard;
extern BOOL fullscreen;
extern int backing_store;
extern int private_colormap;
extern int desktopsize_percent;
extern BOOL systembeep;
extern BOOL broken_x_kb;
extern BOOL vncviewer;
static int depth = -1;
static Display *display = NULL;
static Window wnd;
static GC gc = NULL;
static GC back_gc = NULL;
static GC glyph_gc = NULL;
static Visual *visual;
static XIM IM;
static uint32 *colmap;
static Pixmap back_pixmap;
static int dpy_width;
static int dpy_height;
static unsigned int key_modifier_state = 0;
static unsigned int numlock_modifier_mask = 0;
static unsigned int key_down_state = 0;
static int xserver_be = 0;
static int last_main_gc_func = -1;

#define DShift1Mask   (1<<0)
#define DShift2Mask   (1<<1)
#define DControl1Mask (1<<2)
#define DControl2Mask (1<<3)
#define DMod1Mask     (1<<4)
#define DMod2Mask     (1<<5)

#define Ctrans(col) ( private_colormap ? col : colmap[col])

static uint8 *translate(int width, int height, uint8 * data);

inline static void
 xwin_set_function(uint8 rop2);

void
ui_grab_keyboard();

void
ui_ungrab_keyboard();


inline static unsigned char
rotate(unsigned char i)
{
	i = (i << 4) | (i >> 4);
	i = ((i << 2) & 0xCC) | ((i >> 2) & 0x33);
	i = ((i << 1) & 0xAA) | ((i >> 1) & 0x55);
	return (i);
}

int
ui_select_fd()
{
	if (display == NULL) {
		return -1;
	}

	else {
		return XConnectionNumber(display);
	}
}

BOOL ui_create_window(char *title)
{
	XSetWindowAttributes attribs;
	XClassHint *classhints;
	XSizeHints *sizehints;
	unsigned long input_mask;
	int count = 0;
	XPixmapFormatValues *pfm;
	KeyCode numlockcode;
	KeyCode *keycode;
	int i, j;
	XModifierKeymap *modmap;

#ifdef WITH_XINERAMA
	int screens;
	XineramaScreenInfo *screeninfo;
	int event_base;
	int error_base;
#endif

	if (width == -1)
	  width = 800;

	if (height == -1)
	  height = 600;

	display = XOpenDisplay(NULL);
	if (display == NULL) {
		error("ui_create_window(): Failed to open display\n");
		return False;
	}
	visual = DefaultVisual(display, DefaultScreen(display));

	depth = DefaultDepth(display, DefaultScreen(display));
	pfm = XListPixmapFormats(display, &count);
	if (pfm != NULL) {
		if (bpp < 0)
			while (count--) {
				if ((pfm + count)->depth == depth
				    && (pfm + count)->bits_per_pixel > bpp) {
					bpp = (pfm + count)->bits_per_pixel;
				}
			}
		XFree(pfm);
	}

	if (bpp < 8) {		/* oops... */
		printf("currently we do not support bpp below 8 bpp.\n");
		/* do we need to close the display here? XCloseDisplay(display) */
		return False;
	}
#ifndef X_SYNCED
	XSynchronize(display, False);	/* production -- let it be unsynced */
#else
	XSynchronize(display, True);	/* DEBUG -- run messages in a synchronized manner */
#endif

	DEBUG("ui_create_window(): using screen depth: %u bpp: %u\n", depth,
	      bpp);

	/* visual->class == TrueColor ? */

	{
		XColor bgp;
		bgp.red = 0xcc << 8;
		bgp.green = 0xcc << 8;
		bgp.blue = 0xcc << 8;
		bgp.flags = DoRed | DoBlue | DoGreen;
		if (XAllocColor
		    (display, DefaultColormap(display, DefaultScreen(display)),
		     &bgp) == 0)
			attribs.background_pixel =
			    BlackPixel(display, DefaultScreen(display));
		else
			attribs.background_pixel = bgp.pixel;
	}

	if (backing_store == BS_XWIN) {
		attribs.background_pixel =
		    BlackPixel(display, DefaultScreen(display));
		attribs.backing_store = Always;
	} else
		attribs.backing_store = NotUseful;

	dpy_width = WidthOfScreen(DefaultScreenOfDisplay(display));
	dpy_height = HeightOfScreen(DefaultScreenOfDisplay(display));

	if (desktopsize_percent) {
		width = dpy_width * desktopsize_percent / 100;
		height = dpy_height * desktopsize_percent / 100;
	}

	/* hack: make sure the width is an even 32-bit word ( in pixels. ) */
	width &= ~0x3;

	if (fullscreen) {
		attribs.override_redirect = True;
		width = dpy_width;
		height = dpy_height;
		wxpos = 0;
		wypos = 0;
#ifdef WITH_XINERAMA
		if (Success !=
		    XineramaQueryExtension(display, &event_base, &error_base)) {
			screeninfo = XineramaQueryScreens(display, &screens);
			/* Xinerama Detected */
			DEBUG
			    ("Display is using Xinerama with %d screens, first screen is %dx%d\n",
			     screens, screeninfo[0].width,
			     screeninfo[0].height);
			width = screeninfo[0].width;
			height = screeninfo[0].height;
		} else {
			/* no Xinerama */
			DEBUG("Display is not using Xinerama.\n");
			width = dpy_width;
			height = dpy_height;
		}
#endif

		wnd =
		    XCreateWindow(display, DefaultRootWindow(display), wxpos,
				  wypos, width, height, 0, CopyFromParent,
				  InputOutput, CopyFromParent,
				  CWBackingStore | CWBackPixel |
				  CWOverrideRedirect, &attribs);

	} else {
		wnd =
		    XCreateWindow(display, DefaultRootWindow(display), wxpos,
				  wypos, width, height, 0, CopyFromParent,
				  InputOutput, CopyFromParent,
				  CWBackingStore | CWBackPixel, &attribs);
	}

	DEBUG("ui_create_window(): geometry selected:%ux%u\n", width, height);

	sizehints = XAllocSizeHints();
	if (sizehints) {
		sizehints->flags =
		    PPosition | PSize | PMinSize | PMaxSize | PBaseSize;
		sizehints->min_width = width;
		sizehints->max_width = width;
		sizehints->min_height = height;
		sizehints->max_height = height;
		sizehints->base_width = width;
		sizehints->base_height = height;
		XSetWMNormalHints(display, wnd, sizehints);
		XFree(sizehints);
	}
	XStoreName(display, wnd, title);
	classhints = XAllocClassHint();
	if (classhints != NULL) {
		classhints->res_name = "rdesktop";
		classhints->res_class = "rdesktop";
		XSetClassHint(display, wnd, classhints);
		XFree(classhints);
	}
	input_mask = KeyPressMask | KeyReleaseMask;
	input_mask |= ExposureMask | StructureNotifyMask;
	input_mask |= ButtonPressMask | ButtonReleaseMask;
	if (sendmotion)
		input_mask |= PointerMotionMask;
	if (grab_keyboard)
		input_mask |= EnterWindowMask | LeaveWindowMask;
	XSelectInput(display, wnd, input_mask);
	gc = XCreateGC(display, wnd, 0, NULL);
	IM = XOpenIM(display, NULL, NULL, NULL);

	/* clear the window so that cached data is not viewed upon start... */
	XSetBackground(display, gc, 0);
	XSetForeground(display, gc, 0);
	XFillRectangle(display, wnd, gc, 0, 0, width, height);

	XMapWindow(display, wnd);

	/* Wait for the MapNotify Event */
	for (;;) {
		XEvent e;
		XNextEvent(display, &e);
		if (e.type == MapNotify)
			break;
	}

	XSetInputFocus(display, wnd, RevertToPointerRoot, CurrentTime);

	/* Create a pixmap for backing the window */
	if (backing_store == BS_PIXMAP) {
		XGCValues gcvalues;

		back_pixmap =
		    XCreatePixmap(display, DefaultRootWindow(display), width,
				  height, depth);

		gcvalues.graphics_exposures = 0;
		back_gc =
		    XCreateGC(display, back_pixmap, GCGraphicsExposures,
			      &gcvalues);

	}

	if (!vncviewer) {
		/* Find out if numlock is already defined as a modifier key, and if so where */
		numlockcode = XKeysymToKeycode(display, 0xFF7F);	/* XF_Num_Lock = 0xFF7F */
		if (numlockcode) {
			modmap = XGetModifierMapping(display);
			if (modmap) {
				keycode = modmap->modifiermap;
				for (i = 0; i < 8; i++)
					for (j = modmap->max_keypermod; j--;) {
						if (*keycode == numlockcode) {
							numlock_modifier_mask =
							    (1 << i);
							i = 8;
							break;
						}
						keycode++;
					}
				if (!numlock_modifier_mask) {
					modmap->modifiermap[7 *
							    modmap->
							    max_keypermod] =
					    numlockcode;
					if (XSetModifierMapping(display, modmap)
					    == MappingSuccess)
						numlock_modifier_mask =
						    (1 << 7);
					else
						printf
						    ("XSetModifierMapping failed!\n");
				}
				XFreeModifiermap(modmap);
			}
		}

		if (!numlock_modifier_mask)
			printf
			    ("WARNING: Failed to get a numlock modifier mapping.\n");
	}

	if (ImageByteOrder(display) == MSBFirst)
		xserver_be = 1;

	return True;
}

void
ui_destroy_window()
{
	if (backing_store == BS_PIXMAP) {
		XFreeGC(display, back_gc);
		XFreePixmap(display, back_pixmap);
	}
	if (glyph_gc)
		XFreeGC(display, glyph_gc);
	if (IM)
		XCloseIM(IM);
	XFreeGC(display, gc);
	XDestroyWindow(display, wnd);
	XCloseDisplay(display);
	display = NULL;
}

static uint8
xwin_translate_key2(unsigned long key)
{
	if ((key > 8) && (key <= 0x60))
		return (key - 8);
	switch (key) {
	case 0x61:		/* home */
		return 0x47 | 0x80;
	case 0x62:		/* up arrow */
		return 0x48 | 0x80;
	case 0x63:		/* page up */
		return 0x49 | 0x80;
	case 0x64:		/* left arrow */
		return 0x4b | 0x80;
	case 0x66:		/* right arrow */
		return 0x4d | 0x80;
	case 0x67:		/* end */
		return 0x4f | 0x80;
	case 0x68:		/* down arrow */
		return 0x50 | 0x80;
	case 0x69:		/* page down */
		return 0x51 | 0x80;
	case 0x6a:		/* insert */
		return 0x52 | 0x80;
	case 0x6b:		/* delete */
		return 0x53 | 0x80;
	case 0x6c:		/* keypad enter */
		return 0x1c | 0x80;
	case 0x6d:		/* right ctrl */
		return 0x1d | 0x80;
	case 0x6f:		/* ctrl - print screen */
		return 0x37 | 0x80;
	case 0x70:		/* keypad '/' */
		return 0x35 | 0x80;
	case 0x71:		/* right alt */
		return 0x38 | 0x80;
	case 0x72:		/* ctrl break */
		return 0x46 | 0x80;
	case 0x73:		/* left window key */
		return 0xff;	/* real scancode is 5b */
	case 0x74:		/* right window key */
		return 0xff;	/* real scancode is 5c */
	case 0x75:		/* menu key */
		return 0x5d | 0x80;
	}
	return 0;
}

static uint8
xwin_translate_key(XKeyEvent * ev)
{
	uint16 scan = 0;
	if (broken_x_kb) {
		char buf[256];
		KeySym key;
		XLookupString(ev, buf, 256, &key, 0);
		scan = XKeysym2PCKeyCode[key & KS_MASK];
		if (scan == 0) {	/* this is not a special code */
			scan = XKeysym2PCKeyCode[(unsigned char) *buf];
			if (vncviewer) {
				if ((scan & 0x100) && !(ev->state & ShiftMask))
					ev->state ^= ShiftMask;
				else if (!(scan & 0x100)
					 && (ev->state & ShiftMask))
					ev->state ^= ShiftMask;
				if (scan & 0x200 && (!(ev->state & ControlMask)
						     || !(ev->state &
							  Mod1Mask))) ev->
					    state |= (ControlMask | Mod1Mask);
			}
		}
		DEBUG("KEY(keysym=0x%lx,char=%c,scan=0x%lx)\n", key, key, scan);
	} else {
		DEBUG("KEY(code=0x%lx)\n", ev->keycode);
		scan = xwin_translate_key2(ev->keycode);
		DEBUG("SCAN(code=0x%lx)\n", scan);
	}

	return (scan);
}

static uint16
xwin_translate_mouse(unsigned long button)
{
	switch (button) {
	case Button1:		/* left */
		return MOUSE_FLAG_BUTTON1;
	case Button2:		/* middle */
		return MOUSE_FLAG_BUTTON3;
	case Button3:		/* right */
		return MOUSE_FLAG_BUTTON2;
	case Button4:
		return MOUSE_FLAG_BUTTON4;	/* wheel up */
	case Button5:
		return MOUSE_FLAG_BUTTON5;	/* wheel down */
	}
	return 0;
}

void
ui_process_events()
{
	XEvent event;
	uint8 scancode;
	uint16 button;
	uint32 ev_time;

	Window tmpwnd;
	int rvt;

	if (display == NULL)
		return;

/*  while (XCheckWindowEvent (display, wnd, ~0, &event))*/
	while (XCheckMaskEvent(display, ~0, &event)) {
		ev_time = time(NULL);

		XGetInputFocus(display, &tmpwnd, &rvt);
		if (tmpwnd == wnd || event.type == Expose)
			switch (event.type) {
			case KeyPress:

				scancode = xwin_translate_key(&(event.xkey));
				if (scancode == 0)
					break;

				if (!(event.xkey.state & ShiftMask)) {
					if ((key_down_state & DShift1Mask)) {
						rdp_send_input(ev_time,
							       RDP_INPUT_SCANCODE,
							       KBD_FLAG_DOWN |
							       KBD_FLAG_UP,
							       0x2a, 0);
						key_down_state &= ~DShift1Mask;
					}

					if ((key_down_state & DShift2Mask)) {
						rdp_send_input(ev_time,
							       RDP_INPUT_SCANCODE,
							       KBD_FLAG_DOWN |
							       KBD_FLAG_UP,
							       0x36, 0);
						key_down_state &= ~DShift2Mask;
					}
				}

				if (!(event.xkey.state & ControlMask)) {
					if ((key_down_state & DControl1Mask)) {
						rdp_send_input(ev_time,
							       RDP_INPUT_SCANCODE,
							       KBD_FLAG_DOWN |
							       KBD_FLAG_UP,
							       0x1d, 0);
						key_down_state &=
						    ~DControl1Mask;
					}
					if ((key_down_state & DControl2Mask)) {
						rdp_send_input(ev_time,
							       RDP_INPUT_SCANCODE,
							       KBD_FLAG_EXT |
							       KBD_FLAG_DOWN |
							       KBD_FLAG_UP,
							       0x9d, 0);
						key_down_state &=
						    ~DControl2Mask;
					}
				}

				if ((key_down_state & DMod1Mask)
				    && !(event.xkey.state & Mod1Mask)) {
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP, 0x38, 0);
					key_down_state &= ~DMod1Mask;
				}

				if ((key_down_state & DMod2Mask)
				    && !(event.xkey.state & Mod2Mask)) {
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP, 0xb8, 0);
					key_down_state &= ~DMod2Mask;
				}

				/* Check caps lock status */
				if ((event.xkey.state & LockMask) !=
				    (key_modifier_state & LockMask)) {
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE, 0,
						       0x3a, 0);
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP, 0x3a, 0);
				}

				/* Check num lock status */
				if ((event.xkey.state & numlock_modifier_mask)
				    !=
				    (key_modifier_state &
				     numlock_modifier_mask)) {
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE, 0,
						       0x45, 0);
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP, 0x45, 0);
				}

				key_modifier_state = event.xkey.state;

				switch (scancode) {
				case 0x2a:
					key_down_state |= DShift1Mask;
					break;
				case 0x36:
					key_down_state |= DShift2Mask;
					break;
				case 0x1d:
					key_down_state |= DControl1Mask;
					break;
				case 0x9d:
					key_down_state |= DControl2Mask;
					break;
				case 0x38:
					key_down_state |= DMod1Mask;
					break;
				case 0xb8:
					key_down_state |= DMod2Mask;
					break;
				}

				if (scancode == 0xff) {
					/* Window keys equivalent to ctrl-esc */
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE, 0,
						       0x1d, 0);	/* ctrl down */
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE, 0, 1, 0);	/* esc down */
					break;
				}

				if (scancode & 0x80)
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_EXT,
						       scancode & 0x7f, 0);
				else if ((scancode != 0x3a && scancode != 0x45)
					 || vncviewer)
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE, 0,
						       scancode, 0);
				break;
			case KeyRelease:
				scancode = xwin_translate_key(&(event.xkey));
				if (scancode == 0)
					break;
				if (scancode == 0xff) {
					/* Window keys equivalent to ctrl-esc */
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP, 0x1, 0);	/* esc up */
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP, 0x1d, 0);	/* ctrl up */
					break;
				}

				switch (scancode) {
				case 0x2a:
					key_down_state &= ~DShift1Mask;
					break;
				case 0x36:
					key_down_state &= ~DShift2Mask;
					break;
				case 0x1d:
					key_down_state &= ~DControl1Mask;
					break;
				case 0x9d:
					key_down_state &= ~DControl2Mask;
					break;
				case 0x38:
					key_down_state &= ~DMod1Mask;
					break;
				case 0xb8:
					key_down_state &= ~DMod2Mask;
					break;
				}

				if (scancode & 0x80)
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_EXT |
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP,
						       scancode & 0x7f, 0);
				else if ((scancode != 0x3a && scancode != 0x45)
					 || vncviewer)	/* Ignore caps lock and num lock */
					rdp_send_input(ev_time,
						       RDP_INPUT_SCANCODE,
						       KBD_FLAG_DOWN |
						       KBD_FLAG_UP, scancode,
						       0);
				break;
			case ButtonPress:
				button =
				    xwin_translate_mouse(event.xbutton.button);

				if (button == 0) {
					STATUS
					    ("pressed unknown mouse button\n");
					break;
				}

				rdp_send_input(ev_time, RDP_INPUT_MOUSE,
					       button | MOUSE_FLAG_DOWN,
					       event.xbutton.x,
					       event.xbutton.y);
				break;
			case ButtonRelease:
				button =
				    xwin_translate_mouse(event.xbutton.button);

				if (button == 0) {
					STATUS
					    ("released unknown mouse button\n");
					break;
				}

				rdp_send_input(ev_time, RDP_INPUT_MOUSE, button,
					       event.xbutton.x,
					       event.xbutton.y);
				break;
			case MotionNotify:
				rdp_send_input(ev_time, RDP_INPUT_MOUSE,
					       MOUSE_FLAG_MOVE, event.xmotion.x,
					       event.xmotion.y);
				break;
			case Expose:
				if (backing_store == BS_PIXMAP) {
					XSetFunction(display, gc, GXcopy);
					XCopyArea(display, back_pixmap, wnd, gc,
						  event.xexpose.x,
						  event.xexpose.y,
						  event.xexpose.width,
						  event.xexpose.height,
						  event.xexpose.x,
						  event.xexpose.y);
				}
				break;
			case EnterNotify:
				if (grab_keyboard)
					ui_grab_keyboard();
				break;
			case LeaveNotify:
				if (grab_keyboard)
					ui_ungrab_keyboard();
				break;
			}
	}
}

void
ui_move_pointer(int x, int y)
{
	XWarpPointer(display, wnd, wnd, 0, 0, 0, 0, x, y);
}

HBITMAP ui_create_bitmap(int width, int height, uint8 * data)
{
	XImage *image;
	Pixmap bitmap;
	uint8 *tdata;
	tdata = (private_colormap ? data : translate(width, height, data));
	bitmap = XCreatePixmap(display, wnd, width, height, depth);
	image =
	    XCreateImage(display, visual,
			 depth, ZPixmap,
			 0, tdata, width, height, BitmapPad(display), 0);

	xwin_set_function(ROP2_COPY);
	XPutImage(display, bitmap, gc, image, 0, 0, 0, 0, width, height);

	XFree(image);
	if (!private_colormap)
		xfree(tdata);
	return (HBITMAP) bitmap;
}

void
ui_paint_bitmap(int x, int y, int cx, int cy, int width, int height,
		uint8 * data)
{
	XImage *image;
	uint8 *tdata =
	    (private_colormap ? data : translate(width, height, data));
	image =
	    XCreateImage(display, visual, depth, ZPixmap, 0, tdata, width,
			 height, BitmapPad(display), 0);

	xwin_set_function(ROP2_COPY);

	/* Window */
	XPutImage(display, wnd, gc, image, 0, 0, x, y, cx, cy);

	/* Pixmap */
	if (backing_store == BS_PIXMAP)
		XPutImage(display, back_pixmap, back_gc, image, 0, 0, x, y, cx,
			  cy);

	XFree(image);
	if (!private_colormap)
		xfree(tdata);
}

void
ui_destroy_bitmap(HBITMAP bmp)
{
	int err = 0;

	if ((err = XFreePixmap(display, (Pixmap) bmp)) != Success) {
		DEBUG("ui_destroy_bitmap(): pixmap free error %d\n", err);
	}
}

HCURSOR
ui_create_cursor(unsigned int x, unsigned int y, int width,
		 int height, uint8 * andmask, uint8 * xormask)
{
	XImage *imagecursor;
	XImage *imagemask;
	Pixmap maskbitmap, cursorbitmap;
	Cursor cursor;
	XColor bg, fg;
	GC lgc;
	int i, x1, y1, scanlinelen;
	uint8 *cdata, *cmask;
	uint8 c;
	cdata = (uint8 *) malloc(sizeof (uint8) * width * height);
	if (!cdata)
		return NULL;
	scanlinelen = (width + 7) >> 3;
	cmask = (uint8 *) malloc(sizeof (uint8) * scanlinelen * height);
	if (!cmask) {
		free(cdata);
		return NULL;
	}
	i = (height - 1) * scanlinelen;

	if (!xserver_be) {
		while (i >= 0) {
			for (x1 = 0; x1 < scanlinelen; x1++) {
				c = *(andmask++);
				cmask[i + x1] = rotate(c);
			}
			i -= scanlinelen;
		}
	} else {
		while (i >= 0) {
			for (x1 = 0; x1 < scanlinelen; x1++) {
				cmask[i + x1] = *(andmask++);
			}
			i -= scanlinelen;
		}
	}

	fg.red = 0;
	fg.blue = 0;
	fg.green = 0;
	fg.flags = DoRed | DoBlue | DoGreen;
	bg.red = 65535;
	bg.blue = 65535;
	bg.green = 65535;
	bg.flags = DoRed | DoBlue | DoGreen;
	maskbitmap = XCreatePixmap(display, wnd, width, height, 1);
	cursorbitmap = XCreatePixmap(display, wnd, width, height, 1);
	lgc = XCreateGC(display, maskbitmap, 0, NULL);
	XSetFunction(display, lgc, GXcopy);
	imagemask =
	    XCreateImage(display, visual, 1, XYBitmap, 0, cmask, width, height,
			 8, 0);
	imagecursor =
	    XCreateImage(display, visual, 1, XYBitmap, 0, cdata, width, height,
			 8, 0);
	for (y1 = height - 1; y1 >= 0; y1--)
		for (x1 = 0; x1 < width; x1++) {
			if (xormask[0] >= 0x80 || xormask[1] >= 0x80
			    || xormask[2] >= 0x80) if (XGetPixel(imagemask, x1,
							      y1)) {
					XPutPixel(imagecursor, x1, y1, 0);
					XPutPixel(imagemask, x1, y1, 0);	/* mask is blank for text cursor! */
				}

				else
					XPutPixel(imagecursor, x1, y1, 1);

			else
				XPutPixel(imagecursor, x1, y1,
					  XGetPixel(imagemask, x1, y1));
			xormask += 3;
		}
	XPutImage(display, maskbitmap, lgc, imagemask, 0, 0, 0, 0, width,
		  height);
	XPutImage(display, cursorbitmap, lgc, imagecursor, 0, 0, 0, 0, width,
		  height); XFree(imagemask);
	XFree(imagecursor);
	free(cmask);
	free(cdata);
	XFreeGC(display, lgc);
	cursor =
	    XCreatePixmapCursor(display, cursorbitmap, maskbitmap, &fg, &bg, x,
				y);
	XFreePixmap(display, maskbitmap);
	XFreePixmap(display, cursorbitmap);
	return (HCURSOR) cursor;
}

void
ui_set_cursor(HCURSOR cursor)
{
	XDefineCursor(display, wnd, (Cursor) cursor);
}

void
ui_destroy_cursor(HCURSOR cursor)
{
	XFreeCursor(display, (Cursor) cursor);
}

HGLYPH ui_create_glyph(int width, int height, uint8 * data)
{
	XImage *image;
	Pixmap bitmap;
	uint8 *rev_data;
	int j, datasize;
	/*uint8 in, out; */

	bitmap = XCreatePixmap(display, wnd, width, height, 1);

	if (!glyph_gc) {	/* only needed to be done once. */
		glyph_gc = XCreateGC(display, bitmap, 0, NULL);
		XSetFunction(display, glyph_gc, GXcopy);
	}

	/* XXX: if little-endian on big-endian screen we turn the bits twice, once in orders.c and once here.. */
#ifdef L_ENDIAN
	if (xserver_be) {
#else				/* bigendian reverse logic */
	if (!xserver_be) {
#endif

		rev_data = xmalloc(width * height);
		datasize = width * height;	/* 8bpp. */
		for (j = 0; j < datasize; j++) {
			rev_data[j] = rotate(data[j]);
		}

		image =
		    XCreateImage(display, visual, 1, ZPixmap, 0, rev_data,
				 width, height, 8, 0);
		xfree(rev_data);

	} else {
		image =
		    XCreateImage(display, visual, 1, ZPixmap, 0, data, width,
				 height, 8, 0);
	}

	XPutImage(display, bitmap, glyph_gc, image, 0, 0, 0, 0, width, height);
	XFree(image);

	return (HGLYPH) bitmap;
}

void
ui_destroy_glyph(HGLYPH glyph)
{
	XFreePixmap(display, (Pixmap) glyph);
}

HCOLOURMAP ui_create_colourmap(COLOURMAP * colours)
{
	if (!private_colormap) {
		COLOURENTRY *entry;
		int i, ncolours = colours->ncolours;
		uint32 *nc = xmalloc(sizeof (*colmap) * ncolours);
		for (i = 0; i < ncolours; i++) {
			XColor xc;
			entry = &colours->colours[i];
			xc.pixel = 0;
			xc.red = entry->red << 8;
			xc.green = entry->green << 8;
			xc.blue = entry->blue << 8;
			if (!XAllocColor(display,
					 DefaultColormap(display,
							 DefaultScreen
							 (display)), &xc)) {
				/* can't allocate a color, so find the nearest existing one */
				long nMinDist = 3 * 256 * 256;
				int j;
				for (j = 0; j < 256; j++) {
					XColor xc2;
					xc2.pixel = j;
					xc2.red = xc2.green = xc2.blue = 0;
					xc2.flags = 0;
					XQueryColor(display,
						    DefaultColormap(display,
								    DefaultScreen
								    (display)),
						    &xc2);
					if (xc2.flags) {
						long nDist =
						    ((long) (xc2.red >> 8) -
						     (long) (xc.red >> 8))
						    * ((long) (xc2.red >> 8) -
						       (long) (xc.red >> 8))
						    + ((long) (xc2.green >> 8) -
						       (long) (xc.green >> 8))
						    * ((long) (xc2.green >> 8) -
						       (long) (xc.green >> 8))
						    + ((long) (xc2.blue >> 8) -
						       (long) (xc.blue >> 8))
						    * ((long) (xc2.blue >> 8) -
						       (long) (xc.blue >> 8));
						if (nDist < nMinDist) {
							nMinDist = nDist;
							xc.pixel = j;
						}
					}
				}
			}
			nc[i] = xc.pixel;
		}
		return nc;
	} else {
		COLOURENTRY *entry;
		XColor *xcolours, *xentry;
		Colormap map;
		int i, ncolours = colours->ncolours;
		xcolours = xmalloc(sizeof (XColor) * ncolours);
		for (i = 0; i < ncolours; i++) {
			entry = &colours->colours[i];
			xentry = &xcolours[i];

			xentry->pixel = i;
			xentry->red = entry->red << 8;
			xentry->blue = entry->blue << 8;
			xentry->green = entry->green << 8;
			xentry->flags = DoRed | DoBlue | DoGreen;
		}

		map = XCreateColormap(display, wnd, visual, AllocAll);
		XStoreColors(display, map, xcolours, ncolours);

		xfree(xcolours);
		return (HCOLOURMAP) map;
	}
}

void
ui_destroy_colourmap(HCOLOURMAP map)
{
	if (!private_colormap)
		xfree(map);
	else
		XFreeColormap(display, (Colormap) map);
}

void
ui_set_colourmap(HCOLOURMAP map)
{

	/* XXX, change values of all pixels on the screen if the new colmap
	 * doesn't have the same values as the old one? */
	if (!private_colormap)
		colmap = map;
	else {
		XSetWindowColormap(display, wnd, (Colormap) map);
		if (fullscreen)
			XInstallColormap(display, (Colormap) map);
	}
}

void
ui_set_clip(int x, int y, int cx, int cy)
{
	XRectangle rect;
	rect.x = x;
	rect.y = y;
	rect.width = cx;
	rect.height = cy;
	XSetClipRectangles(display, gc, 0, 0, &rect, 1, YXBanded);
	if (backing_store == BS_PIXMAP)
		XSetClipRectangles(display, back_gc, 0, 0, &rect, 1, YXBanded);
}

void
ui_reset_clip()
{
	XRectangle rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;
	XSetClipRectangles(display, gc, 0, 0, &rect, 1, YXBanded);
	if (backing_store == BS_PIXMAP)
		XSetClipRectangles(display, back_gc, 0, 0, &rect, 1, YXBanded);
}

void
ui_bell()
{
	if (systembeep)
		XBell(display, 0);
}

uint16 ui_get_toggle_keys()
{
	uint16 keys = 0;
	/* XXX  */
	if (key_modifier_state & LockMask)
		keys |= KBD_FLAG_CAPITAL;
	if (key_modifier_state & numlock_modifier_mask)
		keys |= KBD_FLAG_NUMLOCK;

/* if (GetKeyState(VK_SCROLL) & 1)
  keys |= KBD_FLAG_SCROLL;
*/
	return keys;
}

static int rop2_map[] = {
	GXclear,		/* 0 */
	GXnor,			/* DPon */
	GXandInverted,		/* DPna */
	GXcopyInverted,		/* Pn */
	GXandReverse,		/* PDna */
	GXinvert,		/* Dn */
	GXxor,			/* DPx */
	GXnand,			/* DPan */
	GXand,			/* DPa */
	GXequiv,		/* DPxn */
	GXnoop,			/* D */
	GXorInverted,		/* DPno */
	GXcopy,			/* P */
	GXorReverse,		/* PDno */
	GXor,			/* DPo */
	GXset			/* 1 */
};

inline static void
xwin_set_function(uint8 rop2)
{
	if (last_main_gc_func == rop2)
		return;
	last_main_gc_func = rop2;
	XSetFunction(display, gc, rop2_map[rop2]);
	if (backing_store == BS_PIXMAP)
		XSetFunction(display, back_gc, rop2_map[rop2]);
}

void
ui_destblt(uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy)
{
	xwin_set_function(opcode);
	if (!private_colormap) {
		/* without a private colormap, we can't simply clear/set all bits;
		 * we must change them to black/white */
		if (opcode == 0 && Ctrans(0) != 0) {
			xwin_set_function(ROP2_COPY);
			XSetForeground(display, gc, Ctrans(0));	/* black */
		} else if (opcode == 0x0f && Ctrans(0xff) != 0xff) {
			xwin_set_function(ROP2_COPY);
			XSetForeground(display, gc, Ctrans(0xff));	/* white */
		}
	}
	XFillRectangle(display, wnd, gc, x, y, cx, cy);
	if (backing_store == BS_PIXMAP) {
		if (!private_colormap) {
			if (opcode == 0 && Ctrans(0) != 0) {
				XSetForeground(display, back_gc, Ctrans(0));
			} else if (opcode == 0x0f && Ctrans(0xff) != 0xff) {
				XSetForeground(display, back_gc, Ctrans(0xff));
			}
		}
		XFillRectangle(display, back_pixmap, back_gc, x, y, cx, cy);
	}
}

void
ui_patblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	Display *dpy = display;
	Pixmap fill;
	uint8 i, ipattern[8];
	xwin_set_function(opcode);
	switch (brush->style) {
	case 0:		/* Solid */
		XSetForeground(dpy, gc, Ctrans(fgcolour));
		XFillRectangle(dpy, wnd, gc, x, y, cx, cy);

		if (backing_store == BS_PIXMAP) {
			XSetForeground(dpy, back_gc, Ctrans(fgcolour));
			XFillRectangle(dpy, back_pixmap, back_gc, x, y, cx, cy);
		}
		break;
	case 3:		/* Pattern */
		for (i = 0; i != 8; i++)
			ipattern[i] = ~brush->pattern[i];
		fill = (Pixmap) ui_create_glyph(8, 8, ipattern);

		XSetForeground(dpy, gc, Ctrans(fgcolour));
		XSetBackground(dpy, gc, Ctrans(bgcolour));
		XSetFillStyle(dpy, gc, FillOpaqueStippled);
		XSetTSOrigin(display, gc, brush->xorigin, brush->yorigin);
		XSetStipple(dpy, gc, fill);
		XFillRectangle(dpy, wnd, gc, x, y, cx, cy);
		XSetFillStyle(dpy, gc, FillSolid);

		if (backing_store == BS_PIXMAP) {
			XSetForeground(dpy, back_gc, Ctrans(fgcolour));
			XSetBackground(dpy, back_gc, Ctrans(bgcolour));
			XSetFillStyle(dpy, back_gc, FillOpaqueStippled);
			XSetTSOrigin(display, back_gc, brush->xorigin,
				     brush->yorigin);
			XSetStipple(dpy, back_gc, fill);
			XFillRectangle(dpy, back_pixmap, back_gc, x, y, cx, cy);
			XSetFillStyle(dpy, back_gc, FillSolid);
		}

		ui_destroy_glyph((HGLYPH) fill);
		break;
	default:
		unimpl("brush %d\n", brush->style);
	}
}

void
ui_screenblt(uint8 opcode,
	     /* dest */ int x, int y, int cx, int cy,
	     /* src */ int srcx, int srcy)
{
	xwin_set_function(opcode);
	XCopyArea(display, wnd, wnd, gc, srcx, srcy, cx, cy, x, y);
	if (backing_store == BS_PIXMAP)
		XCopyArea(display, back_pixmap, back_pixmap, back_gc, srcx,
			  srcy, cx, cy, x, y);
}

void
ui_memblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, HCOLOURMAP map, int srcx, int srcy)
{
	/* FIXME: use the map!!! */
	if( colmap != map )
		ui_set_colourmap( map );

	xwin_set_function(opcode);
	XCopyArea(display, (Pixmap) src, wnd, gc, srcx, srcy, cx, cy, x, y);
	if (backing_store == BS_PIXMAP)
		XCopyArea(display, (Pixmap) src, back_pixmap, back_gc, srcx,
			  srcy, cx, cy, x, y);
}

void
ui_triblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, HCOLOURMAP map, int srcx, int srcy,
	  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	/* FIXME: use the map!!!*/
	if( colmap != map )
 		ui_set_colourmap( map );

	/* This is potentially difficult to do in general. Until someone
	   comes up with a more efficient way of doing it I am using cases. */
	switch (opcode) {
	case 0x69:		/* PDSxxn */
		ui_memblt(ROP2_XOR, x, y, cx, cy, src, 0, srcx, srcy);
		ui_patblt(ROP2_NXOR, x, y, cx, cy, brush, bgcolour, fgcolour);
		break;
	case 0xb8:		/* PSDPxax */
		ui_patblt(ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
		ui_memblt(ROP2_AND, x, y, cx, cy, src, 0, srcx, srcy);
		ui_patblt(ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
		break;
	case 0xc0:
		ui_memblt(ROP2_COPY, x, y, cx, cy, src, 0, srcx, srcy);
		ui_patblt(ROP2_AND, x, y, cx, cy, brush, bgcolour, fgcolour);
		break;
	default:
		unimpl("triblt 1x%x\n", opcode);
		ui_memblt(ROP2_COPY, x, y, cx, cy, src, 0, srcx, srcy);
	}
}
void
ui_line(uint8 opcode,
	/* dest */ int startx, int starty, int endx, int endy,
	/* pen */ PEN * pen)
{
	xwin_set_function(opcode);

	XSetForeground(display, gc, Ctrans(pen->colour));
	XDrawLine(display, wnd, gc, startx, starty, endx, endy);

	if (backing_store == BS_PIXMAP) {
		XSetForeground(display, back_gc, Ctrans(pen->colour));
		XDrawLine(display, back_pixmap, back_gc, startx, starty, endx,
			  endy);
	}
}

void
ui_rect(
	       /* dest */ int x, int y, int cx, int cy,
	       /* brush */ int colour)
{
	xwin_set_function(ROP2_COPY);

	XSetForeground(display, gc, Ctrans(colour));
	XFillRectangle(display, wnd, gc, x, y, cx, cy);

	if (backing_store == BS_PIXMAP) {
		XSetForeground(display, back_gc, Ctrans(colour));
		XFillRectangle(display, back_pixmap, back_gc, x, y, cx, cy);
	}
}

void
ui_draw_glyph(int mixmode,
	      /* dest */ int x, int y, int cx, int cy,
	      /* src */ HGLYPH glyph, int srcx, int srcy,
	      int bgcolour, int fgcolour, HBITMAP dst)
{
	Pixmap pixmap = (Pixmap) glyph;
	xwin_set_function(ROP2_COPY);
	XSetForeground(display, gc, Ctrans(fgcolour));
	switch (mixmode) {
	case MIX_TRANSPARENT:
		XSetStipple(display, gc, pixmap);
		XSetFillStyle(display, gc, FillStippled);
		XSetTSOrigin(display, gc, x, y);
		XFillRectangle(display, (Pixmap) dst, gc, x, y, cx, cy);
		XSetFillStyle(display, gc, FillSolid);
		XSetTSOrigin(display, gc, 0, 0);
		break;
	case MIX_OPAQUE:
		XSetBackground(display, gc, Ctrans(bgcolour));
/*      XCopyPlane (display, pixmap, back_pixmap, back_gc, srcx, srcy, cx, cy, x, y, 1); */
		XSetStipple(display, gc, pixmap);
		XSetFillStyle(display, gc, FillOpaqueStippled);
		XSetTSOrigin(display, gc, x, y);
		XFillRectangle(display, (Pixmap) dst, gc, x, y, cx, cy);
		XSetFillStyle(display, gc, FillSolid);
		XSetTSOrigin(display, gc, 0, 0);
		break;
	default:
		unimpl("mix %d\n", mixmode);
	}
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
                     (HBITMAP) pixmap);\
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
	Pixmap pixmap;
	DATABLOB *entry;

	xwin_set_function(ROP2_COPY);
	XSetForeground(display, gc, Ctrans(bgcolour));

	if (backing_store == BS_PIXMAP)
		pixmap = back_pixmap;
	else
		pixmap = wnd;

	if (boxcx > 1)
		XFillRectangle(display, pixmap, gc, boxx, boxy, boxcx, boxcy);
	else if (mixmode == MIX_OPAQUE)
		XFillRectangle(display, pixmap, gc, clipx, clipy, clipcx,
			       clipcy);

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

	if (backing_store == BS_PIXMAP) {
		if (boxcx > 1)
			XCopyArea(display, back_pixmap, wnd, back_gc, boxx,
				  boxy, boxcx, boxcy, boxx, boxy);
		else
			XCopyArea(display, back_pixmap, wnd, back_gc, clipx,
				  clipy, clipcx, clipcy, clipx, clipy);
	}
}

void
ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
	Pixmap pix;
	XImage *image;
	DEBUG("desktop save\n");

	pix = XCreatePixmap(display, wnd, cx, cy, depth);
	xwin_set_function(ROP2_COPY);

	if (backing_store == BS_PIXMAP)
		XCopyArea(display, back_pixmap, pix, back_gc, x, y, cx, cy, 0,
			  0);
	else
		XCopyArea(display, wnd, pix, gc, x, y, cx, cy, 0, 0);

	image = XGetImage(display, pix, 0, 0, cx, cy, AllPlanes, ZPixmap);

	offset *= TOBYTES(bpp);
	cache_put_desktop(offset, cx, cy, image->bytes_per_line,
			  TOBYTES(bpp) /*image->bytes_per_line/cx */ ,
			  image->data);

	XDestroyImage(image);
	XFreePixmap(display, pix);
}

void
ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
	XImage *image;
	uint8 *data;
	DEBUG("desktop restore\n");

	offset *= TOBYTES(bpp);
	data = cache_get_desktop(offset, cx, cy, TOBYTES(bpp));
	if (data == NULL)
		return;
	image =
	    XCreateImage(display, visual,
			 depth, ZPixmap,
			 0, data, cx, cy, BitmapPad(display),
			 cx * TOBYTES(bpp));
	xwin_set_function(ROP2_COPY);
	XPutImage(display, wnd, gc, image, 0, 0, x, y, cx, cy);

	if (backing_store == BS_PIXMAP)
		XPutImage(display, back_pixmap, back_gc, image, 0, 0, x, y, cx,
			  cy);

	XFree(image);
}

void
ui_sync()
{
	if (display != NULL)
		XFlush(display);
}

BOOL ui_pending()
{
	return (XEventsQueued(display, QueuedAlready) > 0);
}

void
ui_grab_keyboard()
{
	int err =
	    XGrabKeyboard(display, wnd, True, GrabModeAsync, GrabModeAsync,
			  CurrentTime);
	if (err != GrabSuccess && err != AlreadyGrabbed)
		STATUS("keyboard grab failed, err=%d\n", err);
}

void
ui_ungrab_keyboard()
{
	XUngrabKeyboard(display, CurrentTime);
}

/* unroll defines, used to make the loops a bit more readable... */
#define unroll8Expr(uexp) uexp uexp uexp uexp uexp uexp uexp uexp
#define unroll8Lefts(uexp) case 7: uexp \
	case 6: uexp \
	case 5: uexp \
	case 4: uexp \
	case 3: uexp \
	case 2: uexp \
	case 1: uexp

static uint8 *
translate(int width, int height, uint8 * data)
{
	uint32 i;
	uint32 size = width * height;
	uint8 *d2 = xmalloc(size * TOBYTES(bpp));
	uint8 *d3 = d2;
	uint32 pix;
	i = (size & ~0x7);

	/* XXX: where are the bits swapped??? */
#ifdef L_ENDIAN			/* little-endian */
	/* big-endian screen */
	if (xserver_be) {
		switch (bpp) {
		case 32:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix >> 24;
					    *d3++ = pix >> 16; *d3++ = pix >> 8;
					    *d3++ = pix;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix >> 24;
						     *d3++ = pix >> 16;
						     *d3++ = pix >> 8;
						     *d3++ = pix;)}
			break;
		case 24:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix >> 16;
					    *d3++ = pix >> 8; *d3++ = pix;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix >> 16;
						     *d3++ = pix >> 8;
						     *d3++ = pix;)}
			break;
		case 16:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix >> 8; *d3++ = pix;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix >> 8;
						     *d3++ = pix;)}
			break;
		case 8:
			while (i) {
				unroll8Expr(pix = colmap[*data++]; *d3++ = pix;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;)
				}
			break;
		}
	} else {		/* little-endian screen */
		switch (bpp) {
		case 32:
			while (i) {
				unroll8Expr(*((uint32 *) d3) = colmap[*data++];
					    d3 += sizeof (uint32);)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(*((uint32 *) d3) =
						     colmap[*data++];
						     d3 += sizeof (uint32);)
				}
			break;
		case 24:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix; *d3++ = pix >> 8;
					    *d3++ = pix >> 16;) i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;
						     *d3++ = pix >> 8;
						     *d3++ = pix >> 16;)
				}
			break;
		case 16:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix; *d3++ = pix >> 8;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;
						     *d3++ = pix >> 8;)}
			break;
		case 8:
			while (i) {
				unroll8Expr(pix = colmap[*data++]; *d3++ = pix;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;)
				}
		}
	}

#else				/* bigendian-compiled */
	if (xserver_be) {
		/* big-endian screen */
		switch (bpp) {
		case 32:
			while (i) {
				unroll8Expr(*((uint32 *) d3) = colmap[*data++];
					    d3 += sizeof (uint32);)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(*((uint32 *) d3) =
						     colmap[*data++];
						     d3 += sizeof (uint32);)
				}
			break;
		case 24:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix; *d3++ = pix >> 8;
					    *d3++ = pix >> 16;) i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;
						     *d3++ = pix >> 8;
						     *d3++ = pix >> 16;)
				}
			break;
		case 16:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix; *d3++ = pix >> 8;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;
						     *d3++ = pix >> 8;)}
			break;
		case 8:
			while (i) {
				unroll8Expr(pix = colmap[*data++]; *d3++ = pix;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;)
				}
		}
	} else {
		/* little-endian screen */
		switch (bpp) {
		case 32:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix;
					    *d3++ = pix >> 8;
					    *d3++ = pix >> 16;
					    *d3++ = pix >> 24;) i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;
						     *d3++ = pix >> 8;
						     *d3++ = pix >> 16;
						     *d3++ = pix >> 24;)

				}
			break;
		case 24:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix; *d3++ = pix >> 8;
					    *d3++ = pix >> 16;) i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;
						     *d3++ = pix >> 8;
						     *d3++ = pix >> 16;)
				}
			break;
		case 16:
			while (i) {
				unroll8Expr(pix = colmap[*data++];
					    *d3++ = pix; *d3++ = pix >> 8;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;
						     *d3++ = pix >> 8;)}
			break;
		case 8:
			while (i) {
				unroll8Expr(pix = colmap[*data++]; *d3++ = pix;)
				    i -= 8;
			}
			i = (size & 0x7);
			if (i != 0)
				switch (i) {
					unroll8Lefts(pix = colmap[*data++];
						     *d3++ = pix;)
				}
		}
	}
#endif

	return d2;
}

#ifdef SERVER

#include <X11/keysym.h>

x_key_to_sym(key, down)
int key;
int down;
{
	unsigned int code;
	static int shiftstate = 0;
	static int controlstate = 0;

	if (display == NULL) {
		display = XOpenDisplay(NULL);
		printf("bitmappad: %d\n", BitmapPad(display));
	}

	code = XKeycodeToKeysym(display, key + 8, shiftstate);

	if (code == XK_Shift_L || code == XK_Shift_R){
		if (down)
			shiftstate |= 1;
		else
			shiftstate &= ~1;
	}
	if (code == XK_Control_L || code == XK_Control_R){
		if (down)
			controlstate |= 1;
		else
			controlstate &= ~1;
	}
	if (controlstate) {
		if (code >= 0x40 && code <= 0x7f)
			code &= 0x1f;
		else if (code == 0x20 || code == 0x32)
			code = 0x00;
		else if (code == 0x5e || code == 0x36)
			code = 0x1e;
		else if (code == 0x5f || code == '-')
			code = 0x1f;
		else if (code == '0')
//      exit(0);
			rdp_disconnect();
	}
	return (code);
}

#endif				/* SERVER */
