 /*
  * UAE - The Un*x Amiga Emulator
  *
  * glgfx interface
  *
  * Copyright 2005 Martin Blom
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <ctype.h>

#include "config.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "xwin.h"
#include "custom.h"
#include "drawing.h"
#include "newcpu.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include "debug.h"
#include "picasso96.h"
#include "inputdevice.h"
#include "hotkeys.h"

/*
 * Default hotkeys
 *
 * We need a better way of doing this. ;-)
 */
static struct uae_hotkeyseq default_hotkeys[] =
{
    { MAKE_HOTKEYSEQ (XK_F12, XK_q, -1, -1,           INPUTEVENT_SPC_QUIT) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_r, -1, -1,           INPUTEVENT_SPC_WARM_RESET) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_Shift_L, XK_r, -1,   INPUTEVENT_SPC_COLD_RESET) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_d, -1, -1,           INPUTEVENT_SPC_ENTERDEBUGGER) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_s, -1, -1,           INPUTEVENT_SPC_TOGGLEFULLSCREEN) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_m, -1, -1,           INPUTEVENT_SPC_TOGGLEMOUSEMODE) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_g, -1, -1,           INPUTEVENT_SPC_TOGGLEMOUSEGRAB) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_i, -1, -1,           INPUTEVENT_SPC_INHIBITSCREEN) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_p, -1, -1,           INPUTEVENT_SPC_SCREENSHOT) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_a, -1, -1,           INPUTEVENT_SPC_SWITCHINTERPOL) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_KP_Add, -1, -1,      INPUTEVENT_SPC_INCRFRAMERATE) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_KP_Subtract, -1, -1, INPUTEVENT_SPC_DECRFRAMERATE) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_F1, -1, -1,	      INPUTEVENT_SPC_FLOPPY0) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_F2, -1, -1,	      INPUTEVENT_SPC_FLOPPY1) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_F3, -1, -1,	      INPUTEVENT_SPC_FLOPPY2) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_F4, -1, -1,	      INPUTEVENT_SPC_FLOPPY3) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_Shift_L, XK_F1, -1,  INPUTEVENT_SPC_EFLOPPY0) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_Shift_L, XK_F2, -1,  INPUTEVENT_SPC_EFLOPPY1) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_Shift_L, XK_F3, -1,  INPUTEVENT_SPC_EFLOPPY2) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_Shift_L, XK_F4, -1,  INPUTEVENT_SPC_EFLOPPY3) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_Return, -1, -1,      INPUTEVENT_SPC_ENTERGUI) },
    { MAKE_HOTKEYSEQ (XK_F12, XK_f, -1, -1,           INPUTEVENT_SPC_FREEZEBUTTON) },
    { HOTKEYS_END }
};


static int screen_is_picasso;
static char picasso_invalid_lines[1201];
static int picasso_has_invalid_lines;
static int picasso_invalid_start, picasso_invalid_stop;
static int picasso_maxw = 0, picasso_maxh = 0;

static struct disp_info ami_dinfo, pic_dinfo;
static Visual *vis;
static XVisualInfo visualInfo;
static int bitdepth, bit_unit;
static Cursor blankCursor, xhairCursor;
static int cursorOn;
static int inverse_byte_order = 0;

static int current_width, current_height;

static int x11_init_ok;
static int dgaavail = 0, vidmodeavail = 0, shmavail = 0;
static int dgamode;
static int grabbed;

void toggle_mousegrab (void);
void framerate_up (void);
void framerate_down (void);
int xkeysym2amiga (int);

int pause_emulation;


void flush_line (int y)
{
    char *linebuf = gfxvidinfo.linemem;
    int xs, xe;
    int len;

    if (linebuf == NULL)
	linebuf = y*gfxvidinfo.rowbytes + gfxvidinfo.bufmem;

#ifdef USE_DGA_EXTENSION
    if (dgamode && need_dither) {
	DitherLine ((unsigned char *)(fb_addr + fb_width*y),
		    (uae_u16 *)linebuf, 0, y, gfxvidinfo.width, bit_unit);
	return;
    }
#endif
    xs = 0;
    xe = gfxvidinfo.width - 1;

    if (currprefs.x11_use_low_bandwidth) {
	char *src, *dst;
	switch (gfxvidinfo.pixbytes) {
	 case 4:
	    {
		uae_u32 *newp = (uae_u32 *)linebuf;
		uae_u32 *oldp = (uae_u32 *)((uae_u8 *)ami_dinfo.image_mem + y*ami_dinfo.ximg->bytes_per_line);
		while (newp[xs] == oldp[xs]) {
		    if (xs == xe)
			return;
		    xs++;
		}
		while (newp[xe] == oldp[xe]) xe--;

		dst = (char *)(oldp + xs); src = (char *)(newp + xs);
	    }
	    break;
	 case 2:
	    {
		uae_u16 *newp = (uae_u16 *)linebuf;
		uae_u16 *oldp = (uae_u16 *)((uae_u8 *)ami_dinfo.image_mem + y*ami_dinfo.ximg->bytes_per_line);
		while (newp[xs] == oldp[xs]) {
		    if (xs == xe)
			return;
		    xs++;
		}
		while (newp[xe] == oldp[xe]) xe--;

		dst = (char *)(oldp + xs); src = (char *)(newp + xs);
	    }
	    break;
	 case 1:
	    {
		uae_u8 *newp = (uae_u8 *)linebuf;
		uae_u8 *oldp = (uae_u8 *)((uae_u8 *)ami_dinfo.image_mem + y*ami_dinfo.ximg->bytes_per_line);
		while (newp[xs] == oldp[xs]) {
		    if (xs == xe)
			return;
		    xs++;
		}
		while (newp[xe] == oldp[xe]) xe--;

		dst = (char *)(oldp + xs); src = (char *)(newp + xs);
	    }
	    break;

	 default:
	    abort ();
	    break;
	}

	len = xe - xs + 1;
	memcpy (dst, src, len * gfxvidinfo.pixbytes);
    } else if (need_dither) {
	uae_u8 *target = (uae_u8 *)ami_dinfo.image_mem + ami_dinfo.ximg->bytes_per_line * y;
	len = currprefs.gfx_width_win;
	DitherLine (target, (uae_u16 *)linebuf, 0, y, gfxvidinfo.width, bit_unit);
    } else {
	write_log ("Bug!\n");
	abort();
    }

    DO_PUTIMAGE (ami_dinfo.ximg, xs, y, xs, y, len, 1);
}

void flush_block (int ystart, int ystop) {
}

void flush_screen (int ystart, int ystop) {
}

void flush_clear_screen (void) {
}


static unsigned long pixel_return[256];
static XColor parsed_xcolors[256];
static int ncolors = 0;

static int blackval = 32767;
static int whiteval = 0;



int graphics_setup (void) {
    return 1;
}



static void graphics_subinit (void)
{
    int i, j;
    XSetWindowAttributes wattr;
    XClassHint classhint;
    XWMHints *hints;
    unsigned long valuemask;

    dgamode = screen_is_picasso ? currprefs.gfx_pfullscreen : currprefs.gfx_afullscreen;
    dgamode = dgamode && dgaavail;

    wattr.background_pixel = /*black.pixel*/0;
    wattr.backing_store = Always;
    wattr.backing_planes = bitdepth;
    wattr.border_pixmap = None;
    wattr.border_pixel = /*black.pixel*/0;
    wattr.colormap = cmap;
    valuemask = (CWEventMask | CWBackPixel | CWBorderPixel
		 | CWBackingStore | CWBackingPlanes | CWColormap);

    if (dgamode) {
	wattr.event_mask = DGA_EVENTMASK;
	wattr.override_redirect = 1;
	valuemask |= CWOverrideRedirect;
    } else
	wattr.event_mask = EVENTMASK;

    XSync (display, 0);

    delete_win = XInternAtom(display, "WM_DELETE_WINDOW", False);
    mywin = XCreateWindow (display, rootwin, 0, 0, current_width, current_height,
			   0, bitdepth, InputOutput, vis, valuemask, &wattr);
    XSetWMProtocols (display, mywin, &delete_win, 1);
    XSync (display, 0);
    XStoreName (display, mywin, PACKAGE_NAME);
    XSetIconName (display, mywin, "UAE Screen");

    /* set class hint */
    classhint.res_name = "UAE";
    classhint.res_class = "UAEScreen";
    XSetClassHint(display, mywin, &classhint);

    hints = XAllocWMHints();
    /* Set window group leader to self to become an application
     * that can be hidden by e.g. WindowMaker.
     * Would be more useful if we could find out what the
     * (optional) GTK+ window ID is :-/ */
    hints->window_group = mywin;
    hints->flags = WindowGroupHint;
    XSetWMHints(display, mywin, hints);

    XMapRaised (display, mywin);
    XSync (display, 0);
    mygc = XCreateGC (display, mywin, 0, 0);

    if (dgamode) {
#ifdef USE_DGA_EXTENSION
	enter_dga_mode ();
	/*setuid(getuid());*/
	picasso_vidinfo.rowbytes = fb_width * picasso_vidinfo.pixbytes;
#endif
    } else {
	get_image (current_width, current_height, &ami_dinfo);
	if (screen_is_picasso) {
	    get_image (current_width, current_height, &pic_dinfo);
	    picasso_vidinfo.rowbytes = pic_dinfo.ximg->bytes_per_line;
	}
    }

    picasso_vidinfo.extra_mem = 1;

    if (need_dither) {
	gfxvidinfo.maxblocklines = 0;
	gfxvidinfo.rowbytes = gfxvidinfo.pixbytes * currprefs.gfx_width_win;
	gfxvidinfo.linemem = (char *)malloc (gfxvidinfo.rowbytes);
    } else if (! dgamode) {
	gfxvidinfo.emergmem = 0;
	gfxvidinfo.linemem = 0;
	gfxvidinfo.bufmem = ami_dinfo.image_mem;
	gfxvidinfo.rowbytes = ami_dinfo.ximg->bytes_per_line;
	if (currprefs.x11_use_low_bandwidth) {
	    gfxvidinfo.maxblocklines = 0;
	    gfxvidinfo.rowbytes = ami_dinfo.ximg->bytes_per_line;
	    gfxvidinfo.linemem = (char *)malloc (gfxvidinfo.rowbytes);
	} else {
	    gfxvidinfo.maxblocklines = 100; /* whatever... */
	}
    }

    if (visualInfo.VI_CLASS != TrueColor && ! screen_is_picasso) {
	int i;
	for (i = 0; i < 256; i++)
	    XStoreColor (display, cmap, parsed_xcolors + i);
    }

#ifdef USE_DGA_EXTENSION
    if (dgamode) {
	dga_colormap_installed = 0;
	XF86DGAInstallColormap (display, screen, cmap2);
	XF86DGAInstallColormap (display, screen, cmap);
    }
#endif

    if (! dgamode) {
	if (! currprefs.x11_hide_cursor)
	    XDefineCursor (display, mywin, xhairCursor);
	else
	    XDefineCursor (display, mywin, blankCursor);
	cursorOn = 1;
    }

    if (screen_is_picasso) {
	picasso_has_invalid_lines = 0;
	picasso_invalid_start = picasso_vidinfo.height + 1;
	picasso_invalid_stop = -1;
	memset (picasso_invalid_lines, 0, sizeof picasso_invalid_lines);
    }

    inwindow = 0;
    inputdevice_release_all_keys ();
    reset_hotkeys ();
}



int graphics_init (void)
{
    int i,j;
    XPixmapFormatValues *xpfvs;

    if (currprefs.x11_use_mitshm && ! shmavail) {
	write_log ("MIT-SHM extension not supported by X server.\n");
    }
    if (currprefs.color_mode > 5)
	write_log ("Bad color mode selected. Using default.\n"), currprefs.color_mode = 0;

    x11_init_ok = 0;
    need_dither = 0;
    screen_is_picasso = 0;
    dgamode = 0;

    init_dispinfo (&ami_dinfo);
    init_dispinfo (&pic_dinfo);

    screen = XDefaultScreen (display);
    rootwin = XRootWindow (display, screen);

    if (!get_best_visual (&visualInfo)) return 0;

    vis = visualInfo.visual;
    bitdepth = visualInfo.depth;

    if (!(bit_unit = get_visual_bit_unit (&visualInfo, bitdepth))) return 0;

    write_log ("Using %d bit visual, %d bits per pixel\n", bitdepth, bit_unit);

    fixup_prefs_dimensions (&currprefs);

    gfxvidinfo.width = currprefs.gfx_width_win;
    gfxvidinfo.height = currprefs.gfx_height_win;
    current_width = currprefs.gfx_width_win;
    current_height = currprefs.gfx_height_win;

    cmap = XCreateColormap (display, rootwin, vis, AllocNone);
    cmap2 = XCreateColormap (display, rootwin, vis, AllocNone);
    if (visualInfo.VI_CLASS == GrayScale || visualInfo.VI_CLASS == PseudoColor) {
	XAllocColorCells (display, cmap, 0, 0, 0, pixel_return, 1 << bitdepth);
	XAllocColorCells (display, cmap2, 0, 0, 0, pixel_return, 1 << bitdepth);
    }

    if (bitdepth < 8 || (bitdepth == 8 && currprefs.color_mode == 3)) {
	gfxvidinfo.pixbytes = 2;
	currprefs.x11_use_low_bandwidth = 0;
	need_dither = 1;
    } else {
	gfxvidinfo.pixbytes = bit_unit >> 3;
    }

    if (! init_colors ())
	return 0;

    blankCursor = XCreatePixmapCursor (display,
				       XCreatePixmap (display, rootwin, 1, 1, 1),
				       XCreatePixmap (display, rootwin, 1, 1, 1),
				       &black, &white, 0, 0);
    xhairCursor = XCreateFontCursor (display, XC_crosshair);

    graphics_subinit ();

    grabbed = 0;

    return x11_init_ok = 1;
}


static void graphics_subshutdown (void)
{
    XSync (display, 0);
#ifdef USE_DGA_EXTENSION
    if (dgamode)
	leave_dga_mode ();
#endif

    destroy_dinfo (&ami_dinfo);
    destroy_dinfo (&pic_dinfo);

    if (mywin) {
	XDestroyWindow (display, mywin);
	mywin = 0;
    }

    if (gfxvidinfo.linemem != NULL)
	free (gfxvidinfo.linemem);
    if (gfxvidinfo.emergmem != NULL)
	free (gfxvidinfo.emergmem);
}

void graphics_leave (void) {

  graphics_subshutdown ();

  dumpcustom ();
}

static struct timeval lastMotionTime;

static int refresh_necessary = 0;

void handle_events (void)
{
    gui_handle_events ();

    for (;;) {
	XEvent event;
#if 0
	if (! XCheckMaskEvent (display, eventmask, &event))
	    break;
#endif
	if (! XPending (display))
	    break;

	XNextEvent (display, &event);

	switch (event.type) {
	 case KeyPress:
	 case KeyRelease: {
	    int state = (event.type == KeyPress);
	    KeySym keysym;
	    int index = 0;
	    int ievent, amiga_keycode;
	    do {
		keysym = XLookupKeysym ((XKeyEvent *)&event, index);
		if ((ievent = match_hotkey_sequence (keysym, state))) {
		    handle_hotkey_event (ievent, state);
		    break;
		} else
		    if ((amiga_keycode = xkeysym2amiga (keysym)) >= 0) {
			inputdevice_do_keyboard (amiga_keycode, state);
			break;
		    }
		index++;
	    } while (keysym != NoSymbol);
	    break;
	 }
	 case ButtonPress:
	 case ButtonRelease: {
	    int state = (event.type == ButtonPress);
	    int buttonno = -1;
	    switch ((int)((XButtonEvent *)&event)->button) {
		case 1:  buttonno = 0; break;
		case 2:  buttonno = 2; break;
		case 3:  buttonno = 1; break;
		/* buttons 4 and 5 report mousewheel events */
		case 4:  if (state) record_key (0x7a << 1); break;
		case 5:  if (state) record_key (0x7b << 1); break;
	    }
            if (buttonno >=0)
		setmousebuttonstate(0, buttonno, state);
	    break;
	 }
	 case MotionNotify:
	    if (dgamode) {
		int tx = ((XMotionEvent *)&event)->x_root;
		int ty = ((XMotionEvent *)&event)->y_root;
		printf("DGA: tx: %d; ty: %d\n", tx, ty);
		setmousestate (0, 0, tx, 0);
		setmousestate (0, 1, ty, 0);
	    } else if (grabbed) {
		int realmove = 0;
		int tx, ty,ttx,tty;

		tx = ((XMotionEvent *)&event)->x;
		ty = ((XMotionEvent *)&event)->y;

		printf("grabbed: tx: %d; ty: %d\n", tx, ty);
		if (! event.xmotion.send_event) {
		    setmousestate( 0,0,tx-oldx,0);
		    setmousestate( 0,1,ty-oldy,0);
		    realmove = 1;
#undef ABS
#define ABS(a) (((a)<0) ? -(a) : (a) )
		    if (ABS(current_width / 2 - tx) > 3 * current_width / 8
			|| ABS(current_height / 2 - ty) > 3 * current_height / 8)
		    {
#undef ABS
			XEvent event;
			ttx = current_width / 2;
			tty = current_height / 2;
			event.type = MotionNotify;
			event.xmotion.display = display;
			event.xmotion.window = mywin;
			event.xmotion.x = ttx;
			event.xmotion.y = tty;
			XSendEvent (display, mywin, False,
				    PointerMotionMask, &event);
			XWarpPointer (display, None, mywin, 0, 0, 0, 0, ttx, tty);
		    }
		} else {
		    tx=event.xmotion.x;
		    ty=event.xmotion.y;
		printf("grabbed nosend: tx: %d; ty: %d\n", tx, ty);
		}
		oldx = tx;
		oldy = ty;
	    } else if (inwindow) {
		int tx = ((XMotionEvent *)&event)->x;
		int ty = ((XMotionEvent *)&event)->y;
		printf("ungrabbed nosend: tx: %d; ty: %d\n", tx, ty);
		setmousestate(0,0,tx,1);
		setmousestate(0,1,ty,1);
		if (! cursorOn && !currprefs.x11_hide_cursor) {
		    XDefineCursor(display, mywin, xhairCursor);
		    cursorOn = 1;
		}
		gettimeofday(&lastMotionTime, NULL);
	    }
	    break;
	 case EnterNotify:
	    {
		int tx = ((XCrossingEvent *)&event)->x;
		int ty = ((XCrossingEvent *)&event)->y;
		setmousestate(0,0,tx,1);
		setmousestate(0,1,ty,1);
	    }
	    inwindow = 1;
	    break;
	 case LeaveNotify:
	    inwindow = 0;
	    break;
	 case FocusIn:
	    if (! autorepeatoff)
		XAutoRepeatOff (display);
	    autorepeatoff = 1;
	    break;
	 case FocusOut:
	    if (autorepeatoff)
		XAutoRepeatOn (display);
	    autorepeatoff = 0;
	    break;
	 case Expose:
	    refresh_necessary = 1;
	    break;
         case ClientMessage:
            if (((Atom)event.xclient.data.l[0]) == delete_win) {
		uae_quit ();
            }
            break;
	}
    }

#if defined PICASSO96
    if (! dgamode) {
	if (screen_is_picasso && refresh_necessary) {
	    DO_PUTIMAGE (pic_dinfo.ximg, 0, 0, 0, 0,
			 picasso_vidinfo.width, picasso_vidinfo.height);
	    refresh_necessary = 0;
	    memset (picasso_invalid_lines, 0, sizeof picasso_invalid_lines);
	} else if (screen_is_picasso && picasso_has_invalid_lines) {
	    int i;
	    int strt = -1;

	    picasso_invalid_lines[picasso_vidinfo.height] = 0;
	    for (i = picasso_invalid_start; i < picasso_invalid_stop + 2; i++) {
		if (picasso_invalid_lines[i]) {
		    picasso_invalid_lines[i] = 0;
		    if (strt != -1)
			continue;
		    strt = i;
		} else {
		    if (strt == -1)
			continue;
		    DO_PUTIMAGE (pic_dinfo.ximg, 0, strt, 0, strt,
				 picasso_vidinfo.width, i - strt);
		    strt = -1;
		}
	    }
	    if (strt != -1)
		abort ();
	}
    }
    picasso_has_invalid_lines = 0;
    picasso_invalid_start = picasso_vidinfo.height + 1;
    picasso_invalid_stop = -1;
#endif

    if (! dgamode) {
	if (! screen_is_picasso && refresh_necessary) {
	    DO_PUTIMAGE (ami_dinfo.ximg, 0, 0, 0, 0, currprefs.gfx_width_fs, currprefs.gfx_height_fs);
	    refresh_necessary = 0;
	}
	if (cursorOn && !currprefs.x11_hide_cursor) {
	    struct timeval now;
	    int diff;
	    gettimeofday(&now, NULL);
	    diff = (now.tv_sec - lastMotionTime.tv_sec) * 1000000 +
		(now.tv_usec - lastMotionTime.tv_usec);
	    if (diff > 1000000) {
		XDefineCursor (display, mywin, blankCursor);
		cursorOn = 0;
	    }
	}
    }
}

int check_prefs_changed_gfx (void)
{
    if (changed_prefs.gfx_width_win != currprefs.gfx_width_win
	|| changed_prefs.gfx_height_win != currprefs.gfx_height_win)
	fixup_prefs_dimensions (&changed_prefs);

    if (changed_prefs.gfx_width_win == currprefs.gfx_width_win
	&& changed_prefs.gfx_height_win == currprefs.gfx_height_win
	&& changed_prefs.gfx_lores == currprefs.gfx_lores
	&& changed_prefs.gfx_linedbl == currprefs.gfx_linedbl
	&& changed_prefs.gfx_correct_aspect == currprefs.gfx_correct_aspect
	&& changed_prefs.gfx_xcenter == currprefs.gfx_xcenter
	&& changed_prefs.gfx_ycenter == currprefs.gfx_ycenter
	&& changed_prefs.gfx_afullscreen == currprefs.gfx_afullscreen
	&& changed_prefs.gfx_pfullscreen == currprefs.gfx_pfullscreen)
	return 0;

    graphics_subshutdown ();
    currprefs.gfx_width_win = changed_prefs.gfx_width_win;
    currprefs.gfx_height_win = changed_prefs.gfx_height_win;
    currprefs.gfx_lores = changed_prefs.gfx_lores;
    currprefs.gfx_linedbl = changed_prefs.gfx_linedbl;
    currprefs.gfx_correct_aspect = changed_prefs.gfx_correct_aspect;
    currprefs.gfx_xcenter = changed_prefs.gfx_xcenter;
    currprefs.gfx_ycenter = changed_prefs.gfx_ycenter;
    currprefs.gfx_afullscreen = changed_prefs.gfx_afullscreen;
    currprefs.gfx_pfullscreen = changed_prefs.gfx_pfullscreen;

    gui_update_gfx ();

    graphics_subinit ();

    if (! inwindow)
	XWarpPointer (display, None, mywin, 0, 0, 0, 0,
		      current_width / 2, current_height / 2);

    notice_screen_contents_lost ();
    init_row_map ();
    if (screen_is_picasso)
	picasso_enablescreen (1);
    return 0;
}

int debuggable (void) {
    return 1;
}

int needmousehack (void) {
  return 0;
}

void LED (int on) {
}

#ifdef PICASSO96

void DX_Invalidate (int first, int last)
{
    if (first > last)
	return;

    picasso_has_invalid_lines = 1;
    if (first < picasso_invalid_start)
	picasso_invalid_start = first;
    if (last > picasso_invalid_stop)
	picasso_invalid_stop = last;

    while (first <= last) {
	picasso_invalid_lines[first] = 1;
	first++;
    }
}

int DX_BitsPerCannon (void)
{
    return 8;
}

static int palette_update_start=256;
static int palette_update_end=0;

static void DX_SetPalette_real (int start, int count)
{
    if (! screen_is_picasso || picasso96_state.RGBFormat != RGBFB_CHUNKY)
	return;

    if (picasso_vidinfo.pixbytes != 1) {
	/* This is the case when we're emulating a 256 color display.  */
	while (count-- > 0) {
	    int r = picasso96_state.CLUT[start].Red;
	    int g = picasso96_state.CLUT[start].Green;
	    int b = picasso96_state.CLUT[start].Blue;
	    picasso_vidinfo.clut[start++] = (doMask256 (r, red_bits, red_shift)
					     | doMask256 (g, green_bits, green_shift)
					     | doMask256 (b, blue_bits, blue_shift));
	}
	return;
    }

    while (count-- > 0) {
	XColor col = parsed_xcolors[start];
	col.red = picasso96_state.CLUT[start].Red * 0x0101;
	col.green = picasso96_state.CLUT[start].Green * 0x0101;
	col.blue = picasso96_state.CLUT[start].Blue * 0x0101;
	XStoreColor (display, cmap, &col);
	XStoreColor (display, cmap2, &col);
	start++;
    }
#ifdef USE_DGA_EXTENSION
    if (dgamode) {
	dga_colormap_installed ^= 1;
	if (dga_colormap_installed == 1)
	    XF86DGAInstallColormap (display, screen, cmap2);
	else
	    XF86DGAInstallColormap (display, screen, cmap);
    }
#endif
}
void DX_SetPalette (int start, int count)
{
   DX_SetPalette_real (start, count);
}

static void DX_SetPalette_delayed (int start, int count)
{
    if (bit_unit!=8) {
	DX_SetPalette_real(start,count);
	return;
    }
    if (start<palette_update_start)
	palette_update_start=start;
    if (start+count>palette_update_end)
	palette_update_end=start+count;
}

void DX_SetPalette_vsync(void)
{
  if (palette_update_end>palette_update_start) {
    DX_SetPalette_real(palette_update_start,
		       palette_update_end-palette_update_start);
    palette_update_end=0;
    palette_update_start=256;
  }
}

int DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color, RGBFTYPE rgbtype)
{
    /* not implemented yet */
    return 0;
}

int DX_Blit (int srcx, int srcy, int dstx, int dsty, int width, int height, BLIT_OPCODE opcode)
{
    /* not implemented yet */
    return 0;
}

#define MAX_SCREEN_MODES 12

static int x_size_table[MAX_SCREEN_MODES] = { 320, 320, 320, 320, 640, 640, 640, 800, 1024, 1152, 1280, 1280 };
static int y_size_table[MAX_SCREEN_MODES] = { 200, 240, 256, 400, 350, 480, 512, 600, 768,  864,  960,  1024 };

int DX_FillResolutions (uae_u16 *ppixel_format)
{
    Screen *scr = ScreenOfDisplay (display, screen);
    int i, count = 0;
    int w = WidthOfScreen (scr);
    int h = HeightOfScreen (scr);
    int emulate_chunky = 0;

    /* we now need to find display depth first */
    XVisualInfo vi;
    if (!get_best_visual (&vi)) return 0;
    bitdepth = vi.depth;
    bit_unit = get_visual_bit_unit (&vi, bitdepth);

    if (ImageByteOrder (display) == LSBFirst) {
    picasso_vidinfo.rgbformat = (bit_unit == 8 ? RGBFB_CHUNKY
				 : bitdepth == 15 && bit_unit == 16 ? RGBFB_R5G5B5PC
				 : bitdepth == 16 && bit_unit == 16 ? RGBFB_R5G6B5PC
				 : bit_unit == 24 ? RGBFB_B8G8R8
				 : bit_unit == 32 ? RGBFB_B8G8R8A8
				 : RGBFB_NONE);
    } else {
    picasso_vidinfo.rgbformat = (bit_unit == 8 ? RGBFB_CHUNKY
				 : bitdepth == 15 && bit_unit == 16 ? RGBFB_R5G5B5
				 : bitdepth == 16 && bit_unit == 16 ? RGBFB_R5G6B5
				 : bit_unit == 24 ? RGBFB_R8G8B8
				 : bit_unit == 32 ? RGBFB_A8R8G8B8
				 : RGBFB_NONE);
    }

    *ppixel_format = 1 << picasso_vidinfo.rgbformat;
    if (vi.VI_CLASS == TrueColor && (bit_unit == 16 || bit_unit == 32))
	*ppixel_format |= RGBFF_CHUNKY, emulate_chunky = 1;

#if defined USE_DGA_EXTENSION && defined USE_VIDMODE_EXTENSION
    if (dgaavail && vidmodeavail) {
	for (i = 0; i < vidmodecount && count < MAX_PICASSO_MODES; i++) {
	    int j;
	    for (j = 0; j <= emulate_chunky && count < MAX_PICASSO_MODES; j++) {
		DisplayModes[count].res.width = allmodes[i]->hdisplay;
		DisplayModes[count].res.height = allmodes[i]->vdisplay;
		DisplayModes[count].depth = j == 1 ? 1 : bit_unit >> 3;
		DisplayModes[count].refresh = 75;
		count++;
	    }
	}
    } else
#endif
    {
	for (i = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; i++) {
	    int j;
	    for (j = 0; j <= emulate_chunky && count < MAX_PICASSO_MODES; j++) {
		if (x_size_table[i] <= w && y_size_table[i] <= h) {
		    if (x_size_table[i] > picasso_maxw)
			picasso_maxw = x_size_table[i];
		    if (y_size_table[i] > picasso_maxh)
			picasso_maxh = y_size_table[i];
		    DisplayModes[count].res.width = x_size_table[i];
		    DisplayModes[count].res.height = y_size_table[i];
		    DisplayModes[count].depth = j == 1 ? 1 : bit_unit >> 3;
		    DisplayModes[count].refresh = 75;
		    count++;
		}
	    }
	}
    }

    return count;
}

static void set_window_for_picasso (void)
{
    if (current_width == picasso_vidinfo.width && current_height == picasso_vidinfo.height)
	return;

    current_width = picasso_vidinfo.width;
    current_height = picasso_vidinfo.height;
    XResizeWindow (display, mywin, current_width, current_height);
#if defined USE_DGA_EXTENSION && defined USE_VIDMODE_EXTENSION
    if (dgamode && vidmodeavail)
	switch_to_best_mode ();
#endif
}

void gfx_set_picasso_modeinfo (int w, int h, int depth, int rgbfmt)
{
    picasso_vidinfo.width = w;
    picasso_vidinfo.height = h;
    picasso_vidinfo.depth = depth;
    picasso_vidinfo.pixbytes = bit_unit >> 3;

    if (screen_is_picasso)
	set_window_for_picasso ();
}

void gfx_set_picasso_baseaddr (uaecptr a)
{
}

void gfx_set_picasso_state (int on)
{
    if (on == screen_is_picasso)
	return;
    graphics_subshutdown ();
    screen_is_picasso = on;
    if (on) {
	current_width = picasso_vidinfo.width;
	current_height = picasso_vidinfo.height;
        graphics_subinit ();
    } else {
	current_width = gfxvidinfo.width;
	current_height = gfxvidinfo.height;
        graphics_subinit ();
        reset_drawing ();
    }
    if (on)
	DX_SetPalette_real (0, 256);
}

uae_u8 *gfx_lock_picasso (void)
{
#ifdef USE_DGA_EXTENSION
    if (dgamode)
	return fb_addr;
    else
#endif
	return pic_dinfo.ximg->data;
}

void gfx_unlock_picasso (void) {
}
#endif

int lockscr (void) {
    return 1;
}

void unlockscr (void) {
}

void toggle_mousegrab (void) {
}

void framerate_up (void) {
    if (currprefs.gfx_framerate < 20)
	changed_prefs.gfx_framerate = currprefs.gfx_framerate + 1;
}

void framerate_down (void) {
    if (currprefs.gfx_framerate > 1)
	changed_prefs.gfx_framerate = currprefs.gfx_framerate - 1;
}

int is_fullscreen (void) {
  return 1;
}

void toggle_fullscreen (void) {
}

void screenshot (int type) {
    write_log ("Screenshot not implemented yet\n");
}

/*
 * Mouse inputdevice functions
 */

/* Hardwire for 3 axes and 3 buttons
 * There is no 3rd axis as such - it's the mousewheel 
 */
#define MAX_BUTTONS     3
#define MAX_AXES        3
#define FIRST_AXIS      0
#define FIRST_BUTTON    MAX_AXES

static int init_mouse (void) {
   return 1;
}

static void close_mouse (void) {
   return;
}

static int acquire_mouse (int num, int flags) {
   return 1;
}

static void unacquire_mouse (int num) {
   return;
}

static int get_mouse_num (void) {
    return 1;
}

static char *get_mouse_name (int mouse) {
    return 0;
}

static int get_mouse_widget_num (int mouse) {
    return MAX_AXES + MAX_BUTTONS;
}

static int get_mouse_widget_first (int mouse, int type) {
    switch (type) {
        case IDEV_WIDGET_BUTTON:
            return FIRST_BUTTON;
        case IDEV_WIDGET_AXIS:
            return FIRST_AXIS;
    }
    return -1;
}

static int get_mouse_widget_type (int mouse, int num, char *name, uae_u32 *code) {
    if (num >= MAX_AXES && num < MAX_AXES + MAX_BUTTONS) {
        if (name)
            sprintf (name, "Button %d", num + 1 + MAX_AXES);
        return IDEV_WIDGET_BUTTON;
    } else if (num < MAX_AXES) {
        if (name)
            sprintf (name, "Axis %d", num + 1);
        return IDEV_WIDGET_AXIS;
    }
    return IDEV_WIDGET_NONE;
}

static void read_mouse (void) {
}

struct inputdevice_functions inputdevicefunc_mouse = {
    init_mouse, close_mouse, acquire_mouse, unacquire_mouse, read_mouse,
    get_mouse_num, get_mouse_name,
    get_mouse_widget_num, get_mouse_widget_type,
    get_mouse_widget_first
};

/*
 * Keyboard inputdevice functions
 */
static int get_kb_num (void) {
    return 1;
}

static char *get_kb_name (int kb) {
    return 0;
}

static int get_kb_widget_num (int kb) {
    return 255; // fix me
}

static int get_kb_widget_first (int kb, int type) {
    return 0;
}

static int get_kb_widget_type (int kb, int num, char *name, uae_u32 *code) {
    // fix me
    *code = num;
    return IDEV_WIDGET_KEY;
}

static void read_kb (void) {
}

static int init_kb (void) {
    set_default_hotkeys(default_hotkeys);
    return 1;
}

static void close_kb (void) {
}

static int acquire_kb (int num, int flags) {
    return 1;
}

static void unacquire_kb (int num) {
}

/*
 * Default inputdevice config for X11 mouse
 */
void input_get_default_mouse (struct uae_input_device *uid) {
    /* Supports only one mouse */
    uid[0].eventid[ID_AXIS_OFFSET + 0][0]   = INPUTEVENT_MOUSE1_HORIZ;
    uid[0].eventid[ID_AXIS_OFFSET + 1][0]   = INPUTEVENT_MOUSE1_VERT;
    uid[0].eventid[ID_AXIS_OFFSET + 2][0]   = INPUTEVENT_MOUSE1_WHEEL;
    uid[0].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
    uid[0].enabled = 1;
}

struct inputdevice_functions inputdevicefunc_keyboard = {
    init_kb, close_kb, acquire_kb, unacquire_kb,
    read_kb, get_kb_num, get_kb_name, get_kb_widget_num,
    get_kb_widget_type, get_kb_widget_first
};

int getcapslockstate (void) {
    return 0;
}

void setcapslockstate (int state) {
}

/*
 * Handle gfx cfgfile options
 */
void gfx_save_options (FILE *f, struct uae_prefs *p) {
}

int gfx_parse_option (struct uae_prefs *p, char *option, char *value) {
}

void gfx_default_options (struct uae_prefs *p) {
}
