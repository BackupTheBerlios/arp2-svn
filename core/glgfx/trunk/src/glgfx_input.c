
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


#ifndef KEY_NEW
# define KEY_NEW         181
#endif
#ifndef KEY_REDO
# define KEY_REDO        182
#endif
#ifndef KEY_SEND
# define KEY_SEND        231
#endif
#ifndef KEY_REPLY
# define KEY_REPLY       232
#endif
#ifndef KEY_FORWARDMAIL
# define KEY_FORWARDMAIL 233
#endif
#ifndef KEY_SAVE
# define KEY_SAVE        234
#endif
#ifndef KEY_DOCUMENTS
# define KEY_DOCUMENTS   235
#endif

static GList* monitors = NULL;
static GQueue* pending_queue = NULL;
static GQueue* cache_queue = NULL;

/*** Event mappings **********************************************************/

static enum glgfx_event_code xorg_codes[256] = {
  // Row 0: 0-
  glgfx_event_none,	glgfx_event_none,	glgfx_event_none,	glgfx_event_none,
  glgfx_event_none,	glgfx_event_none,	glgfx_event_none,	glgfx_event_none,
  glgfx_event_none,

  // Row 1: 9-
  glgfx_event_escape,	glgfx_event_1,  	glgfx_event_2,  	glgfx_event_3,
  glgfx_event_4,     	glgfx_event_5,  	glgfx_event_6,  	glgfx_event_7,
  glgfx_event_8,     	glgfx_event_9,  	glgfx_event_0,  	glgfx_event_minus,
  glgfx_event_equal, 	glgfx_event_backspace,

  // Row 2: 23-
  glgfx_event_tab,      glgfx_event_q,       	glgfx_event_w,  	glgfx_event_e,
  glgfx_event_r,        glgfx_event_t,       	glgfx_event_y,  	glgfx_event_u,
  glgfx_event_i,        glgfx_event_o,       	glgfx_event_p,  	glgfx_event_lbracket,
  glgfx_event_rbracket, glgfx_event_return,

  // Row 3: 37-
  glgfx_event_lctrl,    glgfx_event_a,  	glgfx_event_s,         	glgfx_event_d,
  glgfx_event_f,        glgfx_event_g,  	glgfx_event_h,          glgfx_event_j,
  glgfx_event_k,        glgfx_event_l,  	glgfx_event_semicolon,  glgfx_event_quote,
  glgfx_event_backquote,

  // Row 4: 50-
  glgfx_event_lshift,  	glgfx_event_backslash,  glgfx_event_z,       	glgfx_event_x,
  glgfx_event_c,       	glgfx_event_v,          glgfx_event_b,       	glgfx_event_n,
  glgfx_event_m,       	glgfx_event_comma,      glgfx_event_period,  	glgfx_event_slash,
  glgfx_event_rshift,  	glgfx_event_np_mul,

  // Row 5: 64-
  glgfx_event_lalt,  	glgfx_event_space,  	glgfx_event_capslock,  	glgfx_event_f1,
  glgfx_event_f2,  	glgfx_event_f3,  	glgfx_event_f4,  	glgfx_event_f5,
  glgfx_event_f6,  	glgfx_event_f7,  	glgfx_event_f8,  	glgfx_event_f9,
  glgfx_event_f10,

  // Keypad
  glgfx_event_numlock,  glgfx_event_scrlock,  	glgfx_event_np_7,  	glgfx_event_np_8,
  glgfx_event_np_9,  	glgfx_event_np_sub,  	glgfx_event_np_4,  	glgfx_event_np_5,
  glgfx_event_np_6,  	glgfx_event_np_add,  	glgfx_event_np_1,  	glgfx_event_np_2,
  glgfx_event_np_3,  	glgfx_event_np_0,  	glgfx_event_np_decimal,

  // Extra
  glgfx_event_none,  	glgfx_event_none,  	glgfx_event_ltgt,  	glgfx_event_f11,
  glgfx_event_f12, 	glgfx_event_home,  	glgfx_event_up,  	glgfx_event_pageup,
  glgfx_event_left,  	glgfx_event_none,  	glgfx_event_right,  	glgfx_event_end,
  glgfx_event_down,  	glgfx_event_pagedown,  	glgfx_event_insert,  	glgfx_event_delete,
  glgfx_event_np_enter,	glgfx_event_rctrl,  	glgfx_event_pause,  	glgfx_event_prtscr,
  glgfx_event_np_div,  	glgfx_event_ralt,  	glgfx_event_none,   	glgfx_event_lgui,
  glgfx_event_rgui,  	glgfx_event_menu,       // None: 92-93, 101, 114

/*   glgfx_event_np_lparen, */
/*   glgfx_event_np_rparen, */
/*   glgfx_event_numbersign = 0x2b, */
};

static enum glgfx_event_code linux_codes[KEY_MAX + 1] = {
  [0 ... KEY_MAX]	= glgfx_event_none,

  [KEY_ESC]		= glgfx_event_escape,
  [KEY_1]		= glgfx_event_1,
  [KEY_2]		= glgfx_event_2,
  [KEY_3]		= glgfx_event_3,
  [KEY_4]		= glgfx_event_4,
  [KEY_5]		= glgfx_event_5,
  [KEY_6]		= glgfx_event_6,
  [KEY_7]		= glgfx_event_7,
  [KEY_8]		= glgfx_event_8,
  [KEY_9]		= glgfx_event_9,
  [KEY_0]		= glgfx_event_0,
  [KEY_MINUS]		= glgfx_event_minus,
  [KEY_EQUAL]		= glgfx_event_equal,
  [KEY_BACKSPACE]	= glgfx_event_backspace,
  [KEY_TAB]		= glgfx_event_tab,
  [KEY_Q]		= glgfx_event_q,
  [KEY_W]		= glgfx_event_w,
  [KEY_E]		= glgfx_event_e,
  [KEY_R]		= glgfx_event_r,
  [KEY_T]		= glgfx_event_t,
  [KEY_Y]		= glgfx_event_y,
  [KEY_U]		= glgfx_event_u,
  [KEY_I]		= glgfx_event_i,
  [KEY_O]		= glgfx_event_o,
  [KEY_P]		= glgfx_event_p,
  [KEY_LEFTBRACE]	= glgfx_event_lbracket,
  [KEY_RIGHTBRACE]	= glgfx_event_rbracket,
  [KEY_ENTER]		= glgfx_event_return,
  [KEY_LEFTCTRL]	= glgfx_event_lctrl,
  [KEY_A]		= glgfx_event_a,
  [KEY_S]		= glgfx_event_s,
  [KEY_D]		= glgfx_event_d,
  [KEY_F]		= glgfx_event_f,
  [KEY_G]		= glgfx_event_g,
  [KEY_H]		= glgfx_event_h,
  [KEY_J]		= glgfx_event_j,
  [KEY_K]		= glgfx_event_k,
  [KEY_L]		= glgfx_event_l,
  [KEY_SEMICOLON]	= glgfx_event_semicolon,
  [KEY_APOSTROPHE]	= glgfx_event_quote,
  [KEY_GRAVE]		= glgfx_event_backquote,
  [KEY_LEFTSHIFT]	= glgfx_event_lshift,
  [KEY_BACKSLASH]	= glgfx_event_backslash,
  [KEY_Z]		= glgfx_event_z,
  [KEY_X]		= glgfx_event_x,
  [KEY_C]		= glgfx_event_c,
  [KEY_V]		= glgfx_event_v,
  [KEY_B]		= glgfx_event_b,
  [KEY_N]		= glgfx_event_n,
  [KEY_M]		= glgfx_event_m,
  [KEY_COMMA]		= glgfx_event_comma,
  [KEY_DOT]		= glgfx_event_period,
  [KEY_SLASH]		= glgfx_event_slash,
  [KEY_RIGHTSHIFT]	= glgfx_event_rshift,
  [KEY_KPASTERISK]	= glgfx_event_np_mul,
  [KEY_LEFTALT]		= glgfx_event_lalt,
  [KEY_SPACE]		= glgfx_event_space,
  [KEY_CAPSLOCK]	= glgfx_event_capslock,
  [KEY_F1]		= glgfx_event_f1,
  [KEY_F2]		= glgfx_event_f2,
  [KEY_F3]		= glgfx_event_f3,
  [KEY_F4]		= glgfx_event_f4,
  [KEY_F5]		= glgfx_event_f5,
  [KEY_F6]		= glgfx_event_f6,
  [KEY_F7]		= glgfx_event_f7,
  [KEY_F8]		= glgfx_event_f8,
  [KEY_F9]		= glgfx_event_f9,
  [KEY_F10]		= glgfx_event_f10,
  [KEY_NUMLOCK]		= glgfx_event_numlock,
  [KEY_SCROLLLOCK]	= glgfx_event_scrlock,
  [KEY_KP7]		= glgfx_event_np_7,
  [KEY_KP8]		= glgfx_event_np_8,
  [KEY_KP9]		= glgfx_event_np_9,
  [KEY_KPMINUS]		= glgfx_event_np_sub,
  [KEY_KP4]		= glgfx_event_np_4,
  [KEY_KP5]		= glgfx_event_np_5,
  [KEY_KP6]		= glgfx_event_np_6,
  [KEY_KPPLUS]		= glgfx_event_np_add,
  [KEY_KP1]		= glgfx_event_np_1,
  [KEY_KP2]		= glgfx_event_np_2,
  [KEY_KP3]		= glgfx_event_np_3,
  [KEY_KP0]		= glgfx_event_np_0,
  [KEY_KPDOT]		= glgfx_event_np_decimal,
//  [KEY_ZENKAKUHANKAKU]= glgfx_event_none,
  [KEY_102ND]		= glgfx_event_ltgt,
  [KEY_F11]		= glgfx_event_f11,
  [KEY_F12]		= glgfx_event_f12,
//  [KEY_RO]		= glgfx_event_none,
//  [KEY_KATAKANA]	= glgfx_event_none,
//  [KEY_HIRAGANA]	= glgfx_event_none,
//  [KEY_HENKAN]	= glgfx_event_none,
//  [KEY_KATAKANAHIRAGANA]= glgfx_event_none,
//  [KEY_MUHENKAN]	= glgfx_event_none,
//  [KEY_KPJPCOMMA]	= glgfx_event_none,
  [KEY_KPENTER]		= glgfx_event_np_enter,
  [KEY_RIGHTCTRL]	= glgfx_event_rctrl,
  [KEY_KPSLASH]		= glgfx_event_np_div,
  [KEY_SYSRQ]		= glgfx_event_prtscr,
  [KEY_RIGHTALT]	= glgfx_event_ralt,
//  [KEY_LINEFEED]	= glgfx_event_none,
  [KEY_HOME]		= glgfx_event_home,
  [KEY_UP]		= glgfx_event_up,
  [KEY_PAGEUP]		= glgfx_event_pageup,
  [KEY_LEFT]		= glgfx_event_left,
  [KEY_RIGHT]		= glgfx_event_right,
  [KEY_END]		= glgfx_event_end,
  [KEY_DOWN]		= glgfx_event_down,
  [KEY_PAGEDOWN]	= glgfx_event_pagedown,
  [KEY_INSERT]		= glgfx_event_insert,
  [KEY_DELETE]		= glgfx_event_delete,
//  [KEY_MACRO]		= glgfx_event_none,
  [KEY_MUTE]		= glgfx_event_cd_mute,
  [KEY_VOLUMEDOWN]	= glgfx_event_cd_volumedown,
  [KEY_VOLUMEUP]	= glgfx_event_cd_volumeup,
  [KEY_POWER]		= glgfx_event_poweroff,
  [KEY_KPEQUAL]		= glgfx_event_np_equal,
  [KEY_KPPLUSMINUS]	= glgfx_event_np_plusminus,
  [KEY_PAUSE]		= glgfx_event_pause,
  [KEY_KPCOMMA]		= glgfx_event_np_decimal,		/* ? */
//  [KEY_HANGUEL]	= glgfx_event_none,
//  [KEY_HANJA]		= glgfx_event_none,
//  [KEY_YEN]		= glgfx_event_none,
  [KEY_LEFTMETA]	= glgfx_event_lgui,
  [KEY_RIGHTMETA]	= glgfx_event_rgui,
  [KEY_COMPOSE]		= glgfx_event_menu,
  [KEY_STOP]		= glgfx_event_ac_stop,
  [KEY_AGAIN]		= glgfx_event_ac_redo,
  [KEY_PROPS]		= glgfx_event_ac_properties,
  [KEY_UNDO]		= glgfx_event_ac_undo,
  [KEY_FRONT]		= glgfx_event_front,
  [KEY_COPY]		= glgfx_event_ac_copy,
  [KEY_OPEN]		= glgfx_event_ac_open,
  [KEY_PASTE]		= glgfx_event_ac_paste,
  [KEY_FIND]		= glgfx_event_ac_find,
  [KEY_CUT]		= glgfx_event_ac_cut,
  [KEY_HELP]		= glgfx_event_help,
  [KEY_CALC]		= glgfx_event_al_calculator,
//  [KEY_SETUP]		= glgfx_event_none,
  [KEY_SLEEP]		= glgfx_event_sleep,
  [KEY_WAKEUP]		= glgfx_event_wakeup,
  [KEY_FILE]		= glgfx_event_al_browser,
//  [KEY_SENDFILE]	= glgfx_event_none,
//  [KEY_DELETEFILE]	= glgfx_event_none,
//  [KEY_XFER]		= glgfx_event_none,
//  [KEY_PROG1]		= glgfx_event_none,
//  [KEY_PROG2]		= glgfx_event_none,
  [KEY_WWW]		= glgfx_event_al_internetbrowser,
  [KEY_MSDOS]		= glgfx_event_al_cli,
//  [KEY_COFFEE]	= glgfx_event_none,
//  [KEY_DIRECTION]	= glgfx_event_none,
//  [KEY_CYCLEWINDOWS]	= glgfx_event_none,
  [KEY_MAIL]		= glgfx_event_al_email,
  [KEY_BOOKMARKS]	= glgfx_event_ac_bookmarks,
  [KEY_COMPUTER]	= glgfx_event_al_browser,
  [KEY_BACK]		= glgfx_event_ac_back,
  [KEY_FORWARD]		= glgfx_event_ac_forward,
  [KEY_EJECTCD]		= glgfx_event_cd_eject,
  [KEY_NEXTSONG]	= glgfx_event_cd_next,
  [KEY_PLAYPAUSE]	= glgfx_event_cd_playpause,
  [KEY_PREVIOUSSONG]	= glgfx_event_cd_prev,
  [KEY_STOPCD]		= glgfx_event_cd_stop,
  [KEY_RECORD]		= glgfx_event_cd_record,
  [KEY_REWIND]		= glgfx_event_cd_rew,
  [KEY_PHONE]		= glgfx_event_al_telephony,
//  [KEY_ISO]		= glgfx_event_none,
  [KEY_CONFIG]		= glgfx_event_al_controlconfig,
  [KEY_HOMEPAGE]	= glgfx_event_ac_home,
  [KEY_REFRESH]		= glgfx_event_ac_refresh,
  [KEY_EXIT]		= glgfx_event_ac_exit,
//  [KEY_MOVE]		= glgfx_event_none,
//  [KEY_EDIT]		= glgfx_event_none,
  [KEY_SCROLLUP]	= glgfx_event_ac_scrollup,
  [KEY_SCROLLDOWN]	= glgfx_event_ac_scrolldown,
  [KEY_KPLEFTPAREN]	= glgfx_event_np_lparen,
  [KEY_KPRIGHTPAREN]	= glgfx_event_np_rparen,
  [KEY_NEW]		= glgfx_event_ac_new,
  [KEY_REDO]		= glgfx_event_ac_redo,
  [KEY_F13]		= glgfx_event_f13,
  [KEY_F14]		= glgfx_event_f14,
  [KEY_F15]		= glgfx_event_f15,
  [KEY_F16]		= glgfx_event_f16,
  [KEY_F17]		= glgfx_event_f17,
  [KEY_F18]		= glgfx_event_f18,
  [KEY_F19]		= glgfx_event_f19,
  [KEY_F20]		= glgfx_event_f20,
  [KEY_F21]		= glgfx_event_f21,
  [KEY_F22]		= glgfx_event_f22,
  [KEY_F23]		= glgfx_event_f23,
  [KEY_F24]		= glgfx_event_f24,
  [KEY_PLAYCD]		= glgfx_event_cd_play,
  [KEY_PAUSECD]		= glgfx_event_cd_pause,
//  [KEY_PROG3]		= glgfx_event_none,
//  [KEY_PROG4]		= glgfx_event_none,
  [KEY_SUSPEND]		= glgfx_event_sleep,
//  [KEY_CLOSE]		= glgfx_event_none,
  [KEY_PLAY]		= glgfx_event_cd_play,		/* ? */
  [KEY_FASTFORWARD]	= glgfx_event_cd_ff,
  [KEY_BASSBOOST]	= glgfx_event_cd_bassboost,
  [KEY_PRINT]		= glgfx_event_ac_print,
//  [KEY_HP]		= glgfx_event_none,
//  [KEY_CAMERA]	= glgfx_event_none,
//  [KEY_SOUND]		= glgfx_event_none,
//  [KEY_QUESTION]	= glgfx_event_none,
  [KEY_EMAIL]		= glgfx_event_al_email,
  [KEY_CHAT]		= glgfx_event_al_networkchat,
  [KEY_SEARCH]		= glgfx_event_ac_search,
//  [KEY_CONNECT]	= glgfx_event_none,
  [KEY_FINANCE]		= glgfx_event_al_finance,
//  [KEY_SPORT]		= glgfx_event_none,
//  [KEY_SHOP]		= glgfx_event_none,
//  [KEY_ALTERASE]	= glgfx_event_none,
//  [KEY_CANCEL]	= glgfx_event_none,
//  [KEY_BRIGHTNESSDOWN]= glgfx_event_none,
//  [KEY_BRIGHTNESSUP]	= glgfx_event_none,
//  [KEY_MEDIA]		= glgfx_event_none,
//  [KEY_UNKNOWN]	= glgfx_event_none,
//  [BTN_MISC]		= glgfx_event_none,
//  [BTN_0]		= glgfx_event_none,
//  [BTN_1]		= glgfx_event_none,
//  [BTN_2]		= glgfx_event_none,
//  [BTN_3]		= glgfx_event_none,
//  [BTN_4]		= glgfx_event_none,
//  [BTN_5]		= glgfx_event_none,
//  [BTN_6]		= glgfx_event_none,
//  [BTN_7]		= glgfx_event_none,
//  [BTN_8]		= glgfx_event_none,
//  [BTN_9]		= glgfx_event_none,
//  [BTN_MOUSE]		= glgfx_event_none,
//  [BTN_LEFT]		= glgfx_event_none,
//  [BTN_RIGHT]		= glgfx_event_none,
//  [BTN_MIDDLE]	= glgfx_event_none,
//  [BTN_SIDE]		= glgfx_event_none,
//  [BTN_EXTRA]		= glgfx_event_none,
//  [BTN_FORWARD]	= glgfx_event_none,
//  [BTN_BACK]		= glgfx_event_none,
//  [BTN_TASK]		= glgfx_event_none,
//  [BTN_JOYSTICK]	= glgfx_event_none,
//  [BTN_TRIGGER]	= glgfx_event_none,
//  [BTN_THUMB]		= glgfx_event_none,
//  [BTN_THUMB2]	= glgfx_event_none,
//  [BTN_TOP]		= glgfx_event_none,
//  [BTN_TOP2]		= glgfx_event_none,
//  [BTN_PINKIE]	= glgfx_event_none,
//  [BTN_BASE]		= glgfx_event_none,
//  [BTN_BASE2]		= glgfx_event_none,
//  [BTN_BASE3]		= glgfx_event_none,
//  [BTN_BASE4]		= glgfx_event_none,
//  [BTN_BASE5]		= glgfx_event_none,
//  [BTN_BASE6]		= glgfx_event_none,
//  [BTN_DEAD]		= glgfx_event_none,
//  [BTN_GAMEPAD]	= glgfx_event_none,
//  [BTN_A]		= glgfx_event_none,
//  [BTN_B]		= glgfx_event_none,
//  [BTN_C]		= glgfx_event_none,
//  [BTN_X]		= glgfx_event_none,
//  [BTN_Y]		= glgfx_event_none,
//  [BTN_Z]		= glgfx_event_none,
//  [BTN_TL]		= glgfx_event_none,
//  [BTN_TR]		= glgfx_event_none,
//  [BTN_TL2]		= glgfx_event_none,
//  [BTN_TR2]		= glgfx_event_none,
//  [BTN_SELECT]	= glgfx_event_none,
//  [BTN_START]		= glgfx_event_none,
//  [BTN_MODE]		= glgfx_event_none,
//  [BTN_THUMBL]	= glgfx_event_none,
//  [BTN_THUMBR]	= glgfx_event_none,
//  [BTN_DIGI]		= glgfx_event_none,
//  [BTN_TOOL_PEN]	= glgfx_event_none,
//  [BTN_TOOL_RUBBER]	= glgfx_event_none,
//  [BTN_TOOL_BRUSH]	= glgfx_event_none,
//  [BTN_TOOL_PENCIL]	= glgfx_event_none,
//  [BTN_TOOL_AIRBRUSH]	= glgfx_event_none,
//  [BTN_TOOL_FINGER]	= glgfx_event_none,
//  [BTN_TOOL_MOUSE]	= glgfx_event_none,
//  [BTN_TOOL_LENS]	= glgfx_event_none,
//  [BTN_TOUCH]		= glgfx_event_none,
//  [BTN_STYLUS]	= glgfx_event_none,
//  [BTN_STYLUS2]	= glgfx_event_none,
//  [BTN_TOOL_DOUBLETAP]= glgfx_event_none,
//  [BTN_TOOL_TRIPLETAP]= glgfx_event_none,
//  [BTN_WHEEL]		= glgfx_event_none,
//  [BTN_GEAR_DOWN]	= glgfx_event_none,
//  [BTN_GEAR_UP]	= glgfx_event_none,
//  [KEY_OK]		= glgfx_event_none,
//  [KEY_SELECT]	= glgfx_event_none,
//  [KEY_GOTO]		= glgfx_event_none,
//  [KEY_CLEAR]		= glgfx_event_none,
//  [KEY_POWER2]	= glgfx_event_none,
//  [KEY_OPTION]	= glgfx_event_none,
//  [KEY_INFO]		= glgfx_event_none,
//  [KEY_TIME]		= glgfx_event_none,
//  [KEY_VENDOR]	= glgfx_event_none,
//  [KEY_ARCHIVE]	= glgfx_event_none,
//  [KEY_PROGRAM]	= glgfx_event_none,
//  [KEY_CHANNEL]	= glgfx_event_none,
//  [KEY_FAVORITES]	= glgfx_event_none,
//  [KEY_EPG]		= glgfx_event_none,
//  [KEY_PVR]		= glgfx_event_none,
//  [KEY_MHP]		= glgfx_event_none,
//  [KEY_LANGUAGE]	= glgfx_event_none,
//  [KEY_TITLE]		= glgfx_event_none,
//  [KEY_SUBTITLE]	= glgfx_event_none,
//  [KEY_ANGLE]		= glgfx_event_none,
//  [KEY_ZOOM]		= glgfx_event_none,
//  [KEY_MODE]		= glgfx_event_none,
//  [KEY_KEYBOARD]	= glgfx_event_none,
//  [KEY_SCREEN]	= glgfx_event_none,
//  [KEY_PC]		= glgfx_event_none,
//  [KEY_TV]		= glgfx_event_none,
//  [KEY_TV2]		= glgfx_event_none,
//  [KEY_VCR]		= glgfx_event_none,
//  [KEY_VCR2]		= glgfx_event_none,
//  [KEY_SAT]		= glgfx_event_none,
//  [KEY_SAT2]		= glgfx_event_none,
//  [KEY_CD]		= glgfx_event_none,
//  [KEY_TAPE]		= glgfx_event_none,
//  [KEY_RADIO]		= glgfx_event_none,
//  [KEY_TUNER]		= glgfx_event_none,
//  [KEY_PLAYER]	= glgfx_event_none,
//  [KEY_TEXT]		= glgfx_event_none,
//  [KEY_DVD]		= glgfx_event_none,
//  [KEY_AUX]		= glgfx_event_none,
//  [KEY_MP3]		= glgfx_event_none,
//  [KEY_AUDIO]		= glgfx_event_none,
//  [KEY_VIDEO]		= glgfx_event_none,
//  [KEY_DIRECTORY]	= glgfx_event_none,
//  [KEY_LIST]		= glgfx_event_none,
//  [KEY_MEMO]		= glgfx_event_none,
//  [KEY_CALENDAR]	= glgfx_event_none,
//  [KEY_RED]		= glgfx_event_none,
//  [KEY_GREEN]		= glgfx_event_none,
//  [KEY_YELLOW]	= glgfx_event_none,
//  [KEY_BLUE]		= glgfx_event_none,
//  [KEY_CHANNELUP]	= glgfx_event_none,
//  [KEY_CHANNELDOWN]	= glgfx_event_none,
//  [KEY_FIRST]		= glgfx_event_none,
//  [KEY_LAST]		= glgfx_event_none,
//  [KEY_AB]		= glgfx_event_none,
//  [KEY_NEXT]		= glgfx_event_none,
//  [KEY_RESTART]	= glgfx_event_none,
//  [KEY_SLOW]		= glgfx_event_none,
//  [KEY_SHUFFLE]	= glgfx_event_none,
//  [KEY_BREAK]		= glgfx_event_none,
//  [KEY_PREVIOUS]	= glgfx_event_none,
//  [KEY_DIGITS]	= glgfx_event_none,
//  [KEY_TEEN]		= glgfx_event_none,
//  [KEY_TWEN]		= glgfx_event_none,
//  [KEY_DEL_EOL]	= glgfx_event_none,
//  [KEY_DEL_EOS]	= glgfx_event_none,
//  [KEY_INS_LINE]	= glgfx_event_none,
//  [KEY_DEL_LINE]	= glgfx_event_none,
  [KEY_SEND]		= glgfx_event_ac_msgsend,
  [KEY_REPLY]		= glgfx_event_ac_msgreply,
  [KEY_FORWARDMAIL]	= glgfx_event_ac_msgforward,
  [KEY_SAVE]		= glgfx_event_ac_save,
//  [KEY_DOCUMENTS]	= glgfx_event_ac_

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


bool glgfx_input_getcode(struct glgfx_input_event* event) {
  bool rc = false;
  event->class = glgfx_input_none;

  if (g_queue_is_empty(pending_queue)) {
    // For fairness, we scan all displays in order
    fill_queue();
  }

  if (!g_queue_is_empty(pending_queue)) {
    XEvent* xevent = g_queue_pop_head(pending_queue);

    // Return event to cache queue
    g_queue_push_tail(cache_queue, xevent);

    switch (xevent->type) {
      case KeyPress:
      case KeyRelease: {
	XKeyEvent* ev = (XKeyEvent*) xevent;

	if (ev->keycode < 256) {
	  event->class = (glgfx_input_event | 
			  (xevent->type == KeyRelease ? glgfx_input_releasemask : 0));
	  event->event_code = xorg_codes[ev->keycode];

//	  if (ev->altmask is true
	  rc = true;
	}
	else {
	  return glgfx_input_getcode(event);
	}
	break;
      }

      case ButtonPress:
      case ButtonRelease: {
	XButtonEvent* ev = (XButtonEvent*) xevent;
	event->class = (glgfx_input_event |
			(xevent->type == ButtonRelease ? glgfx_input_releasemask : 0));

	switch (ev->button) {
	  case 1:
	    event->event_code = glgfx_event_page_buttons | 0;
	    break;

	  case 2:
	    event->event_code = glgfx_event_page_buttons | 2;
	    break;

	  case 3:
	    event->event_code = glgfx_event_page_buttons | 1;
	    break;

/* 	  case 4: */
/* 	    if (event->type == ButtonPress) { */
/* 	      code |= glgfx_input_mouse_vwheel | (+16 & 0xffff); */
/* 	    } */
/* 	    else { */
/* 	      return glgfx_input_getcode(); */
/* 	    } */
/* 	    break; */

/* 	  case 5: */
/* 	    if (event->type == ButtonPress) { */
/* 	      code |= glgfx_input_mouse_vwheel | (-16 & 0xffff); */
/* 	    } */
/* 	    else { */
/* 	      return glgfx_input_getcode(); */
/* 	    } */
/* 	    break; */

	  default:
	    return glgfx_input_getcode(event);
	}

	rc = true;
	break;
      }

      case MotionNotify: {
	XMotionEvent* ev = (XMotionEvent*) xevent;
	int dx = ev->x_root;
	int dy = ev->y_root;

	event->class = glgfx_input_mouse;
	event->mouse.dx = dx & 0xffff;
	event->mouse.dy = dy & 0xffff;
	rc = true;
	break;
      }

      default:
	return glgfx_input_getcode(event);
    }
  }

  return rc;
}
