/*
   rdesktop: A Remote Desktop Protocol client.
   keyboard layout -- mapping to dos keybcodes... 
   Copyright (C) Matthew Chapman 1999-2000

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


/* if you wish to add or modify:
   the proper values can be found on a windows machine at:
   HKEY_LM\SYSTEM\CurrentControlSet\Control\Keyboard Layout\Dos\KeybCodes
*/

/*
First value found in short_key and key will be used (do not use the same thing twice)
therefore please put into short_key only one keyboard from your country (the main one)

in -k option you can specify both (short_key or key) that means
-k EN or -k "English" behave the same way or -k ? for list.

if you can, please follow the encodings from the iso 639 standard, found at:
http://www.w3.org/WAI/ER/IG/ert/iso639.htm

anyone who can update this list please update it (specialy short_key)
key can't contain NULL
*/


typedef int WORD;
#include "keymap.h"

typedef char LAYOUT[MAIN_LEN][4];
struct
{
	char *short_key;
	char *key;
	unsigned int code;
	const LAYOUT* layout;
} static const dosKeybCodes[] = {
{"SQ", "Albanian"				, 0x0000041c, NULL},	/*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as US */
{"AR", "Arabic (101)"				, 0x00000401, NULL},
{NULL, "Arabic (102) AZERTY"			, 0x00020401, NULL},
{NULL, "Arabic (102)"				, 0x00010401, NULL},
{NULL, "Armenian Eastern"			, 0x0000042b, NULL},
{NULL, "Armenian Western"			, 0x0001042b, NULL},
{NULL, "Azeri Cyrillic"				, 0x0000082c, NULL},
{NULL, "Azeri Latin"				, 0x0000042c, NULL},
{"BE", "Belarusian"				, 0x00000423, NULL},	/*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as us1 */
{NULL, "Belgian (Comma)"			, 0x0001080c, NULL},
{NULL, "Belgian Dutch"				, 0x00000813, NULL},
{NULL, "Belgian French"				, 0x0000080c, &main_key_BE},	/*main_key_BE is saying only Belgian?? which one? there was some mention of azerty..french???*/
{NULL, "Bulgarian (Latin)"			, 0x00010402, NULL},	/*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as us7 */
{"BG", "Bulgarian"				, 0x00000402, NULL},
{"CF", "Canadian French (Legacy)"		, 0x00000c0c, &main_key_CF},	/* main_key_CF is saying only Canadian French?? which one? */ 
{NULL, "Canadian French"			, 0x00001009, NULL},	/*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as us5 */
{NULL, "Canadian Multilingual Standard"		, 0x00011009, NULL},
{NULL, "Chinese (Simplified) - MS-PinYin98"	, 0xE00E0804, NULL},
{NULL, "Chinese (Simplified) - NeiMa"		, 0xE0050804, NULL},
{NULL, "Chinese (Simplified) - QuanPin"		, 0xE0010804, NULL},
{NULL, "Chinese (Simplified) - ShuangPin"	, 0xE0020804, NULL},
{NULL, "Chinese (Simplified) - US Keyboard"	, 0x00000804, NULL},		
{NULL, "Chinese (Simplified) - ZhengMa"		, 0xE0030804, NULL},
{NULL, "Chinese (Traditional) - Alphanumeric"	, 0xE01F0404, NULL},
{NULL, "Chinese (Traditional) - Array"		, 0xE0050404, NULL},
{NULL, "Chinese (Traditional) - Big5 Code"	, 0xE0040404, NULL},
{NULL, "Chinese (Traditional) - ChangJie"	, 0xE0020404, NULL},
{NULL, "Chinese (Traditional) - DaYi"		, 0xE0060404, NULL},
{NULL, "Chinese (Traditional) - New ChangJie"	, 0xE0090404, NULL},
{NULL, "Chinese (Traditional) - New Phonetic"	, 0xE0080404, NULL},
{NULL, "Chinese (Traditional) - Phonetic"	, 0xE0010404, NULL},
{NULL, "Chinese (Traditional) - Quick"		, 0xE0030404, NULL},
{"ZH", "Chinese (Traditional) - US Keyboard"	, 0x00000404, NULL},
{NULL, "Chinese (Traditional) - Unicode"	, 0xE0070404, NULL},
{"HR", "Croatian"				, 0x0000041A, &main_key_HR},
{NULL, "Croatian (specific)"			, 0x0000041A, &main_key_HR_jelly},	/* double line */
{NULL, "Czech (QWERTY)"				, 0x00010405, NULL},
{NULL, "Czech Programmers"			, 0x00020405, &main_key_SK_prog},	/* wine says it is the same as Slovak programers */ 
{"CS", "Czech"					, 0x00000405, &main_key_CS},
{"DA", "Danish"					, 0x00000406, &main_key_DA},
{NULL, "Devanagari - INSCRIPT"			, 0x00000439, NULL},
{"NL", "Dutch"					, 0x00000413, NULL},
{"ET", "Estonian"				, 0x00000425, NULL},
{"FO", "Faeroese"				, 0x00000438, NULL},
{NULL, "Farsi"					, 0x00000429, NULL},
{"FI", "Finnish"				, 0x0000040b, &main_key_FI},
{"FR", "French"					, 0x0000040c, &main_key_FR},
{"GD","Gaelic"					, 0x00011809, NULL},
{"KA", "Georgian"				, 0x00000437, NULL},
{NULL, "German (IBM)"				, 0x00010407, NULL},
{"DE", "German"					, 0x00000407, &main_key_DE},
{NULL, "German (without dead keys)"		, 0x00000407, &main_key_DE_nodead},	/* Double line */
{NULL, "Greek (220) Latin"			, 0x00030408, NULL},
{NULL, "Greek (220)"				, 0x00010408, NULL},
{NULL, "Greek (319) Latin"			, 0x00040408, NULL},
{NULL, "Greek (319)"				, 0x00020408, NULL},
{NULL, "Greek Latin"				, 0x00050408, NULL},
{NULL, "Greek Polytonic"			, 0x00060408, NULL},
{"EL", "Greek"					, 0x00000408, NULL},
{"HE", "Hebrew"					, 0x0000040d, NULL},
{"HI", "Hindi Traditional"			, 0x00010439, NULL},
{NULL, "Hungarian 101-key"			, 0x0001040e, NULL},
{"HU", "Hungarian"				, 0x0000040e, &main_key_HU},
{"IS", "Icelandic"				, 0x0000040f, &main_key_IS},
{"GA", "Irish"					, 0x00001809, NULL},	/*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as us6 */
{NULL, "Italian (142)"				, 0x00010410, NULL},
{"IT", "Italian"				, 0x00000410, &main_key_IT},
{NULL, "Japanese Input System (MS-IME2000)"	, 0xE0010411, NULL},
{"JA", "Japanese"				, 0x00000411, NULL},
{NULL, "Japanese (106 keyboard)"		, 0x00000411, &main_key_JA_jp106},	/* double line */ /* is one of this two "Japanese Input System (MS-IME2000)" or "Japanese" */
{NULL, "Japanese (pc98x1 keyboard)"		, 0x00000411, &main_key_JA_pc98x1},	/* double line */ /* is one of this two "Japanese Input System (MS-IME2000)" or "Japanese" */
{"KK", "Kazakh"					, 0x0000043f, NULL},
{NULL, "Korean(Hangul) (MS-IME98)"		, 0xE0010412, NULL},
{"KO", "Korean(Hangul)"				, 0x00000412, NULL},
{"LA", "Latin American"				, 0x0000080a, &main_key_LA},
{NULL, "Latvian (QWERTY)"			, 0x00010426, NULL},	/*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as us8 */
{"LV", "Latvian"				, 0x00000426, NULL},	/* 0x426 is marked in old source as us2???? */
{NULL, "Lithuanian IBM"				, 0x00000427, NULL},	/* 0x427 is marked in old source as us3???? */
{"LT", "Lithuanian"				, 0x00010427, &main_key_LT_B},	/* main_key_LT_B is saying Lithuanian (Baltic)?? which one? */
{"MK", "Macedonian (FYROM)"			, 0x0000042f, NULL},
{"MR", "Marathi"				, 0x0000044e, NULL},
{"NO", "Norwegian"				, 0x00000414, &main_key_NO},
{NULL, "Polish (214)"				, 0x00010415, NULL},
{"PL", "Polish (Programmers)"			, 0x00000415, &main_key_PL},
{"BR", "Portuguese (Brazilian ABNT)"		, 0x00000416, NULL},
{NULL, "Portuguese (Brazilian ABNT2)"		, 0x00010416, &main_key_PT_br},
{"PT", "Portuguese"				, 0x00000816, &main_key_PT},	/* main_key_PT is saying only Portuguese?? which one? */
{"RO", "Romanian"				, 0x00000418, NULL},
{NULL, "Russian (Typewriter)"			, 0x00010419, NULL},
{"RU", "Russian"				, 0x00000419, &main_key_RU},
{NULL, "Russian (KOI8-R)"			, 0x00000419, &main_key_RU_koi8r},	/* Double Line */ /* or maybe Typewriter?? */
{NULL, "Serbian (Cyrillic)"			, 0x00000c1a, NULL},	/* 0xc1a is marked as us4 in old source */
{"SR", "Serbian (Latin)"			, 0x0000081a, NULL},
{NULL, "Slovak (QWERTY)"			, 0x0001041b, NULL},
{"SK", "Slovak"					, 0x0000041b, &main_key_SK},
{NULL, "Slovak (programers)"			, 0x0000041b, &main_key_SK_prog}, /* double line */
{"SL", "Slovenian"				, 0x00000424, NULL},
{NULL, "Spanish Variation"			, 0x0001040a, NULL},
{"ES", "Spanish"				, 0x0000040a, &main_key_ES},
{"SV", "Swedish"				, 0x0000041d, &main_key_SE},
{"SF", "Swiss French"				, 0x0000100c, &main_key_SF},
{"SG", "Swiss German"				, 0x00000807, &main_key_SG},
{"TA", "Tamil"					, 0x00000449, NULL},
{"TT", "Tatar"					, 0x00000444, NULL},
{NULL, "Thai Kedmanee (non-ShiftLock)"		, 0x0002041e, NULL},
{"TH", "Thai Kedmanee"				, 0x0000041e, NULL},
{NULL, "Thai Pattachote (non-ShiftLock)"	, 0x0003041e, NULL},
{NULL, "Thai Pattachote"			, 0x0001041e, NULL},
{"TR", "Turkish F"				, 0x0001041f, NULL},
{NULL, "Turkish Q"				, 0x0000041f, &main_key_TK},	/*main_key_TK is saying only Turkish?? which one? */
{NULL, "US English Table for IBM Arabic 238_L"	, 0x00050409, NULL},
{"US", "US"					, 0x00000409,  &main_key_US},	/*main_key_US is saying only US?? which one? */
{NULL, "US (phantom key version)"		, 0x00000409,  &main_key_US_phantom},	/* Double Line */
{"UK","Ukrainian"				, 0x00000422, NULL},	/*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as us0 */
{"EN", "English"				, 0x00000809, &main_key_UK},
{NULL, "United States-Dvorak for left hand"	, 0x00030409, NULL}, /*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as usl */
{NULL, "United States-Dvorak for right hand"	, 0x00040409, NULL}, /*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as usr */
{"DV", "United States-Dvorak"			, 0x00010409, NULL},
{NULL, "United States-International"		, 0x00020409, &main_key_US_intl}, /*  short_key is marked in old source (rdesktop-1.0.0-pl19-7-2) as us10 */
{"UZ", "Uzbek Cyrillic"				, 0x00000843, NULL},
{"VI", "Vietnamese"				, 0x0000042a, NULL},
{NULL, "yu",0x41a, NULL},	/* what is that for keyboad I can't find this in windows registry */
{NULL, "cf",0x10c0c, NULL},	/* what is that for keyboad I can't find this in windows registry */
{NULL, "us9",0x10c1a, NULL},	/* what is that for keyboad I can't find this in windows registry */
{NULL, NULL, 0,NULL}
};
