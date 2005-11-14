#ifndef ARP2_glgfx_glgfx_input_h
#define ARP2_glgfx_glgfx_input_h

#include <glgfx.h>

struct glgfx_monitor;

enum glgfx_input_code {
  glgfx_input_none         = 0x00000000,

  glgfx_input_a = 0x20,
  glgfx_input_b = 0x35,
  glgfx_input_c = 0x33,
  glgfx_input_d = 0x22,
  glgfx_input_e = 0x12,
  glgfx_input_f = 0x23,
  glgfx_input_g = 0x24,
  glgfx_input_h = 0x25,
  glgfx_input_i = 0x17,
  glgfx_input_j = 0x26,
  glgfx_input_k = 0x27,
  glgfx_input_l = 0x28,
  glgfx_input_m = 0x37,
  glgfx_input_n = 0x36,
  glgfx_input_o = 0x18,
  glgfx_input_p = 0x19,
  glgfx_input_q = 0x10,
  glgfx_input_r = 0x13,
  glgfx_input_s = 0x21,
  glgfx_input_t = 0x14,
  glgfx_input_u = 0x16,
  glgfx_input_v = 0x34,
  glgfx_input_w = 0x11,
  glgfx_input_x = 0x32,
  glgfx_input_y = 0x15,
  glgfx_input_z = 0x31,
  glgfx_input_0 = 0x0a,
  glgfx_input_1 = 0x01,
  glgfx_input_2 = 0x02,
  glgfx_input_3 = 0x03,
  glgfx_input_4 = 0x04,
  glgfx_input_5 = 0x05,
  glgfx_input_6 = 0x06,
  glgfx_input_7 = 0x07,
  glgfx_input_8 = 0x08,
  glgfx_input_9 = 0x09,
  glgfx_input_np0 = 0x0f,
  glgfx_input_np1 = 0x1d,
  glgfx_input_np2 = 0x1e,
  glgfx_input_np3 = 0x1f,
  glgfx_input_np4 = 0x2d,
  glgfx_input_np5 = 0x2e,
  glgfx_input_np6 = 0x2f,
  glgfx_input_np7 = 0x3d,
  glgfx_input_np8 = 0x3e,
  glgfx_input_np9 = 0x3f,
  glgfx_input_npdiv = 0x5c,
  glgfx_input_npmul = 0x5d,
  glgfx_input_npsub = 0x4a,
  glgfx_input_npadd = 0x5e,
  glgfx_input_npdel = 0x3c,
  glgfx_input_nplparen = 0x5a,
  glgfx_input_nprparen = 0x5b,
  glgfx_input_f1 = 0x50,
  glgfx_input_f2 = 0x51,
  glgfx_input_f3 = 0x52,
  glgfx_input_f4 = 0x53,
  glgfx_input_f5 = 0x54,
  glgfx_input_f6 = 0x55,
  glgfx_input_f7 = 0x56,
  glgfx_input_f8 = 0x57,
  glgfx_input_f9 = 0x58,
  glgfx_input_f10 = 0x59,
  glgfx_input_up = 0x4c,
  glgfx_input_down = 0x4d,
  glgfx_input_left = 0x4f,
  glgfx_input_right = 0x4e,
  glgfx_input_space = 0x40,
  glgfx_input_bs = 0x41,
  glgfx_input_tab = 0x42,
  glgfx_input_enter = 0x43,
  glgfx_input_return = 0x44,
  glgfx_input_esc = 0x45,
  glgfx_input_del = 0x46,
  glgfx_input_lshift = 0x60,
  glgfx_input_rshift = 0x61,
  glgfx_input_capslock = 0x62,
  glgfx_input_lctrl = 0x63,
  glgfx_input_rctrl = 0x7f,
  glgfx_input_lalt = 0x64,
  glgfx_input_ralt = 0x65,
  glgfx_input_lmeta = 0x66,
  glgfx_input_rmeta = 0x67,
  glgfx_input_menu = 0x5f,
  glgfx_input_lbracket = 0x1a,
  glgfx_input_rbracket = 0x1b,
  glgfx_input_semicolon = 0x29,
  glgfx_input_comma = 0x38,
  glgfx_input_period = 0x39,
  glgfx_input_slash = 0x3a,
  glgfx_input_backslash = 0x0d,
  glgfx_input_quote = 0x2a,
  glgfx_input_numbersign = 0x2b,
  glgfx_input_ltgt = 0x30,
  glgfx_input_backquote = 0x00,
  glgfx_input_minus = 0x0b,
  glgfx_input_equal = 0x0c,

  glgfx_input_insert = 0x47,
  glgfx_input_pageup = 0x48,
  glgfx_input_pagedown = 0x49,
  glgfx_input_f11 = 0x4b,
  glgfx_input_f12 = 0x6f,
  glgfx_input_scrlock = 0x6b,
  glgfx_input_prtscr = 0x6c,
  glgfx_input_numlock = 0x6d,
  glgfx_input_pause = 0x6e,
  glgfx_input_home = 0x70,
  glgfx_input_end = 0x71,

  glgfx_input_stop = 0x72,
  glgfx_input_play = 0x73,
  glgfx_input_prev = 0x74,
  glgfx_input_next = 0x75,
  glgfx_input_rew = 0x76,
  glgfx_input_ff = 0x77,
  
  // Multimedia keyboard keys [see also /usr/X11R6/lib/X11/XKeysymDB]
/*   glgfx_input_my_computer = 0x80, */
/*   glgfx_input_my_documents = 0x80, */
/*   glgfx_input_my_pictures = 0x80, */
/*   glgfx_input_my_music = 0x80, */

/*   glgfx_input_mute = 0x80, */
/*   glgfx_input_volume_up = 0x80, */
/*   glgfx_input_volume_down = 0x80, */

/*   glgfx_input_media = 0x80, */
/*   glgfx_input_mail = 0x80, */
/*   glgfx_input_web_home = 0x80, */
/*   glgfx_input_messenger = 0x80, */
/*   glgfx_input_calculator = 0x80, */

/*   glgfx_input_log_off = 0x80, */
/*   glgfx_input_sleep = 0x80, */
  
/*   glgfx_input_help = 0x80, */
/*   glgfx_input_undo = 0x80, */
/*   glgfx_input_redo = 0x80, */
/*   glgfx_input_new = 0x80, */
/*   glgfx_input_open = 0x80, */
/*   glgfx_input_close = 0x80, */
/*   glgfx_input_reply = 0x80, */
/*   glgfx_input_forward = 0x80, */
/*   glgfx_input_send = 0x80, */
/*   glgfx_input_spell = 0x80, */
/*   glgfx_input_save = 0x80, */
/*   glgfx_input_print = 0x80, */

/*   glgfx_input_webcam = 0x80, */
/*   glgfx_input_search = 0x80, */
  
/*   glgfx_input_vendor = 0x80, */

  // System key codes
  glgfx_input_resetwarning = 0x78,
  glgfx_input_badkey = 0xfa,
  glgfx_input_error = 0xfc,
  glgfx_input_init_powerup = 0xfd,
  glgfx_input_term_powerup = 0xfe,

  
  glgfx_input_key          = 0x00 << 24,
  glgfx_input_mouse_xyz    = 0x01 << 24,
  glgfx_input_mouse_button = 0x03 << 24,
  glgfx_input_mouse_vwheel = 0x04 << 24,
  glgfx_input_mouse_hwheel = 0x05 << 24,

  glgfx_input_releasemask  = 0x80000000,
  glgfx_input_typemask     = 0x7f000000,
  glgfx_input_valuemask    = 0x00ffffff,
};


bool glgfx_input_acquire(struct glgfx_monitor* monitor);
bool glgfx_input_release(struct glgfx_monitor* monitor);
enum glgfx_input_code glgfx_input_getcode(void);

#endif /* ARP2_glgfx_glgfx_input_h */
