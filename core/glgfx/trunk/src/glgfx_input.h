#ifndef ARP2_glgfx_glgfx_input_h
#define ARP2_glgfx_glgfx_input_h

#include <glgfx.h>

#ifdef __cplusplus
extern "C" {
#endif

struct glgfx_monitor;

enum glgfx_input_page {
  // Page tables

  glgfx_input_page_desktop		= 0x0001 << 16,				// Desktop
  glgfx_input_page_game			= 0x0001 << 16,				// Game controls
  glgfx_input_page_keyboard		= 0x0007 << 16,				// Keyboard/keypad
  glgfx_input_page_buttons		= 0x0009 << 16,				// Mouse button
  glgfx_input_page_consumer		= 0x000c << 16,				// Consumer
  glgfx_input_page_digitizers		= 0x000d << 16,				// Digitizers

  glgfx_input_pagemask			= 0xffff << 16,
};

enum glgfx_input_class {
  // Event classes

  glgfx_input_class_mouse		= glgfx_input_page_desktop | 0x02,
  glgfx_input_class_joystick		= glgfx_input_page_desktop | 0x04,
  glgfx_input_class_gamepad		= glgfx_input_page_desktop | 0x05,
  glgfx_input_class_keyboard		= glgfx_input_page_desktop | 0x06,

  glgfx_input_class_pov			= glgfx_input_page_game | 0x20,

  glgfx_input_class_digitizer		= glgfx_input_page_digitizers | 0x01,
  glgfx_input_class_pen			= glgfx_input_page_digitizers | 0x02,
  glgfx_input_class_lightpen		= glgfx_input_page_digitizers | 0x03,
  glgfx_input_class_touchscreen		= glgfx_input_page_digitizers | 0x04,
  glgfx_input_class_touchpad		= glgfx_input_page_digitizers | 0x05,
  glgfx_input_class_whiteboard		= glgfx_input_page_digitizers | 0x06,
};

enum glgfx_input_code {
  glgfx_input_none			= 0,

  // ***** Desktop events *****

  glgfx_input_x				= glgfx_input_page_desktop | 0x30,
  glgfx_input_y,
  glgfx_input_z,
  glgfx_input_rx,
  glgfx_input_ry,
  glgfx_input_rz,

  glgfx_input_poweroff			= glgfx_input_page_desktop | 0x81,
  glgfx_input_sleep,
  glgfx_input_wakeup,

  glgfx_input_coldboot			= glgfx_input_page_desktop | 0x8e,
  glgfx_input_warmboot,

  glgfx_input_dock			= glgfx_input_page_desktop | 0xa0,
  glgfx_input_undock,
  glgfx_input_setup,
  glgfx_input_break,

  glgfx_input_hibernate			= glgfx_input_page_desktop | 0xa8,


  // ***** Keyboard/keypad keys *****

  glgfx_input_key_a			= glgfx_input_page_keyboard | 0x04,
  glgfx_input_key_b,
  glgfx_input_key_c,
  glgfx_input_key_d,
  glgfx_input_key_e,
  glgfx_input_key_f,
  glgfx_input_key_g,
  glgfx_input_key_h,
  glgfx_input_key_i,
  glgfx_input_key_j,
  glgfx_input_key_k,
  glgfx_input_key_l,
  glgfx_input_key_m,
  glgfx_input_key_n,
  glgfx_input_key_o,
  glgfx_input_key_p,
  glgfx_input_key_q,
  glgfx_input_key_r,
  glgfx_input_key_s,
  glgfx_input_key_t,
  glgfx_input_key_u,
  glgfx_input_key_v,
  glgfx_input_key_w,
  glgfx_input_key_x,
  glgfx_input_key_y,
  glgfx_input_key_z,

  glgfx_input_key_1			= glgfx_input_page_keyboard | 0x1e,
  glgfx_input_key_2,
  glgfx_input_key_3,
  glgfx_input_key_4,
  glgfx_input_key_5,
  glgfx_input_key_6,
  glgfx_input_key_7,
  glgfx_input_key_8,
  glgfx_input_key_9,
  glgfx_input_key_0,

  glgfx_input_key_return		= glgfx_input_page_keyboard | 0x28,
  glgfx_input_key_escape,
  glgfx_input_key_backspace,
  glgfx_input_key_tab,
  glgfx_input_key_space,

  glgfx_input_key_minus			= glgfx_input_page_keyboard | 0x2d,
  glgfx_input_key_equal,
  glgfx_input_key_lbracket,
  glgfx_input_key_rbracket,
  glgfx_input_key_backslash,
  glgfx_input_key_numbersign,
  glgfx_input_key_semicolon,
  glgfx_input_key_quote,
  glgfx_input_key_backquote,
  glgfx_input_key_comma,
  glgfx_input_key_period,
  glgfx_input_key_slash,

  glgfx_input_key_capslock		= glgfx_input_page_keyboard | 0x39,

  glgfx_input_key_f1			= glgfx_input_page_keyboard | 0x3a,
  glgfx_input_key_f2,
  glgfx_input_key_f3,
  glgfx_input_key_f4,
  glgfx_input_key_f5,
  glgfx_input_key_f6,
  glgfx_input_key_f7,
  glgfx_input_key_f8,
  glgfx_input_key_f9,
  glgfx_input_key_f10,
  glgfx_input_key_f11,
  glgfx_input_key_f12,

  glgfx_input_key_prtscr		= glgfx_input_page_keyboard | 0x46,
  glgfx_input_key_scrlock,
  glgfx_input_key_pause,

  glgfx_input_key_insert		= glgfx_input_page_keyboard | 0x49,
  glgfx_input_key_home,
  glgfx_input_key_pageup,
  glgfx_input_key_delete,
  glgfx_input_key_end,
  glgfx_input_key_pagedown,

  glgfx_input_key_right			= glgfx_input_page_keyboard | 0x49,
  glgfx_input_key_left,
  glgfx_input_key_down,
  glgfx_input_key_up,

  glgfx_input_key_numlock		= glgfx_input_page_keyboard | 0x53,	// NumLock or Clear

  glgfx_input_key_np_div		= glgfx_input_page_keyboard | 0x54,
  glgfx_input_key_np_mul,
  glgfx_input_key_np_sub,
  glgfx_input_key_np_add,
  glgfx_input_key_np_enter,
  glgfx_input_key_np_1,
  glgfx_input_key_np_2,
  glgfx_input_key_np_3,
  glgfx_input_key_np_4,
  glgfx_input_key_np_5,
  glgfx_input_key_np_6,
  glgfx_input_key_np_7,
  glgfx_input_key_np_8,
  glgfx_input_key_np_9,
  glgfx_input_key_np_0,
  glgfx_input_key_np_decimal,

  glgfx_input_key_ltgt			= glgfx_input_page_keyboard | 0x64,

  glgfx_input_key_menu			= glgfx_input_page_keyboard | 0x65,	// Menu, App, Compose

  // 0x66 unused

  glgfx_input_key_np_equal		= glgfx_input_page_keyboard | 0x67,

  glgfx_input_key_f13			= glgfx_input_page_keyboard | 0x68,
  glgfx_input_key_f14,
  glgfx_input_key_f15,
  glgfx_input_key_f16,
  glgfx_input_key_f17,
  glgfx_input_key_f18,
  glgfx_input_key_f19,
  glgfx_input_key_f20,
  glgfx_input_key_f21,
  glgfx_input_key_f22,
  glgfx_input_key_f23,
  glgfx_input_key_f24,

  // 0x74 unused

  glgfx_input_key_help			= glgfx_input_page_keyboard | 0x75,
  glgfx_input_key_front,							// On Sun keyboards
  
  // 0x77 -- 0x99 unused
  glgfx_input_key_sysrq 		= glgfx_input_page_keyboard | 0x9a,	// LAlt+PrtScr

  // 0x9b -- 0xb5 unused

  glgfx_input_key_np_lparen		= glgfx_input_page_keyboard | 0xb6,
  glgfx_input_key_np_rparen,

  // 0xb7 -- 0xd6 unused

  glgfx_input_key_np_plusminus		= glgfx_input_page_keyboard | 0xd7,

  // 0xd8 -- 0xdf unused

  glgfx_input_key_lctrl			= glgfx_input_page_keyboard | 0xe0,
  glgfx_input_key_lshift,
  glgfx_input_key_lalt,
  glgfx_input_key_lgui,
  glgfx_input_key_rctrl,
  glgfx_input_key_rshift,
  glgfx_input_key_ralt,
  glgfx_input_key_rgui,

  // 0xe8 -- 0xffff unused


  // ***** Mouse button events ******

  glgfx_input_btn_none			= glgfx_input_page_buttons | 0x00,
  glgfx_input_btn_left			= glgfx_input_page_buttons | 0x01,
  glgfx_input_btn_right,
  glgfx_input_btn_middle,
  glgfx_input_btn_4,
  glgfx_input_btn_5,
  glgfx_input_btn_6,
  glgfx_input_btn_7,
  glgfx_input_btn_8,								// .. etc


  // ***** Consumer events ******

  // CD/Media control
  glgfx_input_cd_play			= glgfx_input_page_consumer | 0xb0,
  glgfx_input_cd_pause,
  glgfx_input_cd_record,
  glgfx_input_cd_ff,
  glgfx_input_cd_rew,
  glgfx_input_cd_next,
  glgfx_input_cd_prev,
  glgfx_input_cd_stop,
  glgfx_input_cd_eject,

  glgfx_input_cd_stopeject		= glgfx_input_page_consumer | 0xcc,
  glgfx_input_cd_playpause,
  glgfx_input_cd_playskip,

  glgfx_input_cd_mute			= glgfx_input_page_consumer | 0xe2,
  glgfx_input_cd_bassboost		= glgfx_input_page_consumer | 0xe5,
  glgfx_input_cd_volumeup		= glgfx_input_page_consumer | 0xe9,
  glgfx_input_cd_volumedown		= glgfx_input_page_consumer | 0xea,

  // Application Launch
  glgfx_input_al_launchconfig		= glgfx_input_page_consumer | 0x181,
  glgfx_input_al_buttonconfig,
  glgfx_input_al_controlconfig,
  glgfx_input_al_wordprocessor,
  glgfx_input_al_texteditor,
  glgfx_input_al_spreadsheet,
  glgfx_input_al_graphicseditor,
  glgfx_input_al_presentation,
  glgfx_input_al_database,
  glgfx_input_al_email,
  glgfx_input_al_news,
  glgfx_input_al_voicemail,
  glgfx_input_al_addressbook,
  glgfx_input_al_calendar,
  glgfx_input_al_projectmanager,
  glgfx_input_al_journal,
  glgfx_input_al_finance,
  glgfx_input_al_calculator,
  glgfx_input_al_avcapture,
  glgfx_input_al_browser,
  glgfx_input_al_lanbrowser,
  glgfx_input_al_internetbrowser,
  glgfx_input_al_remotenetworking,
  glgfx_input_al_networkconference,
  glgfx_input_al_networkchat,
  glgfx_input_al_telephony,
  glgfx_input_al_logon,
  glgfx_input_al_logoff,
  glgfx_input_al_logonlogoff,
  glgfx_input_al_screensaver,
  glgfx_input_al_controlpanel,
  glgfx_input_al_cli,
  glgfx_input_al_taskmanager,
  glgfx_input_al_selecttask,
  glgfx_input_al_nexttask,
  glgfx_input_al_prevtask,
  glgfx_input_al_halttask,

  // Application Control
  glgfx_input_ac_new			= glgfx_input_page_consumer | 0x201,
  glgfx_input_ac_open,
  glgfx_input_ac_close,
  glgfx_input_ac_exit,
  glgfx_input_ac_maximize,
  glgfx_input_ac_minimize,
  glgfx_input_ac_save,
  glgfx_input_ac_print,
  glgfx_input_ac_properties,
  glgfx_input_ac_undo,
  glgfx_input_ac_copy,
  glgfx_input_ac_cut,
  glgfx_input_ac_paste,
  glgfx_input_ac_selectall,
  glgfx_input_ac_find,
  glgfx_input_ac_findreplace,
  glgfx_input_ac_search,
  glgfx_input_ac_goto,
  glgfx_input_ac_home,
  glgfx_input_ac_back,
  glgfx_input_ac_forward,
  glgfx_input_ac_stop,
  glgfx_input_ac_refresh,
  glgfx_input_ac_prevlink,
  glgfx_input_ac_nextlink,
  glgfx_input_ac_bookmarks,
  glgfx_input_ac_history,
  glgfx_input_ac_subscriptions,
  glgfx_input_ac_zoomin,
  glgfx_input_ac_zoomout,
  glgfx_input_ac_zoom,
  glgfx_input_ac_fullscreenview,
  glgfx_input_ac_normalview,
  glgfx_input_ac_toggleview,
  glgfx_input_ac_scrollup,
  glgfx_input_ac_scrolldown,
  glgfx_input_ac_scroll,							// V. Wheel
  glgfx_input_ac_panleft,
  glgfx_input_ac_panright,
  glgfx_input_ac_pan,								// H. Wheel
  glgfx_input_ac_newwindow,
  glgfx_input_ac_tilehorizontally,
  glgfx_input_ac_tilevertically,
  glgfx_input_ac_format,

  glgfx_input_ac_redo			= glgfx_input_page_consumer | 0x279,
  glgfx_input_ac_msgreply		= glgfx_input_page_consumer | 0x289,
  glgfx_input_ac_msgforward		= glgfx_input_page_consumer | 0x28b,
  glgfx_input_ac_msgsend		= glgfx_input_page_consumer | 0x28c,

  // Multimedia keyboard keys [see also /usr/X11R6/lib/X11/XKeysymDB]
/*   glgfx_input_my_computer = 0x80, */
/*   glgfx_input_my_documents = 0x80, */
/*   glgfx_input_my_pictures = 0x80, */
/*   glgfx_input_my_music = 0x80, */
/*   glgfx_input_media = 0x80, */
/*   glgfx_input_mail = 0x80, */
/*   glgfx_input_web_home = 0x80, */
/*   glgfx_input_messenger = 0x80, */
/*   glgfx_input_calculator = 0x80, */

//  glgfx_input_ac_new			= glgfx_input_page_consumer | 0x201,

};


struct glgfx_input_event {
    enum glgfx_input_class class;
    enum glgfx_input_code  code;
    int32_t                value;
    uint32_t               device;
    struct timespec        timestamp;
};


bool glgfx_input_acquire(struct glgfx_monitor* monitor);
bool glgfx_input_release(struct glgfx_monitor* monitor);
bool glgfx_input_getcode(struct glgfx_input_event* event);

#ifdef __cplusplus
}
#endif

#endif /* ARP2_glgfx_glgfx_input_h */
