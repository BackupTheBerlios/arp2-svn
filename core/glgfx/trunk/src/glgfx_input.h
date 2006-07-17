#ifndef ARP2_glgfx_glgfx_input_h
#define ARP2_glgfx_glgfx_input_h

#include <glgfx.h>

struct glgfx_monitor;

enum glgfx_event_code {
  // Page tables

  glgfx_event_page_desktop	= 0x0001 << 16,				// Desktop
  glgfx_event_page_keyboard	= 0x0007 << 16,				// Keyboard/keypad
  glgfx_event_page_buttons	= 0x0009 << 16,				// Mouse button
  glgfx_event_page_consumer	= 0x000c << 16,				// Consumer

  glgfx_event_pagemask          = 0xffff << 16,
  glgfx_event_none		= 0,

  // ***** Desktop events *****

  glgfx_event_poweroff		= glgfx_event_page_desktop | 0x81,
  glgfx_event_sleep,
  glgfx_event_wakeup,

  glgfx_event_coldboot		= glgfx_event_page_desktop | 0x8e,
  glgfx_event_warmboot,

  glgfx_event_dock		= glgfx_event_page_desktop | 0xa0,
  glgfx_event_undock,
  glgfx_event_setup,
  glgfx_event_break,

  glgfx_event_hibernate		= glgfx_event_page_desktop | 0xa8,


  // ***** Keyboard/keypad keys *****

  glgfx_event_a			= glgfx_event_page_keyboard | 0x04,
  glgfx_event_b,
  glgfx_event_c,
  glgfx_event_d,
  glgfx_event_e,
  glgfx_event_f,
  glgfx_event_g,
  glgfx_event_h,
  glgfx_event_i,
  glgfx_event_j,
  glgfx_event_k,
  glgfx_event_l,
  glgfx_event_m,
  glgfx_event_n,
  glgfx_event_o,
  glgfx_event_p,
  glgfx_event_q,
  glgfx_event_r,
  glgfx_event_s,
  glgfx_event_t,
  glgfx_event_u,
  glgfx_event_v,
  glgfx_event_w,
  glgfx_event_x,
  glgfx_event_y,
  glgfx_event_z,

  glgfx_event_1			= glgfx_event_page_keyboard | 0x1e,
  glgfx_event_2,
  glgfx_event_3,
  glgfx_event_4,
  glgfx_event_5,
  glgfx_event_6,
  glgfx_event_7,
  glgfx_event_8,
  glgfx_event_9,
  glgfx_event_0,

  glgfx_event_return		= glgfx_event_page_keyboard | 0x28,
  glgfx_event_escape,
  glgfx_event_backspace,
  glgfx_event_tab,
  glgfx_event_space,

  glgfx_event_minus		= glgfx_event_page_keyboard | 0x2d,
  glgfx_event_equal,
  glgfx_event_lbracket,
  glgfx_event_rbracket,
  glgfx_event_backslash,
  glgfx_event_numbersign,
  glgfx_event_semicolon,
  glgfx_event_quote,
  glgfx_event_backquote,
  glgfx_event_comma,
  glgfx_event_period,
  glgfx_event_slash,

  glgfx_event_capslock		= glgfx_event_page_keyboard | 0x39,

  glgfx_event_f1		= glgfx_event_page_keyboard | 0x3a,
  glgfx_event_f2,
  glgfx_event_f3,
  glgfx_event_f4,
  glgfx_event_f5,
  glgfx_event_f6,
  glgfx_event_f7,
  glgfx_event_f8,
  glgfx_event_f9,
  glgfx_event_f10,
  glgfx_event_f11,
  glgfx_event_f12,

  glgfx_event_prtscr		= glgfx_event_page_keyboard | 0x46,
  glgfx_event_scrlock,
  glgfx_event_pause,

  glgfx_event_insert		= glgfx_event_page_keyboard | 0x49,
  glgfx_event_home,
  glgfx_event_pageup,
  glgfx_event_delete,
  glgfx_event_end,
  glgfx_event_pagedown,

  glgfx_event_right		= glgfx_event_page_keyboard | 0x49,
  glgfx_event_left,
  glgfx_event_down,
  glgfx_event_up,

  glgfx_event_numlock		= glgfx_event_page_keyboard | 0x53,	// NumLock or Clear

  glgfx_event_np_div		= glgfx_event_page_keyboard | 0x54,
  glgfx_event_np_mul,
  glgfx_event_np_sub,
  glgfx_event_np_add,
  glgfx_event_np_enter,
  glgfx_event_np_1,
  glgfx_event_np_2,
  glgfx_event_np_3,
  glgfx_event_np_4,
  glgfx_event_np_5,
  glgfx_event_np_6,
  glgfx_event_np_7,
  glgfx_event_np_8,
  glgfx_event_np_9,
  glgfx_event_np_0,
  glgfx_event_np_decimal,

  glgfx_event_ltgt		= glgfx_event_page_keyboard | 0x64,

  glgfx_event_menu		= glgfx_event_page_keyboard | 0x65,	// Menu, App, Compose

  // 0x66 unused

  glgfx_event_np_equal		= glgfx_event_page_keyboard | 0x67,

  glgfx_event_f13		= glgfx_event_page_keyboard | 0x68,
  glgfx_event_f14,
  glgfx_event_f15,
  glgfx_event_f16,
  glgfx_event_f17,
  glgfx_event_f18,
  glgfx_event_f19,
  glgfx_event_f20,
  glgfx_event_f21,
  glgfx_event_f22,
  glgfx_event_f23,
  glgfx_event_f24,

  // 0x74 unused

  glgfx_event_help		= glgfx_event_page_keyboard | 0x75,
  glgfx_event_front,							// On Sun keyboards
  
  // 0x77 -- 0x99 unused
  glgfx_event_sysrq 		= glgfx_event_page_keyboard | 0x9a,	// LAlt+PrtScr

  // 0x9b -- 0xb5 unused

  glgfx_event_np_lparen		= glgfx_event_page_keyboard | 0xb6,
  glgfx_event_np_rparen,

  // 0xb7 -- 0xd6 unused

  glgfx_event_np_plusminus	= glgfx_event_page_keyboard | 0xd7,

  // 0xd8 -- 0xdf unused

  glgfx_event_lctrl		= glgfx_event_page_keyboard | 0xe0,
  glgfx_event_lshift,
  glgfx_event_lalt,
  glgfx_event_lgui,
  glgfx_event_rctrl,
  glgfx_event_rshift,
  glgfx_event_ralt,
  glgfx_event_rgui,

  // 0xe8 -- 0xffff unused


  // ***** Mouse button events ******

  glgfx_event_btn_left		= glgfx_event_page_buttons | 0x00,
  glgfx_event_btn_right		= glgfx_event_page_buttons | 0x01,
  glgfx_event_btn_middle	= glgfx_event_page_buttons | 0x02,


  // ***** Consumer events ******

  // CD/Media control
  glgfx_event_cd_play		= glgfx_event_page_consumer | 0xb0,
  glgfx_event_cd_pause,
  glgfx_event_cd_record,
  glgfx_event_cd_ff,
  glgfx_event_cd_rew,
  glgfx_event_cd_next,
  glgfx_event_cd_prev,
  glgfx_event_cd_stop,
  glgfx_event_cd_eject,

  glgfx_event_cd_stopeject	= glgfx_event_page_consumer | 0xcc,
  glgfx_event_cd_playpause,
  glgfx_event_cd_playskip,

  glgfx_event_cd_mute		= glgfx_event_page_consumer | 0xe2,
  glgfx_event_cd_bassboost	= glgfx_event_page_consumer | 0xe5,
  glgfx_event_cd_volumeup	= glgfx_event_page_consumer | 0xe9,
  glgfx_event_cd_volumedown	= glgfx_event_page_consumer | 0xea,

  // Application Launch
  glgfx_event_al_launchconfig	= glgfx_event_page_consumer | 0x181,
  glgfx_event_al_buttonconfig,
  glgfx_event_al_controlconfig,
  glgfx_event_al_wordprocessor,
  glgfx_event_al_texteditor,
  glgfx_event_al_spreadsheet,
  glgfx_event_al_graphicseditor,
  glgfx_event_al_presentation,
  glgfx_event_al_database,
  glgfx_event_al_email,
  glgfx_event_al_news,
  glgfx_event_al_voicemail,
  glgfx_event_al_addressbook,
  glgfx_event_al_calendar,
  glgfx_event_al_projectmanager,
  glgfx_event_al_journal,
  glgfx_event_al_finance,
  glgfx_event_al_calculator,
  glgfx_event_al_avcapture,
  glgfx_event_al_browser,
  glgfx_event_al_lanbrowser,
  glgfx_event_al_internetbrowser,
  glgfx_event_al_remotenetworking,
  glgfx_event_al_networkconference,
  glgfx_event_al_networkchat,
  glgfx_event_al_telephony,
  glgfx_event_al_logon,
  glgfx_event_al_logoff,
  glgfx_event_al_logonlogoff,
  glgfx_event_al_screensaver,
  glgfx_event_al_controlpanel,
  glgfx_event_al_cli,
  glgfx_event_al_taskmanager,
  glgfx_event_al_selecttask,
  glgfx_event_al_nexttask,
  glgfx_event_al_prevtask,
  glgfx_event_al_halttask,

  // Application Control
  glgfx_event_ac_new		= glgfx_event_page_consumer | 0x201,
  glgfx_event_ac_open,
  glgfx_event_ac_close,
  glgfx_event_ac_exit,
  glgfx_event_ac_maximize,
  glgfx_event_ac_minimize,
  glgfx_event_ac_save,
  glgfx_event_ac_print,
  glgfx_event_ac_properties,
  glgfx_event_ac_undo,
  glgfx_event_ac_copy,
  glgfx_event_ac_cut,
  glgfx_event_ac_paste,
  glgfx_event_ac_selectall,
  glgfx_event_ac_find,
  glgfx_event_ac_findreplace,
  glgfx_event_ac_search,
  glgfx_event_ac_goto,
  glgfx_event_ac_home,
  glgfx_event_ac_back,
  glgfx_event_ac_forward,
  glgfx_event_ac_stop,
  glgfx_event_ac_refresh,
  glgfx_event_ac_prevlink,
  glgfx_event_ac_nextlink,
  glgfx_event_ac_bookmarks,
  glgfx_event_ac_history,
  glgfx_event_ac_subscriptions,
  glgfx_event_ac_zoomin,
  glgfx_event_ac_zoomout,
  glgfx_event_ac_zoom,
  glgfx_event_ac_fullscreenview,
  glgfx_event_ac_normalview,
  glgfx_event_ac_toggleview,
  glgfx_event_ac_scrollup,
  glgfx_event_ac_scrolldown,
  glgfx_event_ac_scroll,
  glgfx_event_ac_panleft,
  glgfx_event_ac_panright,
  glgfx_event_ac_pan,
  glgfx_event_ac_newwindow,
  glgfx_event_ac_tilehorizontally,
  glgfx_event_ac_tilevertically,
  glgfx_event_ac_format,

  glgfx_event_ac_redo		= glgfx_event_page_consumer | 0x279,
  glgfx_event_ac_msgreply	= glgfx_event_page_consumer | 0x289,
  glgfx_event_ac_msgforward	= glgfx_event_page_consumer | 0x28b,
  glgfx_event_ac_msgsend	= glgfx_event_page_consumer | 0x28c,

  // Multimedia keyboard keys [see also /usr/X11R6/lib/X11/XKeysymDB]
/*   glgfx_event_my_computer = 0x80, */
/*   glgfx_event_my_documents = 0x80, */
/*   glgfx_event_my_pictures = 0x80, */
/*   glgfx_event_my_music = 0x80, */
/*   glgfx_event_media = 0x80, */
/*   glgfx_event_mail = 0x80, */
/*   glgfx_event_web_home = 0x80, */
/*   glgfx_event_messenger = 0x80, */
/*   glgfx_event_calculator = 0x80, */
};

enum glgfx_input_class {
  glgfx_input_none         = 0x00000000,

  glgfx_input_event        = 0x00 << 24,	/* Keyboard, buttons */
  glgfx_input_mouse        = 0x01 << 24,	/* Mouse movement (relative) */
  glgfx_input_scroll       = 0x02 << 24,	/* Mouse wheel (relative) */
  glgfx_input_tablet       = 0x03 << 24,	/* Mouse pad or tablet (absolute) */
//  glgfx_input_midi	   = 0x04 << 24,	/* MIDI event */

  glgfx_input_releasemask  = 0x80000000,	/* Press/release */
  glgfx_input_classmask    = 0x7f000000,	/* Class (key, mouse ...) */
  glgfx_input_devicemask   = 0x0000ffff,	/* Device id (kbd #0, #1, mouse #2 ...) */
};


struct glgfx_input_event {
    enum glgfx_input_class class;
    uint16_t device;
    
    union {
	enum glgfx_event_code event_code;

	struct {
	    int16_t dx;
	    int16_t dy;
	} mouse;

	struct {
	    uint16_t x;
	    uint16_t y;
	    uint16_t z;
	    int16_t  rx;
	    int16_t  ry;
	    int16_t  rz;
	    int16_t  p;
	} pad;
    };

    struct timespec timestamp;
};


bool glgfx_input_acquire(struct glgfx_monitor* monitor);
bool glgfx_input_release(struct glgfx_monitor* monitor);
bool glgfx_input_getcode(struct glgfx_input_event* event);

#endif /* ARP2_glgfx_glgfx_input_h */
