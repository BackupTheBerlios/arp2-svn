#include "doskbcodes.h"


#ifdef GUI_XWIN
/* don't include all the defines from this file, it can be quite extensive. */
#include <X11/keysymdef.h>
#endif

#ifndef XK_EuroSign
#define XK_EuroSign 0x20ac
#endif

#define KS_MASK 0x3ff

/* KeySym & 0x3ff is sufficient to identify the key */
int XKeysym2PCKeyCode[KS_MASK + 1];

void
init_keycodes(const int id)
{
	static int i, mod_state;
	const char *c;
	const LAYOUT *keymap;

	if ((id == -1) || (NULL == dosKeybCodes[id].layout)) {	/* id == -1 it is not likely to happen should be checked before this is called. */
		error("keymap %s not found.\n", dosKeybCodes[id].key);
		exit(1);
	}
	keymap = dosKeybCodes[id].layout;

	for (i = 0; i < KS_MASK + 1; ++i)
		XKeysym2PCKeyCode[i] = 0;

	/* These are maybe sent when pressing AltGr plus a number */
	for (i = 0; i < 10; ++i)
		XKeysym2PCKeyCode[0xb0 + i] = 1 + i;

	for (i = 0; i < MAIN_LEN; ++i) {
		/* look it up in the kbd map. */
		for (c = &((*keymap)[i][0]), mod_state = 0; *c;
		     c++, mod_state += 0x100) {
			XKeysym2PCKeyCode[(unsigned char) *c] =
			    main_key_scan_qwerty[i] | mod_state;
		}
	}

	/* what are these??? someone? */
	XKeysym2PCKeyCode[0x0113] = 0x66;
	XKeysym2PCKeyCode[0x0114] = 0x46;	/* scroll lock */
	XKeysym2PCKeyCode[0x017e] = 0xb8;
	XKeysym2PCKeyCode[0x01e5] = 0x3A;	/*caps lock */
	XKeysym2PCKeyCode[0x01eb] = 0x2e;	/* C ??? */

	XKeysym2PCKeyCode[XK_space & KS_MASK] = 0x39;
	XKeysym2PCKeyCode[XK_BackSpace & KS_MASK] = 0x0e;
	XKeysym2PCKeyCode[XK_Tab & KS_MASK] = 0x0f;
	XKeysym2PCKeyCode[XK_Return & KS_MASK] = 0x1c;

/*   XKeysym2PCKeyCode[XK_dead_acute & KS_MASK]=
   XKeysym2PCKeyCode[XK_dead_grave & KS_MASK]=
   XKeysym2PCKeyCode[XK_dead_diaeresis & KS_MASK]=
   XKeysym2PCKeyCode[XK_dead_circumflex & KS_MASK]=
   XKeysym2PCKeyCode[XK_dead_tilde & KS_MASK]=

   XKeysym2PCKeyCode[XK_Scroll_Lock & KS_MASK]=
   XKeysym2PCKeyCode[XK_Sys_Req & KS_MASK]=
*/
	switch (dosKeybCodes[id].code) {
	case 0x40c:		/* (FR) French keyboard */
		XKeysym2PCKeyCode[XK_dead_circumflex & KS_MASK] = 0x1a;
		XKeysym2PCKeyCode[XK_dead_diaeresis & KS_MASK] = 0x1a;
		XKeysym2PCKeyCode[XK_EuroSign & KS_MASK] = 0x12;
		break;
	case 0x40a:		/* (SP) Spanish Keyboard */
		XKeysym2PCKeyCode[XK_dead_grave & KS_MASK] = 0x1a;
		XKeysym2PCKeyCode[XK_dead_circumflex & KS_MASK] = 0x1a;
		XKeysym2PCKeyCode[XK_dead_acute & KS_MASK] = 0x1b;
		XKeysym2PCKeyCode[XK_dead_diaeresis & KS_MASK] = 0x1b;
		XKeysym2PCKeyCode[XK_EuroSign & KS_MASK] = 0x12;
		break;
	case 0x404:		/* (CH) Chinese (Traditional) - US Keyboard */
		XKeysym2PCKeyCode[XK_dead_grave & KS_MASK] = 0x0d;
		XKeysym2PCKeyCode[XK_dead_tilde & KS_MASK] = 0x0d;
		XKeysym2PCKeyCode[XK_dead_circumflex & KS_MASK] = 0x0d;
		XKeysym2PCKeyCode[XK_dead_acute & KS_MASK] = 0x0c;
		XKeysym2PCKeyCode[XK_dead_diaeresis & KS_MASK] = 0x1b;
		XKeysym2PCKeyCode[XK_EuroSign & KS_MASK] = 0x12;
		break;
	case 0x41d:		/* (SV) Swedish Keyboard */
		XKeysym2PCKeyCode[XK_EuroSign & KS_MASK] = 0x12;
		break;
	case 0x816:		/* (PO) Portuguese Keyboard */
		XKeysym2PCKeyCode[XK_dead_diaeresis & KS_MASK] = 0x1a;
		XKeysym2PCKeyCode[XK_dead_grave & KS_MASK] = 0x1b;
		XKeysym2PCKeyCode[XK_dead_acute & KS_MASK] = 0x1b;
		XKeysym2PCKeyCode[XK_dead_tilde & KS_MASK] = 0x2B;
		XKeysym2PCKeyCode[XK_dead_circumflex & KS_MASK] = 0x2B;
		XKeysym2PCKeyCode[XK_EuroSign & KS_MASK] = 0x12;
		break;
	case 0x809:		/* (UK) United Kingdom Keyboard */
		XKeysym2PCKeyCode[XK_EuroSign & KS_MASK] = 0x05;
		break;
	case 0x409:		/* (US) US Keyboard */
		XKeysym2PCKeyCode[XK_EuroSign & KS_MASK] = 0x06;
		break;
	}

	XKeysym2PCKeyCode[XK_Escape & KS_MASK] = 0x1;
	XKeysym2PCKeyCode[XK_Delete & KS_MASK] = 0x53 | 0x80;
	XKeysym2PCKeyCode[XK_Home & KS_MASK] = 0x47 | 0x80;
	XKeysym2PCKeyCode[XK_Left & KS_MASK] = 0x4b | 0x80;
	XKeysym2PCKeyCode[XK_Up & KS_MASK] = 0x48 | 0x80;
	XKeysym2PCKeyCode[XK_Right & KS_MASK] = 0x4d | 0x80;
	XKeysym2PCKeyCode[XK_Down & KS_MASK] = 0x50 | 0x80;
	XKeysym2PCKeyCode[XK_Prior & KS_MASK] = 0x49 | 0x80;
	XKeysym2PCKeyCode[XK_Next & KS_MASK] = 0x51 | 0x80;
	XKeysym2PCKeyCode[XK_End & KS_MASK] = 0x4f | 0x80;
	XKeysym2PCKeyCode[XK_Print & KS_MASK] = 0x37 | 0x80;
	XKeysym2PCKeyCode[XK_Insert & KS_MASK] = 0x52 | 0x80;

	/* grey keys around NumPad */
	XKeysym2PCKeyCode[XK_Num_Lock & KS_MASK] = 0x45;
	XKeysym2PCKeyCode[XK_KP_Divide & KS_MASK] = 0x35 | 0x80;
	XKeysym2PCKeyCode[XK_KP_Multiply & KS_MASK] = 0x37;
	XKeysym2PCKeyCode[XK_KP_Subtract & KS_MASK] = 0x4a;
	XKeysym2PCKeyCode[XK_KP_Add & KS_MASK] = 0x4e;
	XKeysym2PCKeyCode[XK_KP_Enter & KS_MASK] = 0x1c | 0x80;

	/* Unlocked NumPad */
	XKeysym2PCKeyCode[XK_KP_Home & KS_MASK] = 0x47;
	XKeysym2PCKeyCode[XK_KP_Left & KS_MASK] = 0x4b;
	XKeysym2PCKeyCode[XK_KP_Begin & KS_MASK] = 0x4c;
	XKeysym2PCKeyCode[XK_KP_Up & KS_MASK] = 0x48;
	XKeysym2PCKeyCode[XK_KP_Right & KS_MASK] = 0x4d;
	XKeysym2PCKeyCode[XK_KP_Down & KS_MASK] = 0x50;
	XKeysym2PCKeyCode[XK_KP_Prior & KS_MASK] = 0x49;
	XKeysym2PCKeyCode[XK_KP_Next & KS_MASK] = 0x51;
	XKeysym2PCKeyCode[XK_KP_End & KS_MASK] = 0x4f;
	XKeysym2PCKeyCode[XK_KP_Insert & KS_MASK] = 0x52;
	XKeysym2PCKeyCode[XK_KP_Delete & KS_MASK] = 0x53;

	/* locked NumPad */
	XKeysym2PCKeyCode[XK_KP_Decimal & KS_MASK] = 0x53;
	XKeysym2PCKeyCode[XK_KP_0 & KS_MASK] = 0x52;
	XKeysym2PCKeyCode[XK_KP_1 & KS_MASK] = 0x4f;
	XKeysym2PCKeyCode[XK_KP_2 & KS_MASK] = 0x50;
	XKeysym2PCKeyCode[XK_KP_3 & KS_MASK] = 0x51;
	XKeysym2PCKeyCode[XK_KP_4 & KS_MASK] = 0x4b;
	XKeysym2PCKeyCode[XK_KP_5 & KS_MASK] = 0x4c;
	XKeysym2PCKeyCode[XK_KP_6 & KS_MASK] = 0x4d;
	XKeysym2PCKeyCode[XK_KP_7 & KS_MASK] = 0x47;
	XKeysym2PCKeyCode[XK_KP_8 & KS_MASK] = 0x48;
	XKeysym2PCKeyCode[XK_KP_9 & KS_MASK] = 0x49;

	/* Function keys */
	XKeysym2PCKeyCode[XK_F1 & KS_MASK] = 0x3b;
	XKeysym2PCKeyCode[XK_F2 & KS_MASK] = 0x3c;
	XKeysym2PCKeyCode[XK_F3 & KS_MASK] = 0x3d;
	XKeysym2PCKeyCode[XK_F4 & KS_MASK] = 0x3e;
	XKeysym2PCKeyCode[XK_F5 & KS_MASK] = 0x3f;
	XKeysym2PCKeyCode[XK_F6 & KS_MASK] = 0x40;
	XKeysym2PCKeyCode[XK_F7 & KS_MASK] = 0x41;
	XKeysym2PCKeyCode[XK_F8 & KS_MASK] = 0x42;
	XKeysym2PCKeyCode[XK_F9 & KS_MASK] = 0x43;
	XKeysym2PCKeyCode[XK_F10 & KS_MASK] = 0x44;
	XKeysym2PCKeyCode[XK_F11 & KS_MASK] = 0x57;
	XKeysym2PCKeyCode[XK_F12 & KS_MASK] = 0x58;

	XKeysym2PCKeyCode[XK_Shift_L & KS_MASK] = 0x2a;
	XKeysym2PCKeyCode[XK_Shift_R & KS_MASK] = 0x36;
	XKeysym2PCKeyCode[XK_Control_L & KS_MASK] = 0x1d;
	XKeysym2PCKeyCode[XK_Control_R & KS_MASK] = 0x1d | 0x80;

	/* are these correct??? */
	XKeysym2PCKeyCode[XK_Meta_L & KS_MASK] = 0x5b | 0x80;
	XKeysym2PCKeyCode[XK_Alt_L & KS_MASK] = 0x38;
	XKeysym2PCKeyCode[XK_Mode_switch & KS_MASK] = 0x38 | 0x80;
	XKeysym2PCKeyCode[XK_Multi_key & KS_MASK] = 0x5c | 0x80;
	XKeysym2PCKeyCode[XK_Menu & KS_MASK] = 0x5d | 0x80;

}
