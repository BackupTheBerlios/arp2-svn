 /*
  * UAE - The Un*x Amiga Emulator
  *
  * glgfx interface
  *
  * Copyright 2005 Martin Blom
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <glgfx.h>
#include <glgfx_bitmap.h>
#include <glgfx_input.h>
#include <glgfx_view.h>
#include <glgfx_viewport.h>

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
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_q, -1, -1,           INPUTEVENT_SPC_QUIT) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_r, -1, -1,           INPUTEVENT_SPC_WARM_RESET) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_lshift, glgfx_input_r, -1,   INPUTEVENT_SPC_COLD_RESET) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_d, -1, -1,           INPUTEVENT_SPC_ENTERDEBUGGER) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_s, -1, -1,           INPUTEVENT_SPC_TOGGLEFULLSCREEN) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_m, -1, -1,           INPUTEVENT_SPC_TOGGLEMOUSEMODE) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_g, -1, -1,           INPUTEVENT_SPC_TOGGLEMOUSEGRAB) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_i, -1, -1,           INPUTEVENT_SPC_INHIBITSCREEN) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_p, -1, -1,           INPUTEVENT_SPC_SCREENSHOT) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_a, -1, -1,           INPUTEVENT_SPC_SWITCHINTERPOL) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_npadd, -1, -1,      INPUTEVENT_SPC_INCRFRAMERATE) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_npsub, -1, -1, INPUTEVENT_SPC_DECRFRAMERATE) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_f1, -1, -1,	      INPUTEVENT_SPC_FLOPPY0) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_f2, -1, -1,	      INPUTEVENT_SPC_FLOPPY1) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_f3, -1, -1,	      INPUTEVENT_SPC_FLOPPY2) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_f4, -1, -1,	      INPUTEVENT_SPC_FLOPPY3) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_lshift, glgfx_input_f1, -1,  INPUTEVENT_SPC_EFLOPPY0) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_lshift, glgfx_input_f2, -1,  INPUTEVENT_SPC_EFLOPPY1) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_lshift, glgfx_input_f3, -1,  INPUTEVENT_SPC_EFLOPPY2) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_lshift, glgfx_input_f4, -1,  INPUTEVENT_SPC_EFLOPPY3) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_return, -1, -1,      INPUTEVENT_SPC_ENTERGUI) },
    { MAKE_HOTKEYSEQ (glgfx_input_f12, glgfx_input_f, -1, -1,           INPUTEVENT_SPC_FREEZEBUTTON) },
    { HOTKEYS_END }
};


static int screen_is_picasso;
static char picasso_invalid_lines[1201];
static int picasso_has_invalid_lines;
static int picasso_invalid_start, picasso_invalid_stop;
static int picasso_maxw = 0, picasso_maxh = 0;

static struct glgfx_bitmap*   bitmap = NULL;
static struct glgfx_rasinfo*  rasinfo = NULL;
static struct glgfx_viewport* viewport = NULL;
static struct glgfx_view*     view = NULL;
static bool                   fullscreen = true;
static uint32_t               width;
static uint32_t               height;

void toggle_mousegrab (void);
void framerate_up (void);
void framerate_down (void);
int xkeysym2amiga (int);

int pause_emulation;


void flush_line (int y) {
  printf("flush_line(%d)\n", y);
}

void flush_block (int ystart, int ystop) {
//  printf("flush_block(%d, %d)\n", ystart, ystop);
}

void flush_screen (int ystart, int ystop) {
//  printf("flush_screen(%d, %d)\n", ystart, ystop);
  glgfx_view_render(view);
  glgfx_monitor_waittof(glgfx_monitors[0]);
}

void flush_clear_screen (void) {
  printf("flush_clear_screen()\n");
}


int graphics_setup (void) {
  printf("graphics_setup()\n");

  if (glgfx_create_monitors()) {
    if (glgfx_monitor_getattr(glgfx_monitors[0], glgfx_monitor_width, &width) &&
	glgfx_monitor_getattr(glgfx_monitors[0], glgfx_monitor_height, &height)) {

      viewport = glgfx_viewport_create(width, height, 0, 0);

      if (viewport != NULL) {
	view = glgfx_view_create(glgfx_monitors[0]);

	if (view != NULL) {
	  if (glgfx_view_addviewport(view, viewport)) {
	    if (glgfx_input_acquire()) {
	      return 1;
	    }
	  }
	}
      }
    }
  }

  return 0;
}

static int viewport_setup(void) {
  int32_t res;

  bitmap = glgfx_bitmap_create(currprefs.gfx_width_win,
			       currprefs.gfx_height_win,
//			       16, 0, NULL, glgfx_pixel_r5g6b5,
			       16, 0, NULL, glgfx_pixel_a4r4g4b4,
			       glgfx_monitors[0]);

  if (bitmap == NULL) {
    return 0;
  }

//  alloc_colors64k (5, 6, 5, 11, 5, 0, 0, 0, 0); 
  alloc_colors64k (4, 4, 4, 8, 4, 0, 4, 12, 0xf);
 
  if (glgfx_bitmap_getattr(bitmap, glgfx_bitmap_width, &res)) {
    gfxvidinfo.width = res;
  }

  if (glgfx_bitmap_getattr(bitmap, glgfx_bitmap_height, &res)) {
    gfxvidinfo.height = res;
//    gfxvidinfo.maxblocklines = 1000;
    gfxvidinfo.maxblocklines = res + 1;
  }

  if (glgfx_bitmap_getattr(bitmap, glgfx_bitmap_bytesperpixel, &res)) {
    gfxvidinfo.pixbytes = res;
  }

  if (glgfx_bitmap_getattr(bitmap, glgfx_bitmap_bytesperrow, &res)) {
    gfxvidinfo.rowbytes = res;
  }

  rasinfo = glgfx_viewport_addbitmap(viewport, bitmap, 0, 0,
				     gfxvidinfo.width, gfxvidinfo.height);

  if (rasinfo == NULL) {
    return 0;
  }

  gfxvidinfo.emergmem = malloc(gfxvidinfo.rowbytes);
  gfxvidinfo.linemem = NULL;

  inputdevice_release_all_keys ();
  reset_hotkeys ();

  printf("bufmem: %x\n", gfxvidinfo.bufmem);
  printf("linemem: %x\n", gfxvidinfo.linemem);
  printf("emergmem: %x\n", gfxvidinfo.emergmem);
  printf("rowbytes: %d\n", gfxvidinfo.rowbytes);
  printf("pixbytes: %d\n", gfxvidinfo.pixbytes);
  printf("width: %d\n", gfxvidinfo.width);
  printf("height: %d\n", gfxvidinfo.height);
  printf("maxblocklines: %d\n", gfxvidinfo.maxblocklines);
  printf("can_double: %d\n", gfxvidinfo.can_double);
  
  return 1;
}


static void viewport_shutdown(void) {
  if (gfxvidinfo.emergmem != NULL) {
    free(gfxvidinfo.emergmem);
  }

  if (rasinfo != NULL) {
    glgfx_viewport_rembitmap(viewport, rasinfo);
    rasinfo = NULL;
  }

  if (bitmap != NULL) {
    glgfx_bitmap_destroy(bitmap);
    bitmap = NULL;
  }

  gfxvidinfo.width = 0;
  gfxvidinfo.height = 0;
  gfxvidinfo.maxblocklines = 0;
  gfxvidinfo.pixbytes = 0;
  gfxvidinfo.rowbytes = 0;
  gfxvidinfo.can_double = 0;
}

int graphics_init (void)
{
  printf("graphics_init()\n");

  return viewport_setup();
}


void graphics_leave (void) {
  printf("graphics_leave()\n");

  glgfx_input_release();

  viewport_shutdown();

  if (view != NULL) {
    if (viewport != NULL) {
      glgfx_view_remviewport(view, viewport);
      glgfx_viewport_destroy(viewport);
    }

    glgfx_view_destroy(view);
  }

  glgfx_destroy_monitors();
  dumpcustom ();
}

static struct timeval lastMotionTime;

static int refresh_necessary = 0;

void handle_events (void)
{
  enum glgfx_input_code code;
  
//  printf("handle_events()\n");
  gui_handle_events ();

  while ((code = glgfx_input_getcode()) != glgfx_input_none) {
    switch (code & glgfx_input_typemask) {
      case glgfx_input_key: {
	int down = (code & glgfx_input_releasemask) == 0;
	int key = code & glgfx_input_valuemask;
	int ievent;

	if ((ievent = match_hotkey_sequence(key, down))) {
	  handle_hotkey_event(ievent, down);
	} else {
	  inputdevice_do_keyboard(code, down);
	}

	break;
      }
	  
      case glgfx_input_mouse_xyz: {
	int rel_x = (int) (char) (((code & glgfx_input_valuemask) >> 0) & 0xff);
	int rel_y = (int) (char) (((code & glgfx_input_valuemask) >> 8) & 0xff);

	setmousestate(0, 0, rel_x, 0);
	setmousestate(0, 1, rel_y, 0);
	break;
      }

      case glgfx_input_mouse_button: {
	int down = (code & glgfx_input_releasemask) == 0;
	int button = code & glgfx_input_valuemask;

	setmousebuttonstate(0, button, down);
	break;
      }

      case glgfx_input_mouse_vwheel:
      case  glgfx_input_mouse_hwheel:
	break;

      default:
	write_log("gfx-glgfx: Unknown input event %ld\n", code);
	break;
    }
  }
}

int check_prefs_changed_gfx (void)
{
//  printf("check_prefs_changed_gfx ()\n");
  gui_update_gfx ();

  notice_screen_contents_lost ();

  return 0;
}

int debuggable (void) {
    return 1;
}

int needmousehack (void) {
  return 0;
}

void LED (int on) {
  printf("LED(%d)\n", on);
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
//  printf("lockscr()\n");

  if (glgfx_bitmap_lock(bitmap, false, true)) {
    gfxvidinfo.bufmem = glgfx_bitmap_map(bitmap);
    init_row_map();

    if (gfxvidinfo.bufmem != NULL) {
//      static void* old_addr = NULL;
      
//      if (gfxvidinfo.bufmem != old_addr) {
//	old_addr = gfxvidinfo.bufmem;
//      }

      return 1;
    }
  }

  return 0;
}

void unlockscr (void) {
//  printf("unlockscr()\n");

  if (gfxvidinfo.bufmem != NULL) {
      int x,y;
      for(y = 0; y < 100; ++y) {
	for(x = 0; x < 100; ++x) {
	  unsigned short* p = gfxvidinfo.bufmem;

	  p[y*gfxvidinfo.width+x] = -1;
	}
      }
    glgfx_bitmap_unmap(bitmap);
    gfxvidinfo.bufmem = NULL;
    init_row_map();
  }

  glgfx_bitmap_unlock(bitmap);
}


void toggle_mousegrab (void) {
  printf("toggle_mousegrab()\n");
}

void framerate_up (void) {
  printf("framerate_up()\n");
  if (currprefs.gfx_framerate < 20)
    changed_prefs.gfx_framerate = currprefs.gfx_framerate + 1;
}

void framerate_down (void) {
  printf("framerate_down()\n");
  if (currprefs.gfx_framerate > 1)
    changed_prefs.gfx_framerate = currprefs.gfx_framerate - 1;
}

int is_fullscreen (void) {
  printf("is_fullscreen() -> %d\n", fullscreen);
  return fullscreen;
}

void toggle_fullscreen (void) {
  fullscreen = !fullscreen;

  if (fullscreen) {
    glgfx_viewport_move(viewport, width, height, 0, 0);
  }
  else {
    glgfx_viewport_move(viewport, gfxvidinfo.width, gfxvidinfo.height,
			(width - gfxvidinfo.width) / 2, (height - gfxvidinfo.height) / 2);
  }

  printf("toggle_fullscreen()\n");
}

void screenshot (int type) {
  printf("screenshot(%d)\n", type);
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
  return 0;
}

void gfx_default_options (struct uae_prefs *p) {
}
