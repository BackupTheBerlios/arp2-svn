 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff, BeOS version
  *
  * Copyright 1997 Bernd Schmidt
  */

#define TARGET_NAME "beos"

#define TARGET_ROM_PATH         "~/"
#define TARGET_FLOPPY_PATH      "~/"
#define TARGET_HARDFILE_PATH    "~/"
#define TARGET_SAVESTATE_PATH   "~/"

#define OPTIONSFILENAME ".uaerc"

#define DEFPRTNAME "lpr"
#define DEFSERNAME "/dev/ports/serial1"

#define NO_MAIN_IN_MAIN_C

#define write_log write_log_standard
#define flush_log flush_log_standard
#define set_logfile set_logfile_standard

#define SLEEP_DONT_BUSY_WAIT
