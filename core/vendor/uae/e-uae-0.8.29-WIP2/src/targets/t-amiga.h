 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff, AmigaOS version
  *
  * Copyright 1997 Bernd Schmidt
  * Copyright 2003-2006 Richard Drummond
  */

#define TARGET_AMIGAOS

#define TARGET_NAME		"amiga"

#define TARGET_ROM_PATH		"PROGDIR:Roms/"
#define TARGET_FLOPPY_PATH	"PROGDIR:Floppies/"
#define TARGET_HARDFILE_PATH	"PROGDIR:HardFiles/"
#define TARGET_SAVESTATE_PATH   "PROGDIR:SaveStates/"

#define UNSUPPORTED_OPTION_l

#define OPTIONSFILENAME ".uaerc"
//#define OPTIONS_IN_HOME

#define TARGET_SPECIAL_OPTIONS \
    { "x",        "  -x           : Does not use dithering\n"}, \
    { "T",        "  -T           : Try to use grayscale\n"},
#define COLOR_MODE_HELP_STRING \
    "\nValid color modes (see -H) are:\n" \
    "     0 => 256 cols max on customscreen;\n" \
    "     1 => OpenWindow on default public screen;\n" \
    "     2 => Ask the user to select a screen mode with ASL requester;\n" \
    "     3 => use a 320x256 graffiti screen.\n\n"

#define DEFSERNAME "ser:"
#define DEFPRTNAME "par:"

#define write_log write_log_amigaos
#define flush_log flush_log_amigaos

#define NO_MAIN_IN_MAIN_C

#if defined __amigaos4__ || defined __morphos__
# define SLEEP_DONT_BUSY_WAIT
#endif
