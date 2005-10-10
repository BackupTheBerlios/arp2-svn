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
#include <glgfx_pixel.h>
#include <glgfx_view.h>
#include <glgfx_viewport.h>

#include <ctype.h>
#include <inttypes.h>

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

static void dump_picasso_vidinfo(void) {
#define DUMP_FIELD(x) printf(#x ": %08lx\n", picasso_vidinfo.x);
  DUMP_FIELD(width);
  DUMP_FIELD(height);
  DUMP_FIELD(depth);
  DUMP_FIELD(rowbytes);
  DUMP_FIELD(pixbytes);
  DUMP_FIELD(extra_mem);
  DUMP_FIELD(rgbformat);
}

/*
 * Default hotkeys
 *
 * We need a better way of doing this. ;-)
 */
static struct uae_hotkeyseq default_hotkeys[] =
{
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_q, -1, -1,           INPUTEVENT_SPC_QUIT) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_r, -1, -1,           INPUTEVENT_SPC_WARM_RESET) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_lshift, glgfx_input_r, -1,   INPUTEVENT_SPC_COLD_RESET) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_d, -1, -1,           INPUTEVENT_SPC_ENTERDEBUGGER) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_s, -1, -1,           INPUTEVENT_SPC_TOGGLEFULLSCREEN) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_m, -1, -1,           INPUTEVENT_SPC_TOGGLEMOUSEMODE) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_g, -1, -1,           INPUTEVENT_SPC_TOGGLEMOUSEGRAB) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_i, -1, -1,           INPUTEVENT_SPC_INHIBITSCREEN) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_p, -1, -1,           INPUTEVENT_SPC_SCREENSHOT) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_a, -1, -1,           INPUTEVENT_SPC_SWITCHINTERPOL) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_npadd, -1, -1,      INPUTEVENT_SPC_INCRFRAMERATE) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_npsub, -1, -1, INPUTEVENT_SPC_DECRFRAMERATE) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_f1, -1, -1,	      INPUTEVENT_SPC_FLOPPY0) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_f2, -1, -1,	      INPUTEVENT_SPC_FLOPPY1) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_f3, -1, -1,	      INPUTEVENT_SPC_FLOPPY2) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_f4, -1, -1,	      INPUTEVENT_SPC_FLOPPY3) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_lshift, glgfx_input_f1, -1,  INPUTEVENT_SPC_EFLOPPY0) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_lshift, glgfx_input_f2, -1,  INPUTEVENT_SPC_EFLOPPY1) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_lshift, glgfx_input_f3, -1,  INPUTEVENT_SPC_EFLOPPY2) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_lshift, glgfx_input_f4, -1,  INPUTEVENT_SPC_EFLOPPY3) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_return, -1, -1,      INPUTEVENT_SPC_ENTERGUI) },
    { MAKE_HOTKEYSEQ(glgfx_input_f12, glgfx_input_f, -1, -1,           INPUTEVENT_SPC_FREEZEBUTTON) },
    { HOTKEYS_END }
};

static void* p96_buffer;
static size_t p96_bytes_per_row;

static int screen_is_picasso;
static char picasso_invalid_lines[1201];
static int picasso_has_invalid_lines;
static int picasso_invalid_start, picasso_invalid_stop;
static uint32_t picasso_maxw = 0, picasso_maxh = 0;

struct p96_format {
    int                     pixbytes;
    int                     rgbformat;
    enum glgfx_pixel_format format;
};

static struct p96_format p96_formats[] = {
//  { 1, RGBFB_CHUNKY,   glgfx_pixel_format_a4r4g4b4 },
  { 2, RGBFB_R5G6B5PC, glgfx_pixel_format_r5g6b5   },
  { 4, RGBFB_B8G8R8A8, glgfx_pixel_format_a8b8g8r8 },
};

static int pixel_glgfx_to_p96(enum glgfx_pixel_format format) {
  size_t f;

  for (f = 0; f < sizeof (p96_formats) / sizeof (p96_formats[0]); ++f) {
    if (p96_formats[f].format == format) {
      return p96_formats[f].rgbformat;
    }
  }

  return 0;
}

static enum glgfx_pixel_format pixel_p96_to_glgfx(int rgbformat) {
  size_t f;

  for (f = 0; f < sizeof (p96_formats) / sizeof (p96_formats[0]); ++f) {
    if (p96_formats[f].rgbformat == rgbformat) {
      return p96_formats[f].format;
    }
  }

  return glgfx_pixel_format_unknown;
}

static struct glgfx_bitmap*    bitmap = NULL;
static struct glgfx_rasinfo*   rasinfo = NULL;
static struct glgfx_viewport*  viewport = NULL;
static struct glgfx_view*      view = NULL;
static bool                    fullscreen = true;
static uint32_t                real_width;
static uint32_t                real_height;
static uint32_t                bm_width;
static uint32_t                bm_height;
static enum glgfx_pixel_format bm_format;

void toggle_mousegrab(void);
void framerate_up(void);
void framerate_down(void);

int pause_emulation;


void flush_line(int y) {
  printf("flush_line(%d)\n", y);
}

void flush_block(int ystart, int ystop) {
  //  printf("flush_block(%d, %d)\n", ystart, ystop);
}

void flush_screen(int ystart, int ystop) {
  //  printf("flush_screen(%d, %d)\n", ystart, ystop);
  glgfx_view_render(view);
  glgfx_monitor_waittof(glgfx_monitors[0]);
  glgfx_monitor_swapbuffers(glgfx_monitors[0]);
}

void flush_clear_screen(void) {
  printf("flush_clear_screen()\n");
}


int graphics_setup(void) {
  printf("graphics_setup()\n");

  if (glgfx_create_monitors()) {
    if (glgfx_monitor_getattr(glgfx_monitors[0], glgfx_monitor_attr_width, &real_width) &&
	glgfx_monitor_getattr(glgfx_monitors[0], glgfx_monitor_attr_height, &real_height)) {

      viewport = glgfx_viewport_create(real_width, real_height, 0, 0);

      if (viewport != NULL) {
	view = glgfx_view_create(glgfx_monitors[0]);

	if (view != NULL) {
	  if (glgfx_view_addviewport(view, viewport)) {
	    if (glgfx_input_acquire(false)) {
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
  uint32_t width, height, format, rowbytes, pixbytes;
  uint32_t rb = 0, gb = 0, bb = 0, ab = 0;
  uint32_t rs = 0, gs = 0, bs = 0, as = 0;

  bitmap = glgfx_bitmap_create(bm_width, bm_height,
			       16, 0, NULL, bm_format,
			       glgfx_monitors[0]);

  if (bitmap == NULL) {
    return 0;
  }

  printf("created bitmap %d %d format %08lx\n", bm_width, bm_height, bm_format);

  glgfx_bitmap_getattr(bitmap, glgfx_bitmap_attr_width,       &width);
  glgfx_bitmap_getattr(bitmap, glgfx_bitmap_attr_height,      &height);
  glgfx_bitmap_getattr(bitmap, glgfx_bitmap_attr_format,      &format);
  glgfx_bitmap_getattr(bitmap, glgfx_bitmap_attr_bytesperrow, &rowbytes);

  glgfx_pixel_getattr(format, glgfx_pixel_attr_bytesperpixel, &pixbytes);
  
  glgfx_pixel_getattr(format, glgfx_pixel_attr_redbits,       &rb);
  glgfx_pixel_getattr(format, glgfx_pixel_attr_greenbits,     &gb);
  glgfx_pixel_getattr(format, glgfx_pixel_attr_bluebits,      &bb);
  glgfx_pixel_getattr(format, glgfx_pixel_attr_alphabits,     &ab);
  glgfx_pixel_getattr(format, glgfx_pixel_attr_redshift,      &rs);
  glgfx_pixel_getattr(format, glgfx_pixel_attr_greenshift,    &gs);
  glgfx_pixel_getattr(format, glgfx_pixel_attr_blueshift,     &bs);
  glgfx_pixel_getattr(format, glgfx_pixel_attr_alphashift,    &as);
  
  alloc_colors64k(rb, gb, bb, rs, gs, bs, ab, as, ((1 << ab) - 1));

#ifdef PICASSO96
  if (!screen_is_picasso) {
#endif
    gfxvidinfo.width         = width;
    gfxvidinfo.height        = height;
    gfxvidinfo.maxblocklines = height + 1;
    gfxvidinfo.pixbytes      = pixbytes;
    gfxvidinfo.rowbytes      = rowbytes;
    gfxvidinfo.emergmem      = malloc(rowbytes);
    gfxvidinfo.linemem       = NULL;

    reset_drawing ();
    
#ifdef PICASSO96
  }
  else {
    picasso_has_invalid_lines	= 0;
    picasso_invalid_start	= height + 1;
    picasso_invalid_stop	= -1;
    
    picasso_vidinfo.width       = width;
    picasso_vidinfo.height      = height;
    picasso_vidinfo.depth       = rb + gb + bb + ab;
    picasso_vidinfo.pixbytes    = pixbytes;
    picasso_vidinfo.rowbytes    = rowbytes;
    picasso_vidinfo.extra_mem	= 1;

    p96_buffer = malloc(width*height*pixbytes);
    p96_bytes_per_row = width*pixbytes;
    
    memset (picasso_invalid_lines, 0, sizeof picasso_invalid_lines);
  }
#endif

  rasinfo = glgfx_viewport_addbitmap(viewport, bitmap, 0, 0, width, height);

  if (rasinfo == NULL) {
    return 0;
  }
  
  inputdevice_release_all_keys();
  reset_hotkeys();

  printf("viewport_setup -> 1\n");
  return 1;
}


static void viewport_shutdown(void) {

  if (p96_buffer == NULL) {
    free(p96_buffer);
    p96_buffer = NULL;
    p96_bytes_per_row = 0;
  }
  
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

int graphics_init(void)
{
  printf("graphics_init()\n");

  bm_format = glgfx_pixel_format_a4r4g4b4;
  bm_width  = currprefs.gfx_width_fs;
  bm_height = currprefs.gfx_height_fs;
  
  return viewport_setup();
}


void graphics_leave(void) {
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
  dumpcustom();
}

static struct timeval lastMotionTime;

static int refresh_necessary = 0;

void handle_events(void)
{
  enum glgfx_input_code code;
  
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
  
  if (screen_is_picasso && picasso_has_invalid_lines) {
    glgfx_bitmap_update(bitmap, 0, 0, bm_width, bm_height,
			p96_buffer, bm_format, p96_bytes_per_row);
    glgfx_view_render(view);
    glgfx_monitor_swapbuffers(glgfx_monitors[0]);
    printf("update from %d to %d\n", picasso_invalid_start, picasso_invalid_stop);
  }

  picasso_has_invalid_lines = 0;
  picasso_invalid_start = picasso_vidinfo.height + 1;
  picasso_invalid_stop = -1;
}

int check_prefs_changed_gfx(void)
{
  uint32_t w = (maxhpos - 46) * 4;
//  printf("check_prefs_changed_gfx ()\n");
  gui_update_gfx ();

  if (!screen_is_picasso && (bm_height != NUMSCRLINES * 2 || bm_width != w)) {
    viewport_shutdown();
    bm_format = glgfx_pixel_format_a4r4g4b4;
    bm_width  = w;
    bm_height = NUMSCRLINES*2;
    viewport_setup();
  }

  notice_screen_contents_lost();

  return 0;
}

int debuggable(void) {
    return 1;
}

int needmousehack(void) {
  return 0;
}

void LED(int on) {
  printf("LED(%d)\n", on);
}

#ifdef PICASSO96

void DX_Invalidate(int first, int last) {
  printf("DX_Invalidate(%d, %d)\n", first, last);
  if (first > last) {
    return;
  }

  picasso_has_invalid_lines = 1;

  if (first < picasso_invalid_start) {
    picasso_invalid_start = first;
  }

  if (last > picasso_invalid_stop) {
    picasso_invalid_stop = last;
  }
}

int DX_BitsPerCannon(void) {
  return 8;
}

static int palette_update_start=256;
static int palette_update_end=0;

void DX_SetPalette(int start, int count)
{
  if (!screen_is_picasso || picasso96_state.RGBFormat != RGBFB_CHUNKY)
    return;

  if (picasso_vidinfo.pixbytes != 1) {
    /* This is the case when we're emulating a 256 color display. */
    while (count-- > 0) {
      int r = picasso96_state.CLUT[start].Red;
      int g = picasso96_state.CLUT[start].Green;
      int b = picasso96_state.CLUT[start].Blue;
      picasso_vidinfo.clut[start++] = (doMask256 (r, 4, 12)
				       | doMask256 (g, 4, 8)
				       | doMask256 (b, 4, 0));
/*       picasso_vidinfo.clut[start++] = (doMask256 (r, red_bits, red_shift) */
/* 				       | doMask256 (g, green_bits, green_shift) */
/* 				       | doMask256 (b, blue_bits, blue_shift)); */
    }
  }
  else {
    // TODO: Add chunky emulation pixel shaders in libglgfx to handle this
    abort();
  }
}


void DX_SetPalette_vsync(void) {
/*   printf("DX_SetPalette_vsync\n"); */
  if (palette_update_end>palette_update_start) {
    DX_SetPalette(palette_update_start,
		  palette_update_end-palette_update_start);
    palette_update_end=0;
    palette_update_start=256;
  }
}

int DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color, RGBFTYPE rgbtype) {
//  printf("DX_Fill\n");
  /* not implemented yet */
  return 0;
}

int DX_Blit (int srcx, int srcy, int dstx, int dsty, int width, int height, BLIT_OPCODE opcode) {
//  printf("DX_Blit\n");
  /* not implemented yet */
  return 0;//opcode == BLIT_SRC;
}

#define MAX_SCREEN_MODES 19

static uint32_t x_size_table[MAX_SCREEN_MODES] = { 320, 320, 320, 320, 640, 640, 640, 800, 1024, 1024, 1152, 1280, 1280, 1280, 1680, 1600, 1920, 2048, 2560 };
static uint32_t y_size_table[MAX_SCREEN_MODES] = { 200, 240, 256, 400, 400, 480, 512, 600,  640,  768,  864,  800,  960, 1024, 1050, 1200, 1200, 1280, 1600 };

int DX_FillResolutions(uae_u16 *ppixel_format) {
  int i;
  int count;

  picasso_vidinfo.rgbformat = RGBFB_B8G8R8A8;
    
  *ppixel_format = RGBFF_B8G8R8A8 | RGBFF_R5G6B5PC | RGBFF_CHUNKY;

  for (i = 0, count = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; ++i) {
    if (x_size_table[i] <= real_width &&
	y_size_table[i] <= real_height) {
      unsigned int f;
      
      if (x_size_table[i] > picasso_maxw) {
	picasso_maxw = x_size_table[i];
      }

      if (y_size_table[i] > picasso_maxh) {
	picasso_maxh = y_size_table[i];
      }

      for (f = 0; f < sizeof (p96_formats) / sizeof (p96_formats[0]) && count < MAX_PICASSO_MODES; ++f) {
	uint32_t vsync = 60;

	glgfx_monitor_getattr(glgfx_monitors[0], glgfx_monitor_attr_vsync, &vsync);
	DisplayModes[count].res.width  = x_size_table[i];
	DisplayModes[count].res.height = y_size_table[i];
	DisplayModes[count].depth      = p96_formats[f].pixbytes;
	DisplayModes[count].refresh    = vsync;

	printf ("Picasso resolution %d x %d @ %d allowed\n",
		   DisplayModes[count].res.width,
		   DisplayModes[count].res.height,
		   DisplayModes[count].depth);

	count++;
      }
    }
  }

  printf("Max. Picasso screen size: %d x %d\n", picasso_maxw, picasso_maxh);

  return count;
}

void gfx_set_picasso_modeinfo (int w, int h, int depth, int rgbfmt) {
  printf("gfx_set_picasso_modeinfo %d %d %d %08lx\n", w, h, depth, rgbfmt);

  picasso_vidinfo.width     = w;
  picasso_vidinfo.height    = h;
  picasso_vidinfo.rgbformat = rgbfmt;
/*   picasso_vidinfo.depth = depth; */
/*   picasso_vidinfo.pixbytes = 2; */

  if (screen_is_picasso) {
    viewport_shutdown();
    bm_format = pixel_p96_to_glgfx(picasso_vidinfo.rgbformat);
    bm_width  = picasso_vidinfo.width;
    bm_height = picasso_vidinfo.height;
    viewport_setup();
  }
}

void gfx_set_picasso_state (int on)
{
  printf("gfx_set_picasso_state %d\n", on);
  
  if (on == screen_is_picasso)
    return;
    
  viewport_shutdown();
  screen_is_picasso = on;
  if (on) {
    bm_format = pixel_p96_to_glgfx(picasso_vidinfo.rgbformat);
    bm_width  = picasso_vidinfo.width;
    bm_height = picasso_vidinfo.height;
    viewport_setup();
  } else {
    bm_format = glgfx_pixel_format_a4r4g4b4;
    bm_width  = gfxvidinfo.width;
    bm_height = gfxvidinfo.height;
    viewport_setup();
    reset_drawing();
  }

  if (on) {
    DX_SetPalette(0, 256);
  }
}

uae_u8* gfx_lock_picasso(void) {
  return p96_buffer;
  
/*   uae_u8* base = NULL; */
  
/*   if (glgfx_bitmap_lock(bitmap, false, true)) { */
/*     base = glgfx_bitmap_map(bitmap); */
/*   } */

//  printf("gfx_lock_picasso -> %p\n", base);
/*   return base; */
}

void gfx_unlock_picasso(void) {
  return;
/*   glgfx_bitmap_unmap(bitmap); */
/*   glgfx_bitmap_unlock(bitmap); */
//  printf("gfx_unlock_picasso\n");
}

#endif

int lockscr (void) {
  if (glgfx_bitmap_lock(bitmap, false, true)) {
    uintptr_t addr = 0;
    //    gfxvidinfo.bufmem = glgfx_bitmap_map(bitmap);

    if (glgfx_bitmap_getattr(bitmap, 
			     glgfx_bitmap_attr_mapaddr, 
			     &addr) &&
	addr != 0) {
      static void* old_addr = NULL;
      
      gfxvidinfo.bufmem = (void*) addr;

      if (gfxvidinfo.bufmem != old_addr) {
	old_addr = gfxvidinfo.bufmem;
	init_row_map();
      }

      return 1;
    }
  }

  return 0;
}

void unlockscr (void) {
  glgfx_bitmap_unlock(bitmap, 0, 0, bm_width, bm_height);
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
    glgfx_viewport_move(viewport, real_width, real_height, 0, 0);
  }
  else {
    glgfx_viewport_move(viewport, bm_width, bm_height,
			(real_width - bm_width) / 2, (real_height - bm_height) / 2);
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
