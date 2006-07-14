
#include "glgfx-config.h"
#include <errno.h>
#include <stdlib.h>
#include <glib.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

#include <sys/inotify.h>
#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "glgfx.h"
#include "glgfx_input.h"
#include "glgfx_intern.h"
#include "glgfx_monitor.h"

static GList* monitors = NULL;
static GQueue* pending_queue = NULL;
static GQueue* cache_queue = NULL;

/*** Event mappings **********************************************************/

static enum glgfx_input_code xorg_codes[256] = {
  // Row 0: 0-
  glgfx_input_none,	glgfx_input_none,	glgfx_input_none,	glgfx_input_none,
  glgfx_input_none,	glgfx_input_none,	glgfx_input_none,	glgfx_input_none,
  glgfx_input_none,

  // Row 1: 9-
  glgfx_input_esc,	glgfx_input_1,  	glgfx_input_2,  	glgfx_input_3,
  glgfx_input_4,     	glgfx_input_5,  	glgfx_input_6,  	glgfx_input_7,
  glgfx_input_8,     	glgfx_input_9,  	glgfx_input_0,  	glgfx_input_minus,
  glgfx_input_equal, 	glgfx_input_backspace,

  // Row 2: 23-
  glgfx_input_tab,      glgfx_input_q,       	glgfx_input_w,  	glgfx_input_e,
  glgfx_input_r,        glgfx_input_t,       	glgfx_input_y,  	glgfx_input_u,
  glgfx_input_i,        glgfx_input_o,       	glgfx_input_p,  	glgfx_input_lbracket,
  glgfx_input_rbracket, glgfx_input_return,

  // Row 3: 37-
  glgfx_input_lctrl,    glgfx_input_a,  	glgfx_input_s,         	glgfx_input_d,
  glgfx_input_f,        glgfx_input_g,  	glgfx_input_h,          glgfx_input_j,
  glgfx_input_k,        glgfx_input_l,  	glgfx_input_semicolon,  glgfx_input_quote,
  glgfx_input_backquote,

  // Row 4: 50-
  glgfx_input_lshift,  	glgfx_input_backslash,  glgfx_input_z,       	glgfx_input_x,
  glgfx_input_c,       	glgfx_input_v,          glgfx_input_b,       	glgfx_input_n,
  glgfx_input_m,       	glgfx_input_comma,      glgfx_input_period,  	glgfx_input_slash,
  glgfx_input_rshift,  	glgfx_input_np_mul,

  // Row 5: 64-
  glgfx_input_lalt,  	glgfx_input_space,  	glgfx_input_capslock,  	glgfx_input_f1,
  glgfx_input_f2,  	glgfx_input_f3,  	glgfx_input_f4,  	glgfx_input_f5,
  glgfx_input_f6,  	glgfx_input_f7,  	glgfx_input_f8,  	glgfx_input_f9,
  glgfx_input_f10,

  // Keypad
  glgfx_input_numlock,  glgfx_input_scrlock,  	glgfx_input_np_7,  	glgfx_input_np_8,
  glgfx_input_np_9,  	glgfx_input_np_sub,  	glgfx_input_np_4,  	glgfx_input_np_5,
  glgfx_input_np_6,  	glgfx_input_np_add,  	glgfx_input_np_1,  	glgfx_input_np_2,
  glgfx_input_np_3,  	glgfx_input_np_0,  	glgfx_input_np_decimal,

  // Extra
  glgfx_input_none,  	glgfx_input_none,  	glgfx_input_ltgt,  	glgfx_input_f11,
  glgfx_input_f12, 	glgfx_input_home,  	glgfx_input_up,  	glgfx_input_pageup,
  glgfx_input_left,  	glgfx_input_none,  	glgfx_input_right,  	glgfx_input_end,
  glgfx_input_down,  	glgfx_input_pagedown,  	glgfx_input_insert,  	glgfx_input_del,
  glgfx_input_enter,  	glgfx_input_rctrl,  	glgfx_input_pause,  	glgfx_input_prtscr,
  glgfx_input_np_div,  	glgfx_input_ralt,  	glgfx_input_none,   	glgfx_input_lmeta,
  glgfx_input_rmeta,  	glgfx_input_menu,       // None: 92-93, 101, 114

/*   glgfx_input_np_lparen, */
/*   glgfx_input_np_rparen, */
/*   glgfx_input_numbersign = 0x2b, */
};

static enum glgfx_input_code linux_codes[KEY_MAX + 1] = {
  [0 ... KEY_MAX]	= glgfx_input_none,

  [KEY_ESC]		= glgfx_input_key | glgfx_input_esc,
  [KEY_1]		= glgfx_input_key | glgfx_input_1,
  [KEY_2]		= glgfx_input_key | glgfx_input_2,
  [KEY_3]		= glgfx_input_key | glgfx_input_3,
  [KEY_4]		= glgfx_input_key | glgfx_input_4,
  [KEY_5]		= glgfx_input_key | glgfx_input_5,
  [KEY_6]		= glgfx_input_key | glgfx_input_6,
  [KEY_7]		= glgfx_input_key | glgfx_input_7,
  [KEY_8]		= glgfx_input_key | glgfx_input_8,
  [KEY_9]		= glgfx_input_key | glgfx_input_9,
  [KEY_0]		= glgfx_input_key | glgfx_input_0,
  [KEY_MINUS]		= glgfx_input_key | glgfx_input_minus,
  [KEY_EQUAL]		= glgfx_input_key | glgfx_input_equal,
  [KEY_BACKSPACE]	= glgfx_input_key | glgfx_input_backspace,
  [KEY_TAB]		= glgfx_input_key | glgfx_input_tab,
  [KEY_Q]		= glgfx_input_key | glgfx_input_q,
  [KEY_W]		= glgfx_input_key | glgfx_input_w,
  [KEY_E]		= glgfx_input_key | glgfx_input_e,
  [KEY_R]		= glgfx_input_key | glgfx_input_r,
  [KEY_T]		= glgfx_input_key | glgfx_input_t,
  [KEY_Y]		= glgfx_input_key | glgfx_input_y,
  [KEY_U]		= glgfx_input_key | glgfx_input_u,
  [KEY_I]		= glgfx_input_key | glgfx_input_i,
  [KEY_O]		= glgfx_input_key | glgfx_input_o,
  [KEY_P]		= glgfx_input_key | glgfx_input_p,
  [KEY_LEFTBRACE]	= glgfx_input_key | glgfx_input_lbracket,
  [KEY_RIGHTBRACE]	= glgfx_input_key | glgfx_input_rbracket,
  [KEY_ENTER]		= glgfx_input_key | glgfx_input_return,
  [KEY_LEFTCTRL]	= glgfx_input_key | glgfx_input_lctrl,
  [KEY_A]		= glgfx_input_key | glgfx_input_a,
  [KEY_S]		= glgfx_input_key | glgfx_input_s,
  [KEY_D]		= glgfx_input_key | glgfx_input_d,
  [KEY_F]		= glgfx_input_key | glgfx_input_f,
  [KEY_G]		= glgfx_input_key | glgfx_input_g,
  [KEY_H]		= glgfx_input_key | glgfx_input_h,
  [KEY_J]		= glgfx_input_key | glgfx_input_j,
  [KEY_K]		= glgfx_input_key | glgfx_input_k,
  [KEY_L]		= glgfx_input_key | glgfx_input_l,
  [KEY_SEMICOLON]	= glgfx_input_key | glgfx_input_semicolon,
  [KEY_APOSTROPHE]	= glgfx_input_key | glgfx_input_quote,
  [KEY_GRAVE]		= glgfx_input_key | glgfx_input_backquote,
  [KEY_LEFTSHIFT]	= glgfx_input_key | glgfx_input_lshift,
  [KEY_BACKSLASH]	= glgfx_input_key | glgfx_input_backslash,
  [KEY_Z]		= glgfx_input_key | glgfx_input_z,
  [KEY_X]		= glgfx_input_key | glgfx_input_x,
  [KEY_C]		= glgfx_input_key | glgfx_input_c,
  [KEY_V]		= glgfx_input_key | glgfx_input_v,
  [KEY_B]		= glgfx_input_key | glgfx_input_b,
  [KEY_N]		= glgfx_input_key | glgfx_input_n,
  [KEY_M]		= glgfx_input_key | glgfx_input_m,
  [KEY_COMMA]		= glgfx_input_key | glgfx_input_comma,
  [KEY_DOT]		= glgfx_input_key | glgfx_input_period,
  [KEY_SLASH]		= glgfx_input_key | glgfx_input_slash,
  [KEY_RIGHTSHIFT]	= glgfx_input_key | glgfx_input_rshift,
  [KEY_KPASTERISK]	= glgfx_input_key | glgfx_input_np_mul,
  [KEY_LEFTALT]		= glgfx_input_key | glgfx_input_lalt,
  [KEY_SPACE]		= glgfx_input_key | glgfx_input_space,
  [KEY_CAPSLOCK]	= glgfx_input_key | glgfx_input_capslock,
  [KEY_F1]		= glgfx_input_key | glgfx_input_f1,
  [KEY_F2]		= glgfx_input_key | glgfx_input_f2,
  [KEY_F3]		= glgfx_input_key | glgfx_input_f3,
  [KEY_F4]		= glgfx_input_key | glgfx_input_f4,
  [KEY_F5]		= glgfx_input_key | glgfx_input_f5,
  [KEY_F6]		= glgfx_input_key | glgfx_input_f6,
  [KEY_F7]		= glgfx_input_key | glgfx_input_f7,
  [KEY_F8]		= glgfx_input_key | glgfx_input_f8,
  [KEY_F9]		= glgfx_input_key | glgfx_input_f9,
  [KEY_F10]		= glgfx_input_key | glgfx_input_f10,
  [KEY_NUMLOCK]		= glgfx_input_key | glgfx_input_numlock,
  [KEY_SCROLLLOCK]	= glgfx_input_key | glgfx_input_scrlock,
  [KEY_KP7]		= glgfx_input_key | glgfx_input_np_7,
  [KEY_KP8]		= glgfx_input_key | glgfx_input_np_8,
  [KEY_KP9]		= glgfx_input_key | glgfx_input_np_9,
  [KEY_KPMINUS]		= glgfx_input_key | glgfx_input_np_sub,
  [KEY_KP4]		= glgfx_input_key | glgfx_input_np_4,
  [KEY_KP5]		= glgfx_input_key | glgfx_input_np_5,
  [KEY_KP6]		= glgfx_input_key | glgfx_input_np_6,
  [KEY_KPPLUS]		= glgfx_input_key | glgfx_input_np_add,
  [KEY_KP1]		= glgfx_input_key | glgfx_input_np_1,
  [KEY_KP2]		= glgfx_input_key | glgfx_input_np_2,
  [KEY_KP3]		= glgfx_input_key | glgfx_input_np_3,
  [KEY_KP0]		= glgfx_input_key | glgfx_input_np_0,
  [KEY_KPDOT]		= glgfx_input_key | glgfx_input_np_decimal,
//  [KEY_ZENKAKUHANKAKU]= glgfx_input_none,
  [KEY_102ND]		= glgfx_input_key | glgfx_input_ltgt,
  [KEY_F11]		= glgfx_input_key | glgfx_input_f11,
  [KEY_F12]		= glgfx_input_key | glgfx_input_f12,
//  [KEY_RO]		= glgfx_input_none,
//  [KEY_KATAKANA]	= glgfx_input_none,
//  [KEY_HIRAGANA]	= glgfx_input_none,
//  [KEY_HENKAN]	= glgfx_input_none,
//  [KEY_KATAKANAHIRAGANA]= glgfx_input_none,
//  [KEY_MUHENKAN]	= glgfx_input_none,
//  [KEY_KPJPCOMMA]	= glgfx_input_none,
  [KEY_KPENTER]		= glgfx_input_key | glgfx_input_enter,
  [KEY_RIGHTCTRL]	= glgfx_input_key | glgfx_input_rctrl,
  [KEY_KPSLASH]		= glgfx_input_key | glgfx_input_np_div,
  [KEY_SYSRQ]		= glgfx_input_key | glgfx_input_prtscr,
  [KEY_RIGHTALT]	= glgfx_input_key | glgfx_input_ralt,
//  [KEY_LINEFEED]	= glgfx_input_none,
  [KEY_HOME]		= glgfx_input_key | glgfx_input_home,
  [KEY_UP]		= glgfx_input_key | glgfx_input_up,
  [KEY_PAGEUP]		= glgfx_input_key | glgfx_input_pageup,
  [KEY_LEFT]		= glgfx_input_key | glgfx_input_left,
  [KEY_RIGHT]		= glgfx_input_key | glgfx_input_right,
  [KEY_END]		= glgfx_input_key | glgfx_input_end,
  [KEY_DOWN]		= glgfx_input_key | glgfx_input_down,
  [KEY_PAGEDOWN]	= glgfx_input_key | glgfx_input_pagedown,
  [KEY_INSERT]		= glgfx_input_key | glgfx_input_insert,
  [KEY_DELETE]		= glgfx_input_key | glgfx_input_del,
//  [KEY_MACRO]		= glgfx_input_none,
  [KEY_MUTE]		= glgfx_input_key | glgfx_input_cd_mute,
  [KEY_VOLUMEDOWN]	= glgfx_input_key | glgfx_input_cd_volumedown,
  [KEY_VOLUMEUP]	= glgfx_input_key | glgfx_input_cd_volumeup,
  [KEY_POWER]		= glgfx_input_key | glgfx_input_power,
  [KEY_KPEQUAL]		= glgfx_input_key | glgfx_input_np_equal,
  [KEY_KPPLUSMINUS]	= glgfx_input_key | glgfx_input_np_plusminus,
  [KEY_PAUSE]		= glgfx_input_key | glgfx_input_pause,
  [KEY_KPCOMMA]		= glgfx_input_key | glgfx_input_np_decimal,		/* ? */
//  [KEY_HANGUEL]	= glgfx_input_none,
//  [KEY_HANJA]		= glgfx_input_none,
//  [KEY_YEN]		= glgfx_input_none,
  [KEY_LEFTMETA]	= glgfx_input_key | glgfx_input_lmeta,
  [KEY_RIGHTMETA]	= glgfx_input_key | glgfx_input_rmeta,
  [KEY_COMPOSE]		= glgfx_input_key | glgfx_input_menu,
  [KEY_STOP]		= glgfx_input_key | glgfx_input_ac_stop,
  [KEY_AGAIN]		= glgfx_input_key | glgfx_input_ac_redo,
  [KEY_PROPS]		= glgfx_input_key | glgfx_input_ac_properties,
  [KEY_UNDO]		= glgfx_input_key | glgfx_input_ac_undo,
  [KEY_FRONT]		= glgfx_input_key | glgfx_input_front,
  [KEY_COPY]		= glgfx_input_key | glgfx_input_ac_copy,
  [KEY_OPEN]		= glgfx_input_key | glgfx_input_ac_open,
  [KEY_PASTE]		= glgfx_input_key | glgfx_input_ac_paste,
  [KEY_FIND]		= glgfx_input_key | glgfx_input_ac_find,
  [KEY_CUT]		= glgfx_input_key | glgfx_input_ac_cut,
  [KEY_HELP]		= glgfx_input_key | glgfx_input_help,
  [KEY_MENU]		= glgfx_input_key | glgfx_input_cd_menu,
  [KEY_CALC]		= glgfx_input_key | glgfx_input_al_calculator,
//  [KEY_SETUP]		= glgfx_input_none,
  [KEY_SLEEP]		= glgfx_input_key | glgfx_input_sleep,
  [KEY_WAKEUP]		= glgfx_input_key | glgfx_input_wakeup,
  [KEY_FILE]		= glgfx_input_key | glgfx_input_al_browser,
//  [KEY_SENDFILE]	= glgfx_input_none,
//  [KEY_DELETEFILE]	= glgfx_input_none,
//  [KEY_XFER]		= glgfx_input_none,
//  [KEY_PROG1]		= glgfx_input_none,
//  [KEY_PROG2]		= glgfx_input_none,
  [KEY_WWW]		= glgfx_input_key | glgfx_input_al_internetbrowser,
  [KEY_MSDOS]		= glgfx_input_key | glgfx_input_al_cli,
//  [KEY_COFFEE]	= glgfx_input_none,
//  [KEY_DIRECTION]	= glgfx_input_none,
//  [KEY_CYCLEWINDOWS]	= glgfx_input_none,
  [KEY_MAIL]		= glgfx_input_key | glgfx_input_al_email,
  [KEY_BOOKMARKS]	= glgfx_input_key | glgfx_input_ac_bookmarks,
  [KEY_COMPUTER]	= glgfx_input_key | glgfx_input_al_browser,
  [KEY_BACK]		= glgfx_input_key | glgfx_input_ac_back,
  [KEY_FORWARD]		= glgfx_input_key | glgfx_input_ac_forward,
  [KEY_CLOSECD]		= glgfx_input_key | glgfx_input_cd_close,
  [KEY_EJECTCD]		= glgfx_input_key | glgfx_input_cd_eject,
  [KEY_EJECTCLOSECD]	= glgfx_input_key | glgfx_input_cd_ejectclose,
  [KEY_NEXTSONG]	= glgfx_input_key | glgfx_input_cd_next,
  [KEY_PLAYPAUSE]	= glgfx_input_key | glgfx_input_cd_playpause,
  [KEY_PREVIOUSSONG]	= glgfx_input_key | glgfx_input_cd_prev,
  [KEY_STOPCD]		= glgfx_input_key | glgfx_input_cd_stop,
  [KEY_RECORD]		= glgfx_input_key | glgfx_input_cd_record,
  [KEY_REWIND]		= glgfx_input_key | glgfx_input_cd_rew,
  [KEY_PHONE]		= glgfx_input_key | glgfx_input_al_telephony,
//  [KEY_ISO]		= glgfx_input_none,
  [KEY_CONFIG]		= glgfx_input_key | glgfx_input_al_controlconfig,
  [KEY_HOMEPAGE]	= glgfx_input_key | glgfx_input_ac_home,
  [KEY_REFRESH]		= glgfx_input_key | glgfx_input_ac_refresh,
  [KEY_EXIT]		= glgfx_input_key | glgfx_input_ac_exit,
//  [KEY_MOVE]		= glgfx_input_none,
//  [KEY_EDIT]		= glgfx_input_none,
  [KEY_SCROLLUP]	= glgfx_input_key | glgfx_input_ac_scrollup,
  [KEY_SCROLLDOWN]	= glgfx_input_key | glgfx_input_ac_scrolldown,
  [KEY_KPLEFTPAREN]	= glgfx_input_key | glgfx_input_np_lparen,
  [KEY_KPRIGHTPAREN]	= glgfx_input_key | glgfx_input_np_rparen,
  [KEY_NEW]		= glgfx_input_key | glgfx_input_ac_new,
  [KEY_REDO]		= glgfx_input_key | glgfx_input_ac_redo,
  [KEY_F13]		= glgfx_input_key | glgfx_input_f13,
  [KEY_F14]		= glgfx_input_key | glgfx_input_f14,
  [KEY_F15]		= glgfx_input_key | glgfx_input_f15,
  [KEY_F16]		= glgfx_input_key | glgfx_input_f16,
  [KEY_F17]		= glgfx_input_key | glgfx_input_f17,
  [KEY_F18]		= glgfx_input_key | glgfx_input_f18,
  [KEY_F19]		= glgfx_input_key | glgfx_input_f19,
  [KEY_F20]		= glgfx_input_key | glgfx_input_f20,
  [KEY_F21]		= glgfx_input_key | glgfx_input_f21,
  [KEY_F22]		= glgfx_input_key | glgfx_input_f22,
  [KEY_F23]		= glgfx_input_key | glgfx_input_f23,
  [KEY_F24]		= glgfx_input_key | glgfx_input_f24,
  [KEY_PLAYCD]		= glgfx_input_key | glgfx_input_cd_play,
  [KEY_PAUSECD]		= glgfx_input_key | glgfx_input_cd_pause,
//  [KEY_PROG3]		= glgfx_input_none,
//  [KEY_PROG4]		= glgfx_input_none,
  [KEY_SUSPEND]		= glgfx_input_key | glgfx_input_suspend,
//  [KEY_CLOSE]		= glgfx_input_none,
  [KEY_PLAY]		= glgfx_input_key | glgfx_input_cd_play,		/* ? */
  [KEY_FASTFORWARD]	= glgfx_input_key | glgfx_input_cd_ff,
  [KEY_BASSBOOST]	= glgfx_input_key | glgfx_input_cd_bassboost,
  [KEY_PRINT]		= glgfx_input_key | glgfx_input_ac_print,
//  [KEY_HP]		= glgfx_input_none,
//  [KEY_CAMERA]	= glgfx_input_none,
//  [KEY_SOUND]		= glgfx_input_none,
//  [KEY_QUESTION]	= glgfx_input_none,
  [KEY_EMAIL]		= glgfx_input_key | glgfx_input_al_email,
  [KEY_CHAT]		= glgfx_input_key | glgfx_input_al_networkchat,
  [KEY_SEARCH]		= glgfx_input_key | glgfx_input_ac_search,
//  [KEY_CONNECT]	= glgfx_input_none,
  [KEY_FINANCE]		= glgfx_input_key | glgfx_input_al_finance,
//  [KEY_SPORT]		= glgfx_input_none,
//  [KEY_SHOP]		= glgfx_input_none,
//  [KEY_ALTERASE]	= glgfx_input_none,
//  [KEY_CANCEL]	= glgfx_input_none,
//  [KEY_BRIGHTNESSDOWN]= glgfx_input_none,
//  [KEY_BRIGHTNESSUP]	= glgfx_input_none,
//  [KEY_MEDIA]		= glgfx_input_none,
//  [KEY_UNKNOWN]	= glgfx_input_none,
//  [BTN_MISC]		= glgfx_input_none,
//  [BTN_0]		= glgfx_input_none,
//  [BTN_1]		= glgfx_input_none,
//  [BTN_2]		= glgfx_input_none,
//  [BTN_3]		= glgfx_input_none,
//  [BTN_4]		= glgfx_input_none,
//  [BTN_5]		= glgfx_input_none,
//  [BTN_6]		= glgfx_input_none,
//  [BTN_7]		= glgfx_input_none,
//  [BTN_8]		= glgfx_input_none,
//  [BTN_9]		= glgfx_input_none,
//  [BTN_MOUSE]		= glgfx_input_none,
//  [BTN_LEFT]		= glgfx_input_none,
//  [BTN_RIGHT]		= glgfx_input_none,
//  [BTN_MIDDLE]	= glgfx_input_none,
//  [BTN_SIDE]		= glgfx_input_none,
//  [BTN_EXTRA]		= glgfx_input_none,
//  [BTN_FORWARD]	= glgfx_input_none,
//  [BTN_BACK]		= glgfx_input_none,
//  [BTN_TASK]		= glgfx_input_none,
//  [BTN_JOYSTICK]	= glgfx_input_none,
//  [BTN_TRIGGER]	= glgfx_input_none,
//  [BTN_THUMB]		= glgfx_input_none,
//  [BTN_THUMB2]	= glgfx_input_none,
//  [BTN_TOP]		= glgfx_input_none,
//  [BTN_TOP2]		= glgfx_input_none,
//  [BTN_PINKIE]	= glgfx_input_none,
//  [BTN_BASE]		= glgfx_input_none,
//  [BTN_BASE2]		= glgfx_input_none,
//  [BTN_BASE3]		= glgfx_input_none,
//  [BTN_BASE4]		= glgfx_input_none,
//  [BTN_BASE5]		= glgfx_input_none,
//  [BTN_BASE6]		= glgfx_input_none,
//  [BTN_DEAD]		= glgfx_input_none,
//  [BTN_GAMEPAD]	= glgfx_input_none,
//  [BTN_A]		= glgfx_input_none,
//  [BTN_B]		= glgfx_input_none,
//  [BTN_C]		= glgfx_input_none,
//  [BTN_X]		= glgfx_input_none,
//  [BTN_Y]		= glgfx_input_none,
//  [BTN_Z]		= glgfx_input_none,
//  [BTN_TL]		= glgfx_input_none,
//  [BTN_TR]		= glgfx_input_none,
//  [BTN_TL2]		= glgfx_input_none,
//  [BTN_TR2]		= glgfx_input_none,
//  [BTN_SELECT]	= glgfx_input_none,
//  [BTN_START]		= glgfx_input_none,
//  [BTN_MODE]		= glgfx_input_none,
//  [BTN_THUMBL]	= glgfx_input_none,
//  [BTN_THUMBR]	= glgfx_input_none,
//  [BTN_DIGI]		= glgfx_input_none,
//  [BTN_TOOL_PEN]	= glgfx_input_none,
//  [BTN_TOOL_RUBBER]	= glgfx_input_none,
//  [BTN_TOOL_BRUSH]	= glgfx_input_none,
//  [BTN_TOOL_PENCIL]	= glgfx_input_none,
//  [BTN_TOOL_AIRBRUSH]	= glgfx_input_none,
//  [BTN_TOOL_FINGER]	= glgfx_input_none,
//  [BTN_TOOL_MOUSE]	= glgfx_input_none,
//  [BTN_TOOL_LENS]	= glgfx_input_none,
//  [BTN_TOUCH]		= glgfx_input_none,
//  [BTN_STYLUS]	= glgfx_input_none,
//  [BTN_STYLUS2]	= glgfx_input_none,
//  [BTN_TOOL_DOUBLETAP]= glgfx_input_none,
//  [BTN_TOOL_TRIPLETAP]= glgfx_input_none,
//  [BTN_WHEEL]		= glgfx_input_none,
//  [BTN_GEAR_DOWN]	= glgfx_input_none,
//  [BTN_GEAR_UP]	= glgfx_input_none,
//  [KEY_OK]		= glgfx_input_none,
//  [KEY_SELECT]	= glgfx_input_none,
//  [KEY_GOTO]		= glgfx_input_none,
//  [KEY_CLEAR]		= glgfx_input_none,
//  [KEY_POWER2]	= glgfx_input_none,
//  [KEY_OPTION]	= glgfx_input_none,
//  [KEY_INFO]		= glgfx_input_none,
//  [KEY_TIME]		= glgfx_input_none,
//  [KEY_VENDOR]	= glgfx_input_none,
//  [KEY_ARCHIVE]	= glgfx_input_none,
//  [KEY_PROGRAM]	= glgfx_input_none,
//  [KEY_CHANNEL]	= glgfx_input_none,
//  [KEY_FAVORITES]	= glgfx_input_none,
//  [KEY_EPG]		= glgfx_input_none,
//  [KEY_PVR]		= glgfx_input_none,
//  [KEY_MHP]		= glgfx_input_none,
//  [KEY_LANGUAGE]	= glgfx_input_none,
//  [KEY_TITLE]		= glgfx_input_none,
//  [KEY_SUBTITLE]	= glgfx_input_none,
//  [KEY_ANGLE]		= glgfx_input_none,
//  [KEY_ZOOM]		= glgfx_input_none,
//  [KEY_MODE]		= glgfx_input_none,
//  [KEY_KEYBOARD]	= glgfx_input_none,
//  [KEY_SCREEN]	= glgfx_input_none,
//  [KEY_PC]		= glgfx_input_none,
//  [KEY_TV]		= glgfx_input_none,
//  [KEY_TV2]		= glgfx_input_none,
//  [KEY_VCR]		= glgfx_input_none,
//  [KEY_VCR2]		= glgfx_input_none,
//  [KEY_SAT]		= glgfx_input_none,
//  [KEY_SAT2]		= glgfx_input_none,
//  [KEY_CD]		= glgfx_input_none,
//  [KEY_TAPE]		= glgfx_input_none,
//  [KEY_RADIO]		= glgfx_input_none,
//  [KEY_TUNER]		= glgfx_input_none,
//  [KEY_PLAYER]	= glgfx_input_none,
//  [KEY_TEXT]		= glgfx_input_none,
//  [KEY_DVD]		= glgfx_input_none,
//  [KEY_AUX]		= glgfx_input_none,
//  [KEY_MP3]		= glgfx_input_none,
//  [KEY_AUDIO]		= glgfx_input_none,
//  [KEY_VIDEO]		= glgfx_input_none,
//  [KEY_DIRECTORY]	= glgfx_input_none,
//  [KEY_LIST]		= glgfx_input_none,
//  [KEY_MEMO]		= glgfx_input_none,
//  [KEY_CALENDAR]	= glgfx_input_none,
//  [KEY_RED]		= glgfx_input_none,
//  [KEY_GREEN]		= glgfx_input_none,
//  [KEY_YELLOW]	= glgfx_input_none,
//  [KEY_BLUE]		= glgfx_input_none,
//  [KEY_CHANNELUP]	= glgfx_input_none,
//  [KEY_CHANNELDOWN]	= glgfx_input_none,
//  [KEY_FIRST]		= glgfx_input_none,
//  [KEY_LAST]		= glgfx_input_none,
//  [KEY_AB]		= glgfx_input_none,
//  [KEY_NEXT]		= glgfx_input_none,
//  [KEY_RESTART]	= glgfx_input_none,
//  [KEY_SLOW]		= glgfx_input_none,
//  [KEY_SHUFFLE]	= glgfx_input_none,
//  [KEY_BREAK]		= glgfx_input_none,
//  [KEY_PREVIOUS]	= glgfx_input_none,
//  [KEY_DIGITS]	= glgfx_input_none,
//  [KEY_TEEN]		= glgfx_input_none,
//  [KEY_TWEN]		= glgfx_input_none,
//  [KEY_DEL_EOL]	= glgfx_input_none,
//  [KEY_DEL_EOS]	= glgfx_input_none,
//  [KEY_INS_LINE]	= glgfx_input_none,
//  [KEY_DEL_LINE]	= glgfx_input_none,
  [KEY_SEND]		= glgfx_input_key | glgfx_input_ac_emailsend,
  [KEY_REPLY]		= glgfx_input_key | glgfx_input_ac_emailreply,
  [KEY_FORWARDMAIL]	= glgfx_input_key | glgfx_input_ac_emailforward,
  [KEY_SAVE]		= glgfx_input_key | glgfx_input_ac_save,
//  [KEY_DOCUMENTS]	= glgfx_input_key | glgfx_input_ac_

};

/*** /dev/input/eventX support functions *************************************/


struct input_device {
    int event_number;
    int fd;
    struct input_id id;
};

static gint compare_devices(gconstpointer a, gconstpointer b, gpointer userdata) {
  struct input_device const* device1 = a;
  struct input_device const* device2 = b;
  (void) userdata;

  int64_t id1 = ID_TO_LONG(device1->id);
  int64_t id2 = ID_TO_LONG(device2->id);

  if (id1 < id2) {
    return 1;
  }
  else if (id1 > id2) {
    return -1;
  }
  else {
    return 0;
  }
}


static gint compare_device_to_event_number(gconstpointer a, gconstpointer b) {
  struct input_device const* device = a;
  intptr_t                   event_number     = (intptr_t) b;

  return device->event_number - event_number;
}


static int get_event_number(char const* name) {
  int rc = -1;

  if (strncmp(name, "event", 5) == 0 && name[5] != '\0') {
    long event_number;
    char* endp;

    event_number = strtol(&name[5], &endp, 10);

    if (endp[0] == '\0') {
      rc = (int) event_number;
    }
  }

  return rc;
}


static struct input_device* create_device(int event_number) {
  char full_path[64];

  struct input_device* dev = calloc(1, sizeof (*dev));

  if (dev == NULL) {
    return false;
  }

  dev->event_number = event_number;

  snprintf(full_path, sizeof (full_path), "/dev/input/event%d", dev->event_number);

  dev->fd = open(full_path, O_RDWR | O_NONBLOCK);

  if (dev->fd < 0) {
    perror("open(\"/dev/input/eventX\")");
    free(dev);
    return NULL;
  }

  // Grab events so they never reach the X server
//  ioctl(dev->fd, EVIOCGRAB, 1);

  return dev;
}


static void destroy_device(struct input_device* device) {
  if (device == NULL) {
    return;
  }

  close(device->fd);
  free(device);
}



static bool add_device(GQueue* devices, struct input_device* device) {
  if (device == NULL) {
    return false;
  }

  g_queue_insert_sorted(devices, device, compare_devices, NULL);
  return true;
}


static bool remove_device(GQueue* devices, int event_number) {
  if (devices == NULL) {
    return false;
  }

  GList* link = g_queue_find_custom(devices, (gconstpointer) (intptr_t) event_number,
				    compare_device_to_event_number);

  if (link == NULL) {
    return false;
  }

  struct input_device* device = link->data;

  g_queue_delete_link(devices, link);
  destroy_device(device);

  return true;
}


static void remove_all_devices(GQueue* devices) {
  if (devices != NULL) {
    gpointer data;

    while ((data = g_queue_pop_head(devices)) != NULL) {
      struct input_device* device = data;

      destroy_device(device);
    }
  }
}

static bool rescan_devices(GQueue* devices) {
  DIR* dir;
  bool rc = true;

  remove_all_devices(devices);

  dir = opendir("/dev/input");

  if (dir == NULL) {
    perror("opendir(\"/dev/input\")");
    rc = false;
  }
  else {
    struct dirent* de;

    while ((de = readdir(dir)) != NULL) {
      int event_number = get_event_number(de->d_name);

      if (event_number >= 0) {
	struct input_device* device = create_device(event_number);

	if (device == NULL || !add_device(devices, device)) {
	  rc = false;
	  break;
	}
      }
    }

    closedir(dir);
  }

  if (!rc) {
    remove_all_devices(devices);
  }

  return rc;
}


static bool update_devices(GQueue* devices, int inotify_fd) {
  char buf[1024];
  int rc = true;

  while (true) {
    int len = read(inotify_fd, buf, sizeof (buf));

    if (len < 0) {
      if (errno == EINTR) {
	continue;
      }
      else if (errno == EAGAIN) {
	// Would block
	return true;
      }
      else {
	perror("read");
	return false;
      }
    }
    else if (len == 0) {
      /* buffer too small! */
      return false;
    }
    
    int i = 0;

    while (i < len) {
      struct inotify_event* event = (struct inotify_event*) &buf[i];

      if (event->len > 0) {
	int event_number = get_event_number(event->name);

	if (event_number >= 0) {
	  if (event->mask & IN_CREATE) {
	    struct input_device* device = create_device(event_number);

	    if (device == NULL || !add_device(devices, device)) {
	      rc = false;
	      break;
	    }
	  }
	  
	  if (event->mask & IN_DELETE) {
	    if (!remove_device(devices, event_number)) {
	      rc = false;
	    }
	  }
	}
      }

      i += sizeof (*event) + event->len;
    }
  }

  return rc;
}

static int find_fd_set(GQueue* devices, fd_set* read_fds, int inotify_fd) {
  int max_fd = inotify_fd;

  void dev_func(gpointer data, gpointer userdata) {
    struct input_device* device = data;
    (void) userdata;

    if (device->fd > max_fd) {
      max_fd = device->fd;
    }

    FD_SET(device->fd, read_fds);
  };

  FD_ZERO(read_fds);

  if (inotify_fd >= 0) {
    FD_SET(inotify_fd, read_fds);
  }

  g_queue_foreach(devices, dev_func, NULL);
  
  return max_fd;
}

/*** API functions **********************************************************/

bool glgfx_input_acquire(struct glgfx_monitor* monitor) {
  bool rc = true;

  if (monitor == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  if (pending_queue == NULL) {
    pending_queue = g_queue_new();
  }

  if (cache_queue == NULL) {
    cache_queue = g_queue_new();
  }

  if (pending_queue == NULL || cache_queue == NULL) {
    g_queue_free(pending_queue);
    g_queue_free(cache_queue);

    pending_queue = NULL;
    cache_queue = NULL;

    errno = ENOMEM;
    rc = false;
  }
  else {
    Window rootwin = XRootWindow(monitor->display,
				 DefaultScreen(monitor->display));

    XGrabKeyboard(monitor->display, rootwin, 1, GrabModeAsync,
		  GrabModeAsync,  CurrentTime);
    XGrabPointer(monitor->display, rootwin, 1,
		 PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None,  None, CurrentTime);
#ifdef USE_DGA2
    int num_modes = 0;
    XDGAMode* modes = XDGAQueryModes(monitor->display, DefaultScreen(monitor->display),
				     &num_modes);

    if (modes != NULL && num_modes > 0) {
      XDGADevice* dev = XDGASetMode(monitor->display, DefaultScreen(monitor->display),
				    modes[0].num);
      XFree(dev);
      XFree(modes);
    }

    XDGASelectInput(monitor->display, DefaultScreen(monitor->display),
		    PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
		    KeyPressMask | KeyReleaseMask);
#else
    XF86DGADirectVideo(monitor->display, DefaultScreen(monitor->display),
		       XF86DGADirectMouse | XF86DGADirectKeyb);
#endif

    monitors = g_list_append(monitors, monitor);
  }

  pthread_mutex_unlock(&glgfx_mutex);

  return rc;
}


static void free_event(gpointer data, gpointer userdata) {
  (void) userdata;
  free(data);
}

bool glgfx_input_release(struct glgfx_monitor* monitor) {

  if (monitor == NULL) {
    errno = EINVAL;
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

#ifdef USE_DGA2
  XDGASelectInput(monitor->display, DefaultScreen(monitor->display), 0);
  XDGADevice* dev = XDGASetMode(monitor->display, DefaultScreen(monitor->display), 0);
  XFree(dev);
#else
  XF86DGADirectVideo(monitor->display, DefaultScreen(monitor->display), 0);
#endif
  XUngrabPointer(monitor->display, CurrentTime);
  XUngrabKeyboard(monitor->display, CurrentTime);

  monitors = g_list_remove(monitors, monitor);

  if (monitors == NULL) {
    if (pending_queue != NULL) {
      g_queue_foreach(pending_queue, free_event, NULL);
      g_queue_free(pending_queue);
    }

    if (cache_queue != NULL) {
      g_queue_foreach(cache_queue, free_event, NULL);
      g_queue_free(cache_queue);
    }
  }

  pthread_mutex_unlock(&glgfx_mutex);

  return true;
}


static void filler(gpointer data, gpointer userdata) {
  struct glgfx_monitor* monitor = (struct glgfx_monitor*) data;
  (void) userdata;

  while (XPending(monitor->display)) {
    XEvent* event;

    if (!g_queue_is_empty(cache_queue)) {
      event = g_queue_pop_head(cache_queue);
    }
    else {
      event = calloc(1, sizeof (*event));
    }

    XNextEvent(monitor->display, event);
    g_queue_push_tail(pending_queue, event);
  }
}

static void fill_queue(void) {
  g_list_foreach(monitors, filler, NULL);
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
