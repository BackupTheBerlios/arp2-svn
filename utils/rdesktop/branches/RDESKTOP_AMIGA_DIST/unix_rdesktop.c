/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Amiga front-end etc
   Copyright (C) Martin Blom 2004

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

#include "rdesktop.h"

#undef BOOL
#define Bool int

#include <stdarg.h>		/* va_list va_start va_end */
#include <unistd.h>		/* read close getuid getgid getpid getppid gethostname */
#include <fcntl.h>		/* open */
#include <pwd.h>		/* getpwuid */
#include <termios.h>		/* tcgetattr tcsetattr */
#include <sys/stat.h>		/* stat */
#include <sys/time.h>		/* gettimeofday */
#include <sys/times.h>		/* times */
#include <ctype.h>		/* toupper */
#include <errno.h>

extern char g_title[64];
extern char g_username[64];
extern char hostname[16];
extern char keymapname[16];
extern int keylayout;
extern int g_width;
extern int g_height;
extern int tcp_port_rdp;
extern int g_server_bpp;
extern int g_win_button_size;
extern Bool g_bitmap_compression;
extern Bool g_sendmotion;
extern Bool g_orders;
extern Bool g_encryption;
extern Bool packet_encryption;
extern Bool g_desktop_save;
extern Bool g_fullscreen;
extern Bool g_grab_keyboard;
extern Bool g_hide_decorations;
extern Bool g_use_rdp5;
extern Bool g_console_session;
extern Bool g_numlock_sync;
extern Bool g_owncolmap;
extern Bool g_ownbackstore;
extern uint32 g_embed_wnd;
extern uint32 g_rdp5_performanceflags;
// RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;

#ifdef WITH_RDPSND
extern Bool g_rdpsnd;
#endif

extern RDPDR_DEVICE g_rdpdr_device[];
extern uint32 g_num_devices;
extern char *g_rdpdr_clientname;

static const char template[] =
                "USER/K,DOMAIN/K,PASSWORD/K,CLIENT/K,CONSOLE/S,FRENCH/S,"
                "NOENC=NOENCRYPTION/S,SHELL/K,DIR=DIRECTORY/K,LEFT/K/N,"
                "TOP/K/N,W=WIDTH/K/N,H=HEIGHT/K/N,D=DEPTH/K/N,PUBSCREEN/K,"
                "TITLE/K,FS=FULLSCREEN/S,SCREENMODE/K/N,AUDIO/K/N,"
                "REMOTEAUDIO/S,BITMAPSONLY/S,NOMOUSE/S,EXP=EXPERIENCE/K,"
                "RDP/S,SERVER/A";


static Bool
read_password(char *password, int size)
{
/* 	struct termios tios; */
/* 	Bool ret = False; */
/* 	int istty = 0; */
/* 	char *p; */

/* //	if (tcgetattr(STDIN_FILENO, &tios) == 0) */
/* 	{ */
/* 		fprintf(stderr, "Password: "); */
/* 		tios.c_lflag &= ~ECHO; */
/* //		tcsetattr(STDIN_FILENO, TCSANOW, &tios); */
/* 		istty = 1; */
/* 	} */

/* 	if (fgets(password, size, stdin) != NULL) */
/* 	{ */
/* 		ret = True; */

/* 		/\* strip final newline *\/ */
/* 		p = strchr(password, '\n'); */
/* 		if (p != NULL) */
/* 			*p = 0; */
/* 	} */

/* 	if (istty) */
/* 	{ */
/* 		tios.c_lflag |= ECHO; */
/* //		tcsetattr(STDIN_FILENO, TCSANOW, &tios); */
/* 		fprintf(stderr, "\n"); */
/* 	} */

/* 	return ret; */
}

static void
parse_server_and_port(char *server)
{
	char *p;

	p = strchr(server, ':');
	if (p != NULL)
	{
		tcp_port_rdp = strtol(p + 1, NULL, 10);
		*p = 0;
	}
}

/* Client program */
int
main(int argc, char *argv[])
{
	char server[64];
	char fullhostname[64];
	char domain[16];
	char password[64];
	char shell[128];
	char directory[32];
	Bool prompt_password, rdp_retval = False;
	struct passwd *pw;
	uint32 flags;
	char *p;
	int c;

	int username_option = 0;

	flags = RDP_LOGON_NORMAL;
	prompt_password = False;
	domain[0] = password[0] = shell[0] = directory[0] = 0;
	strcpy(keymapname, "en-us");
	g_embed_wnd = 0;

	g_num_devices = 0;

#ifdef RDP2VNC
#define VNCOPT "V:Q:"
#else
#define VNCOPT
#endif

	while ((c = getopt(argc, argv, VNCOPT "u:d:s:c:p:n:k:g:fbBeEmCDKS:T:NX:a:x:r:045h?")) != -1)
	{
		switch (c)
		{
#ifdef RDP2VNC
			case 'V':
				rfb_port = strtol(optarg, NULL, 10);
				if (rfb_port < 100)
					rfb_port += 5900;
				break;

			case 'Q':
				defer_time = strtol(optarg, NULL, 10);
				if (defer_time < 0)
					defer_time = 0;
				break;
#endif

			case 'u':
				STRNCPY(g_username, optarg, sizeof(g_username));
				username_option = 1;
				break;

			case 'd':
				STRNCPY(domain, optarg, sizeof(domain));
				break;

			case 's':
				STRNCPY(shell, optarg, sizeof(shell));
				break;

			case 'c':
				STRNCPY(directory, optarg, sizeof(directory));
				break;

			case 'p':
				if ((optarg[0] == '-') && (optarg[1] == 0))
				{
					prompt_password = True;
					break;
				}

				STRNCPY(password, optarg, sizeof(password));
				flags |= RDP_LOGON_AUTO;

				/* try to overwrite argument so it won't appear in ps */
				p = optarg;
				while (*p)
					*(p++) = 'X';
				break;

			case 'n':
				STRNCPY(hostname, optarg, sizeof(hostname));
				break;

			case 'k':
				STRNCPY(keymapname, optarg, sizeof(keymapname));
				break;

			case 'g':
				g_fullscreen = False;
				if (!strcmp(optarg, "workarea"))
				{
					g_width = g_height = 0;
					break;
				}

				g_width = strtol(optarg, &p, 10);
				if (g_width <= 0)
				{
					error("invalid geometry\n");
					return 1;
				}

				if (*p == 'x')
					g_height = strtol(p + 1, NULL, 10);

				if (g_height <= 0)
				{
					error("invalid geometry\n");
					return 1;
				}

				if (*p == '%')
					g_width = -g_width;

				break;

			case 'f':
				g_fullscreen = True;
				break;

			case 'b':
				g_orders = False;
				break;

			case 'B':
				g_ownbackstore = False;
				break;

			case 'e':
				g_encryption = False;
				break;
			case 'E':
				packet_encryption = False;
				break;
			case 'm':
				g_sendmotion = False;
				break;

			case 'C':
				g_owncolmap = True;
				break;

			case 'D':
				g_hide_decorations = True;
				break;

			case 'K':
				g_grab_keyboard = False;
				break;

			case 'S':
				if (!strcmp(optarg, "standard"))
				{
					g_win_button_size = 18;
					break;
				}

				g_win_button_size = strtol(optarg, &p, 10);

				if (*p)
				{
					error("invalid button size\n");
					return 1;
				}

				break;

			case 'T':
				STRNCPY(g_title, optarg, sizeof(g_title));
				break;

			case 'N':
				g_numlock_sync = True;
				break;

			case 'X':
				g_embed_wnd = strtol(optarg, NULL, 10);
				break;

			case 'a':
				g_server_bpp = strtol(optarg, NULL, 10);
				if (g_server_bpp != 8 && g_server_bpp != 16 && g_server_bpp != 15
				    && g_server_bpp != 24)
				{
					error("invalid server bpp\n");
					return 1;
				}
				break;

			case 'x':

				if (strncmp("modem", optarg, 1) == 0)
				{
					g_rdp5_performanceflags =
						RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
						RDP5_NO_MENUANIMATIONS | RDP5_NO_THEMING;
				}
				else if (strncmp("broadband", optarg, 1) == 0)
				{
					g_rdp5_performanceflags = RDP5_NO_WALLPAPER;
				}
				else if (strncmp("lan", optarg, 1) == 0)
				{
					g_rdp5_performanceflags = RDP5_DISABLE_NOTHING;
				}
				else
				{
					g_rdp5_performanceflags = strtol(optarg, NULL, 16);
				}
				break;

			case 'r':

				if (strncmp("sound", optarg, 5) == 0)
				{
					optarg += 5;

					if (*optarg == ':')
					{
						*optarg++;
						while ((p = next_arg(optarg, ',')))
						{
							if (strncmp("remote", optarg, 6) == 0)
								flags |= RDP_LOGON_LEAVE_AUDIO;

							if (strncmp("local", optarg, 5) == 0)
#ifdef WITH_RDPSND
								g_rdpsnd = True;
#else
								warning("Not compiled with sound support");
#endif

#ifdef WITH_RDPSND
							if (strncmp("off", optarg, 3) == 0)
								g_rdpsnd = False;
#endif
							optarg = p;
						}
					}
					else
					{
#ifdef WITH_RDPSND
						g_rdpsnd = True;
#else
						warning("Not compiled with sound support");
#endif
					}
				}
#ifdef lcs
				else if (strncmp("disk", optarg, 4) == 0)
				{
					/* -r disk:h:=/mnt/floppy */
					disk_enum_devices(&g_num_devices, optarg + 4);
				}
				else if (strncmp("comport", optarg, 7) == 0)
				{
					serial_enum_devices(&g_num_devices, optarg + 7);
				}
				else if (strncmp("lptport", optarg, 7) == 0)
				{
					parallel_enum_devices(&g_num_devices, optarg + 7);
				}
				else if (strncmp("printer", optarg, 7) == 0)
				{
					printer_enum_devices(&g_num_devices, optarg + 7);
				}
#endif
				else if (strncmp("clientname", optarg, 7) == 0)
				{
					g_rdpdr_clientname = xmalloc(strlen(optarg + 11) + 1);
					strcpy(g_rdpdr_clientname, optarg + 11);
				}
				else
				{
					warning("Unknown -r argument\n\n\tPossible arguments are: comport, disk, lptport, printer, sound\n");
				}
				break;

			case '0':
				g_console_session = True;
				break;

			case '4':
				g_use_rdp5 = False;
				break;

			case '5':
				g_use_rdp5 = True;
				break;

			case 'h':
			case '?':
			default:
//				usage(argv[0]);
				return 1;
		}
	}

	if (argc - optind != 1)
	{
//		usage(argv[0]);
		return 1;
	}

	STRNCPY(server, argv[optind], sizeof(server));
	parse_server_and_port(server);

	if (!username_option)
	{
		pw = getpwuid(getuid());
		if ((pw == NULL) || (pw->pw_name == NULL))
		{
			error("could not determine username, use -u\n");
			return 1;
		}

		STRNCPY(g_username, pw->pw_name, sizeof(g_username));
	}

	if (hostname[0] == 0)
	{
		if (gethostname(fullhostname, sizeof(fullhostname)) == -1)
		{
			error("could not determine local hostname, use -n\n");
			return 1;
		}

		p = strchr(fullhostname, '.');
		if (p != NULL)
			*p = 0;

		STRNCPY(hostname, fullhostname, sizeof(hostname));
	}

	if (prompt_password && read_password(password, sizeof(password)))
		flags |= RDP_LOGON_AUTO;

	if (g_title[0] == 0)
	{
		strcpy(g_title, "rdesktop - ");
		strncat(g_title, server, sizeof(g_title) - sizeof("rdesktop - "));
	}

#ifdef RDP2VNC
	rdp2vnc_connect(server, flags, domain, password, shell, directory);
	return 0;
#else

	if (!ui_init())
		return 1;

#ifdef WITH_RDPSND
	if (g_rdpsnd)
		rdpsnd_init();
#endif
	rdpdr_init();

	if (!rdp_connect(server, flags, domain, password, shell, directory))
		return 1;

	/* By setting encryption to False here, we have an encrypted login 
	   packet but unencrypted transfer of other packets */
	if (!packet_encryption)
		g_encryption = False;


	DEBUG(("Connection successful.\n"));
	memset(password, 0, sizeof(password));

	cache_create();
	if (ui_create_window())
	{
		rdp_retval = rdp_main_loop();
		cache_destroy();
		ui_destroy_window();
	}

	DEBUG(("Disconnecting...\n"));
	rdp_disconnect();
	ui_deinit();

	if (True == rdp_retval)
		return 0;
	else
		return 2;

#endif

}

/* report an error */
void
error(char *format, ...)
{
	va_list ap;

	fprintf(stderr, "ERROR: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

/* report a warning */
void
warning(char *format, ...)
{
	va_list ap;

	fprintf(stderr, "WARNING: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

/* report an unimplemented protocol feature */
void
unimpl(char *format, ...)
{
	va_list ap;

	fprintf(stderr, "NOT IMPLEMENTED: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}
