( THIS FILE IS OUTDATED - UPDATEME, short tip: run ./configure --help )

read the manpage... ;)


DEPENDENCIES:
	from pl 19-6 and up, you will need to have the gmp lib installed on your system, this is usually the case, but perhaps you are missing the header files, i.e. Mandrake 8.0. gmp compiles on almost any platform, and is optimized for most of them. It's available at http://www.swox.com/gmp/ This resolves an issue with one of the libs that limited the use of rdesktop to noncomersial use only.
	from pl 19-6-1 and up, ssl is needed. preferably 0.9.6 or similar. Instead of having parts of the code internal in rdesktop we choose to use the lib, which means you will need to have ssl installed.


LONG SWITCHES:

we have included getopt_long from the gnu libc sources in the gnu/ directory, so long
switches should now work on all platforms.

KEYBOARD PROBLEMS:

It is always wise to specify your keyboard layout, like -k us, -k gr etc, also
it may or may not be wise to use the -N flag as well, depending on your setup.

If your layout is not yet in keymap.c then it should be quite trivial to add
it by looking at the existing ones as examples. Please submit the new layout
to the wine project.

If there is an update from wine, you can overwrite your keymap.c (destroying
your changes!) with keymap_rip.sh.


XINERAMA:

edit the Makefile, uncomment the lines:
#XINERAMA_CFLAGS = -DHAVE_XINERAMA
#XINERAMA_LIBS = -lXinerama
this will cause the program to link against the xinerama libs, and also enable some code for
the fullscreen mode.

even if you do not have support for xinerama you may link against it, it will still work, but as a
default it will remain disabled for compatibility.

SERVER MODULE:

edit the Makefile, uncomment the line: #all:   rdesktop rdp-srvr
and make "all:    rdesktop" into a comment.

further information about the server is available in the rdp-srvr-readme.txt

the server module is _not_ intended for general usage, it is in an alpha state... You have been warned.










