/*
 * X11 keyboard driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine 
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 1999 Ove K�ven
 */
#define MAIN_LEN 48
static const WORD main_key_scan_qwerty[MAIN_LEN] = {
/* this is my (102-key) keyboard layout, sorry if it doesn't quite match yours */
	0x29, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
	0x0D,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
	0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2B,
	0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0x56			/* the 102nd key (actually to the right of l-shift) */
};
static const char main_key_US[MAIN_LEN][4] = {
	"`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_",
	"=+",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{", "]}",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:", "'\"",
	"\\|",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?"
};

/*** United States keyboard layout (phantom key version) */
/* (XFree86 reports the <> key even if it's not physically there) */
static const char main_key_US_phantom[MAIN_LEN][4] = {
	"`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_",
	"=+",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{", "]}",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:", "'\"",
	"\\|",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?",
	"<>"			/* the phantom key */
};

/*** British keyboard layout */
static const char main_key_UK[MAIN_LEN][4] = {
	"`", "1!", "2\"", "3�", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_",
	"=+",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{", "]}",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:", "'@", "#~",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?",
	"\\|"
};

/*** French keyboard layout (contributed by Eric Pouech) */
static const char main_key_FR[MAIN_LEN][4] = {
	"�", "&1", "�2~", "\"3#", "'4{", "(5[", "-6|", "�7", "_8\\", "�9^�",
	"�0@", ")�]", "=+}",
	"aA", "zZ", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "^�", "$��",
	"qQ", "sS�", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "mM", "�%", "*�",
	"wW", "xX", "cC", "vV", "bB", "nN", ",?", ";.", ":/", "!�",
	"<>"
};

/*** Icelandic keyboard layout (contributed by R�khar�ur Egilsson) */
static const char main_key_IS[MAIN_LEN][4] = {
	"�", "1!", "2\"", "3#", "4$", "5%", "6&", "7/{", "8([", "9)]", "0=}",
	"��\\", "-_",
	"qQ@", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "��",
	"'?~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "�^", "+*`",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "��",
	"<>|"
};

/*** German keyboard layout (contributed by Ulrich Weigand) */
static const char main_key_DE[MAIN_LEN][4] = {
	"^�", "1!", "2\"�", "3��", "4$", "5%", "6&", "7/{", "8([", "9)]", "0=}",
	"�?\\", "'`",
	"qQ@", "wW", "eE�", "rR", "tT", "zZ", "uU", "iI", "oO", "pP", "��",
	"+*~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��", "#�",
	"yY", "xX", "cC", "vV", "bB", "nN", "mM�", ",;", ".:", "-_",
	"<>|"
};

/*** German keyboard layout without dead keys */
static const char main_key_DE_nodead[MAIN_LEN][4] = {
	"^�", "1!", "2\"", "3�", "4$", "5%", "6&", "7/{", "8([", "9)]", "0=}",
	"�?\\", "�",
	"qQ", "wW", "eE", "rR", "tT", "zZ", "uU", "iI", "oO", "pP", "��", "+*~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��", "#'",
	"yY", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>"
};

/*** Swiss German keyboard layout (contributed by Jonathan Naylor) */
static const char main_key_SG[MAIN_LEN][4] = {
	"��", "1+|", "2\"@", "3*#", "4�", "5%", "6&�", "7/�", "8(�", "9)", "0=",
	"'?�", "^`~",
	"qQ", "wW", "eE", "rR", "tT", "zZ", "uU", "iI", "oO", "pP", "��[",
	"�!]",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��{",
	"$�}",
	"yY", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>\\"
};

/*** Swiss French keyboard layout (contributed by Philippe Froidevaux) */
static const char main_key_SF[MAIN_LEN][4] = {
	"��", "1+|", "2\"@", "3*#", "4�", "5%", "6&�", "7/�", "8(�", "9)", "0=",
	"'?�", "^`~",
	"qQ", "wW", "eE", "rR", "tT", "zZ", "uU", "iI", "oO", "pP", "��[",
	"�!]",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��{",
	"$�}",
	"yY", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>\\"
};

/*** Norwegian keyboard layout (contributed by Ove K�ven) */
static const char main_key_NO[MAIN_LEN][4] = {
	"|�", "1!", "2\"@", "3#�", "4�$", "5%", "6&", "7/{", "8([", "9)]",
	"0=}", "+?", "\\`�",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "��", "�^~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��", "'*",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>"
};

/*** Danish keyboard layout (contributed by Bertho Stultiens) */
static const char main_key_DA[MAIN_LEN][4] = {
	"��", "1!", "2\"@", "3#�", "4�$", "5%", "6&", "7/{", "8([", "9)]",
	"0=}", "+?", "�`|",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "��", "�^~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��", "'*",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>\\"
};

/*** Swedish keyboard layout (contributed by Peter Bortas) */
static const char main_key_SE[MAIN_LEN][4] = {
	"��", "1!", "2\"@", "3#�", "4�$", "5%", "6&", "7/{", "8([", "9)]",
	"0=}", "+?\\", "�`",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "��", "�^~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��", "'*",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>|"
};

/*** Canadian French keyboard layout */
static const char main_key_CF[MAIN_LEN][4] = {
	"#|\\", "1!�", "2\"@", "3/�", "4$�", "5%�", "6?�", "7&�", "8*�", "9(�",
	"0)�", "-_�", "=+�",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO�", "pP�", "^^[",
	"��]",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:~", "``{",
	"<>}",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",'-", ".", "��",
	"���"
};

/*** Portuguese keyboard layout */
static const char main_key_PT[MAIN_LEN][4] = {
	"\\�", "1!", "2\"@", "3#�", "4$�", "5%", "6&", "7/{", "8([", "9)]",
	"0=}", "'?", "��",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "+*\\�",
	"\\'\\`",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "��",
	"\\~\\^",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>"
};

/*** Italian keyboard layout */
static const char main_key_IT[MAIN_LEN][4] = {
	"\\|", "1!�", "2\"�", "3��", "4$�", "5%�", "6&�", "7/{", "8([", "9)]",
	"0=}", "'?`", "�^~",
	"qQ@", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO�", "pP�", "��[",
	"+*]",
	"aA", "sS�", "dD�", "fF", "gG", "hH", "jJ", "kK", "lL", "��@", "�#",
	"��",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM�", ",;", ".:�", "-_",
	"<>|"
};

/*** Finnish keyboard layout */
static const char main_key_FI[MAIN_LEN][4] = {
	"", "1!", "2\"@", "3#", "4$", "5%", "6&", "7/{", "8([", "9)]", "0=}",
	"+?\\", "\'`",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "", "\"^~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "", "", "'*",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>|"
};

/*** Russian keyboard layout (contributed by Pavel Roskin) */
static const char main_key_RU[MAIN_LEN][4] = {
	"`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_",
	"=+",
	"qQ��", "wW��", "eE��", "rR��", "tT��", "yY��", "uU��", "iI��", "oO��",
	"pP��", "[{��", "]}��",
	"aA��", "sS��", "dD��", "fF��", "gG��", "hH��", "jJ��", "kK��", "lL��",
	";:��", "'\"��", "\\|",
	"zZ��", "xX��", "cC��", "vV��", "bB��", "nN��", "mM��", ",<��", ".>��",
	"/?"
};

/*** Russian keyboard layout KOI8-R */
static const char main_key_RU_koi8r[MAIN_LEN][4] = {
	"()", "1!", "2\"", "3/", "4$", "5:", "6,", "7.", "8;", "9?", "0%", "-_",
	"=+",
	"��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	"��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "\\|",
	"��", "��", "��", "��", "��", "��", "��", "��", "��", "/?",
	"<>"			/* the phantom key */
};

/*** Spanish keyboard layout (contributed by Jos� Marcos L�pez) */
static const char main_key_ES[MAIN_LEN][4] = {
	"��\\", "1!|", "2\"@", "3�#", "4$", "5%", "6&�", "7/", "8(", "9)", "0=",
	"'?", "��",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "`^[",
	"+*]",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "'�{",
	"��}",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>"
};

/*** Belgian keyboard layout ***/
static const char main_key_BE[MAIN_LEN][4] = {
	"", "&1|", "�2@", "\"3#", "'4", "(5", "�6^", "�7", "!8", "�9{", "�0}",
	")�", "-_",
	"aA", "zZ", "eE�", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "^�[",
	"$*]",
	"qQ", "sS�", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "mM", "�%�",
	"��`",
	"wW", "xX", "cC", "vV", "bB", "nN", ",?", ";.", ":/", "=+~",
	"<>\\"
};

/*** Hungarian keyboard layout (contributed by Zolt�n Kov�cs) */
static const char main_key_HU[MAIN_LEN][4] = {
	"0�", "1'~", "2\"�", "3+^", "4!�", "5%�", "6/�", "7=`", "8(�", "9)�",
	"�ֽ", "�ܨ", "�Ӹ",
	"qQ\\", "wW|", "eE", "rR", "tT", "zZ", "uU", "iI�", "oO�", "pP", "���",
	"���",
	"aA", "sS�", "dD�", "fF[", "gG]", "hH", "jJ�", "kK�", "lL�", "��$",
	"���", "�ۤ",
	"yY>", "xX#", "cC&", "vV@", "bB{", "nN}", "mM", ",?;", ".:�", "-_*",
	"��<"
};

/*** Polish (programmer's) keyboard layout ***/
static const char main_key_PL[MAIN_LEN][4] = {
	"`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&�", "8*", "9(", "0)", "-_",
	"=+",
	"qQ", "wW", "eE��", "rR", "tT", "yY", "uU", "iI", "oO��", "pP", "[{",
	"]}",
	"aA��", "sS��", "dD", "fF", "gG", "hH", "jJ", "kK", "lL��", ";:", "'\"",
	"\\|",
	"zZ��", "xX��", "cC��", "vV", "bB", "nN��", "mM", ",<", ".>", "/?",
	"<>|"
};

/*** Croatian keyboard layout specific for me <jelly@srk.fer.hr> ***/
static const char main_key_HR_jelly[MAIN_LEN][4] = {
	"`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_",
	"=+",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{��",
	"]}��",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:��", "'\"��",
	"\\|��",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?",
	"<>|"
};

/*** Croatian keyboard layout ***/
static const char main_key_HR[MAIN_LEN][4] = {
	"��", "1!", "2\"�", "3#^", "4$�", "5%�", "6&�", "7/`", "8(�", "9)�",
	"0=�", "'?�", "+*�",
	"qQ\\", "wW|", "eE", "rR", "tT", "zZ", "uU", "iI", "oO", "pP", "���",
	"���",
	"aA", "sS", "dD", "fF[", "gG]", "hH", "jJ", "kK�", "lL�", "��", "���",
	"���",
	"yY", "xX", "cC", "vV@", "bB{", "nN}", "mM�", ",;", ".:", "-_/",
	"<>"
};

/*** Japanese 106 keyboard layout ***/
static const char main_key_JA_jp106[MAIN_LEN][4] = {
	"1!", "2\"", "3#", "4$", "5%", "6&", "7'", "8(", "9)", "0~", "-=", "^~",
	"\\|",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "@`", "[{",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";+", ":*", "]}",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?",
	"\\_",
};

/*** Japanese pc98x1 keyboard layout ***/
static const char main_key_JA_pc98x1[MAIN_LEN][4] = {
	"1!", "2\"", "3#", "4$", "5%", "6&", "7'", "8(", "9)", "0", "-=", "^`",
	"\\|",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "@~", "[{",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";+", ":*", "]}",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?",
	"\\_",
};

/*** Brazilian ABNT-2 keyboard layout (contributed by Raul Gomes Fernandes) */
static const char main_key_PT_br[MAIN_LEN][4] = {
	"'\"", "1!", "2@", "3#", "4$", "5%", "6\"", "7&", "8*", "9(", "0)",
	"-_", "=+",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "'`", "[{",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "~^", "]}",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?"
};

/*** US international keyboard layout (contributed by Gustavo Noronha (kov@debian.org)) */
static const char main_key_US_intl[MAIN_LEN][4] = {
	"`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_",
	"=+", "\\|",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{", "]}",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:", "'\"",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?"
};

/*** Slovak keyboard layout (see cssk_ibm(sk_qwerty) in xkbsel)
  - dead_abovering replaced with degree - no symbol in iso8859-2
  - brokenbar replaced with bar					*/
static const char main_key_SK[MAIN_LEN][4] = {
	";�`'", "+1", "�2", "�3", "�4", "�5", "�6", "�7", "�8", "�9", "�0)",
	"=%", "",
	"qQ\\", "wW|", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "�/�",
	"�(�",
	"aA", "sS�", "dD�", "fF[", "gG]", "hH", "jJ", "kK�", "lL�", "�\"$",
	"�!�", "�)�",
	"zZ>", "xX#", "cC&", "vV@", "bB{", "nN}", "mM", ",?<", ".:>", "-_*",
	"<>\\|"
};

/*** Slovak and Czech (programmer's) keyboard layout (see cssk_dual(cs_sk_ucw)) */
static const char main_key_SK_prog[MAIN_LEN][4] = {
	"`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_",
	"=+",
	"qQ��", "wW��", "eE��", "rR��", "tT��", "yY��", "uU��", "iI��", "oO��",
	"pP��", "[{", "]}",
	"aA��", "sS��", "dD��", "fF��", "gG��", "hH��", "jJ��", "kK��", "lL��",
	";:", "'\"", "\\|",
	"zZ��", "xX�", "cC��", "vV��", "bB", "nN��", "mM��", ",<", ".>", "/?",
	"<>"
};

/*** Czech keyboard layout (see cssk_ibm(cs_qwerty) in xkbsel) */
static const char main_key_CS[MAIN_LEN][4] = {
	";", "+1", "�2", "�3", "�4", "�5", "�6", "�7", "�8", "�9", "�0�)", "=%",
	"",
	"qQ\\", "wW|", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "�/[{",
	")(]}",
	"aA", "sS�", "dD�", "fF[", "gG]", "hH", "jJ", "kK�", "lL�", "�\"$",
	"�!�", "�'",
	"zZ>", "xX#", "cC&", "vV@", "bB{", "nN}", "mM", ",?<", ".:>", "-_*",
	"<>\\|"
};

/*** Latin American keyboard layout (contributed by Gabriel Orlando Garcia) */
static const char main_key_LA[MAIN_LEN][4] = {
	"|��", "1!", "2\"", "3#", "4$", "5%", "6&", "7/", "8(", "9)", "0=",
	"'?\\", "��",
	"qQ@", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "��",
	"+*~",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "{[^",
	"}]`",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",;", ".:", "-_",
	"<>"
};

/*** Lithuanian (Baltic) keyboard layout (contributed by Nerijus Bali�nas) */
static const char main_key_LT_B[MAIN_LEN][4] = {
	"`~", "��", "��", "��", "��", "��", "��", "��", "��", "((", "))", "-_",
	"��",
	"qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{", "]}",
	"aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:", "'\"",
	"\\|",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?"
};

/*** Turkish keyboard Layout */
static const char main_key_TK[MAIN_LEN][4] = {
	"\"�", "1!", "2'", "3^#", "4+$", "5%", "6&", "7/{", "8([", "9)]", "0=}",
	"*?\\", "-_",
	"qQ@", "wW", "eE", "rR", "tT", "yY", "uU", "�I�", "oO", "pP", "��",
	"��~",
	"aA�", "sS�", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", "��", "i�",
	",;`",
	"zZ", "xX", "cC", "vV", "bB", "nN", "mM", "��", "��", ".:"
};

/*** Layout table. Add your keyboard mappings to this list */
