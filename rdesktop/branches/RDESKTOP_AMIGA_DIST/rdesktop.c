/*
   rdesktop: A Remote Desktop Protocol client.
   Entrypoint and utility functions
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

#include <stdlib.h>		/* malloc realloc free */
#include <unistd.h>		/* read close getuid getgid getpid getppid gethostname */
#ifdef LIBC_HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "gnu/getopt.h"
#endif
#include <fcntl.h>		/* open */
#include <pwd.h>		/* getpwuid */
#include <sys/stat.h>		/* stat */
#include <sys/time.h>		/* gettimeofday */
#include <sys/times.h>		/* times */
#include <string.h>		/* strcasecmp */

#include "doskbcodes.h"         /* dosKeybCodes[] struct */

#include "rdesktop.h"
char* username = 0;
char* hostname = 0;
int width = -1;
int height = -1;
int wxpos = 0;
int wypos = 0;
int keylayout = 0x409;
int tcp_port_rdp=3389;
#ifdef SERVER
int vnc_port_rdp=5901;
#endif /* SERVER */

int bpp = -1;
Bool bitmap_compression = True;
Bool motion = True;
Bool orders = True;
Bool licence = True;
Bool use_encryption = True;
Bool w2k = True;
Bool desktop_save = True;
Bool grab_keyboard = True;
Bool fullscreen = False;
Bool systembeep = True;
Bool vncviewer = False;
Bool broken_x_kb = False;
int private_colormap = False;
int backing_store = BS_PIXMAP;
int desktopsize_percent=0;

static struct option prog_switches[] = {
  { "user",1,0,'u'},
  { "domain",1,0,'d'},
  { "password",1,0,'p'},
  { "keycode",1, 0,'k'},
  { "clientname",1, 0,'n'},
  { "shell",1,0,'s'},
  { "workingdir",1, 0,'c'},
  { "geometry",1, 0,'g'},
  { "procent",1, 0,'P'},
  { "bpp",1,0,'D'},
  { "wmkeys",0,0,'K'},
  { "fullscreen",0,0,'F'},
  { "no-license",0,0,'l'},
  { "no-enc",0,0,'e'},
  { "no-bitmapcache",0,0,'b'},
  { "no-motionevents",0,0,'m'},
  { "no-desktopcache",0,0,'C'},
  { "no-bitmapcomp",0,0,'B'},
  { "no-systembeep",0,0,'S'},
  { "nt4tsc",0,0,'4'},
  { "port",1,0,'t'},
  { "bs",1,0,'T'},
  { "new-kbcode",0,0,'N'},
  { "VNC",0,0,'V'},
  { "privcolmap",0,0,'v'},
  { NULL,0,0,0}
};

int
find_keyb_code( const char* string )
{
	int t = 0;
	while(dosKeybCodes[t].key != 0 ){
                if (dosKeybCodes[t].short_key != NULL)
                  if (strcasecmp( string, dosKeybCodes[t].short_key ) == 0)
                    return t;
		if( strcasecmp( string, dosKeybCodes[t].key ) == 0 )
			return t;
		t++;
	}

        printf("Bad syntax: \"%s\" wasn't found in keytable, you can use short or long name, valid values are:\n", string);
        printf("Short | Long Name                             | Code       | New Keymap\n");
        printf("-------------------------------------------------------------------------\n");
        t=0;
        while(dosKeybCodes[t].key != 0){
               if (dosKeybCodes[t].short_key != NULL) 
                 printf("%-6s", dosKeybCodes[t].short_key);
               else
                 printf("      ");
               printf("| %-37s ", dosKeybCodes[t].key);
               printf("| 0x%08x ", dosKeybCodes[t].code);
               if (dosKeybCodes[t].layout != NULL )
                 printf("| Yes\n");
               else
                 printf("| No\n");
               t++;
        }
        return -1;
}


/* Display usage information */
static void
usage (char *program)
{
  STATUS ("Usage: %s [options] server\n", program);
  STATUS ("   -u --user\n");
  STATUS ("   -d --domain\n");
  STATUS ("   -s --shell\n");
  STATUS ("   -c --workingdir:\tworking directory\n");
  STATUS ("   -p --password:\tpassword (autologon) -=ask !=empty\n");
  STATUS ("   -n --clientname:\tclient hostname\n");
  STATUS ("   -g --geometry:\t<width>x<height>[+<xoff>[+<yoff>]], like \"-g 1024x768+0+0\"\n");
  STATUS ("   -k --keycode:\tkeyboard layout on terminal server. (us,sv,gr etc.)\n");
  STATUS ("   -F --fullscreen:\tfull screen mode\n");
  STATUS ("   -P --procent:\tdesktop size in percent of current screen\n");
  STATUS ("   -K --vmkeys:\tenable window manager's key bindings\n");
  STATUS ("   -D --bpp:\tbits per pixel\n");
  STATUS ("   -t --port:\trdp tcp port\n");
  STATUS ("   -T --bs:\tselect the backing-store method (x|p|n)\n");
  STATUS ("\n");
  STATUS ("   -e --no-enc:\tdo not use encryption, french ts' among others.\n");
  STATUS ("   -l --no-license:\tdo not request license\n");
  STATUS ("   --no-bitmapcache:\tforce bitmap updates\n");
  STATUS ("   --no-bitmapcomp:\tdisable bitmap compress\n");
  STATUS ("   --no-desktopcache:\tdo not use desktop cache ( desktop save features )\n");
  STATUS ("   --no-motionevents:\tdo not send motion events\n");
  STATUS ("   --no-systembeep:\tignore system beep\n");
  STATUS ("   -v --privcolmap:\tuse a private color map ( usefull for 8bpp mode )\n");
  STATUS ("   -N --new-kbcode:\tnot using a PC console (formerly BROKEN_X_KB option)\n");
  STATUS ("   -V --VNC:\tuse a VNC viewer\n");
  STATUS ("\n");
  STATUS ("   -4 --nt4tsc:\tserver end is NT4 TSC comp. ( defaults to w2k comp. )\n");
  STATUS ("\n");

}

/* Client program */
int
main (int argc, char *argv[])
{
  struct passwd *pw;
  char *server;
  uint32 flags;
  char *domain = 0; //[16];
  char *password = 0; //[16];
  char *shell = 0; //[64];
  char *directory = 0; //[32];
  char *title = 0; //[32];
  int language_id = -1;
  int c;
  int option_index = 0;


  STATUS ("rdesktop: A Remote Desktop Protocol client.\n");
  STATUS ("Version " VERSION ". Copyright (C) 1999-2000 Matt Chapman.\n");
  STATUS ("See http://www.rdesktop.org/ for more information.\n\n");
  flags = RDP_LOGON_NORMAL;
  while ((c = getopt_long( argc, argv, "u:d:p:k:n:s:c:g:P:D:t:T:KFlebhmCB4SvVN?", prog_switches, &option_index)) != -1)
    {
      switch (c)

	{
	case 'u':
	  username = (char *) xmalloc(strlen(optarg) + 1);
	  strcpy (username, optarg );
	  break;
	case 'd':
          domain = (char*)xmalloc(strlen(optarg) + 1);
	  strcpy (domain, optarg);
	  break;
	case 'p':
	  flags |= RDP_LOGON_AUTO;
          if (strcmp (optarg, "-") == 0) {
	    /* Read password from terminal or stdin */
	    if (isatty (fileno (stdin)))
	    {
	      char *pw;
              pw = getpass("Password: ");
              password = (char*)xmalloc(strlen(pw) + 1);
	      strcpy (password, pw);
	    } else {
              password = (char*)xmalloc(256); // static length
	      if (fgets ((char *)&password, 256, stdin) == NULL)
	      {
		ERROR("fgets\n");
		exit(1);
	      }
	    }
	    if (password[strlen(password)-1] == '\n')
	      password[strlen(password)-1] = '\0'; /* Chop newline */
          } else if (strcmp (optarg, "!") == 0) {
            /* empty password */
            password = (char*)xmalloc(1);
            strcpy (password, "");
          } else {
            password = (char*)xmalloc(strlen(optarg) + 1);
            strcpy (password, optarg);
	  }
	  break;
	case 's':
          shell = (char*)xmalloc(strlen(optarg) + 1);
          strcpy (shell, optarg);
	  break;
	case 'c':
          directory = (char*)xmalloc(strlen(optarg) + 1);
          strcpy (directory, optarg);
	  break;
	case 'n':
          hostname = (char*)xmalloc(strlen(optarg) + 1);
	  strcpy (hostname, optarg);
	  break;
	case 'g':
	  {
		  char* tgem = 0;
		  width = strtol ( optarg, NULL, 10 );
		  tgem = strchr( optarg, 'x' );
		  if( (tgem == 0) || (strlen(tgem) < 2) ){
			ERROR("-g: invalid parameter. Syntax example: -g 1024x768\n");
			exit(1);
		  }
		  height = strtol( tgem+1, NULL, 10 );
                 if ((tgem = strchr( tgem+1, '+' )) != NULL) {
                       wxpos = strtol( tgem+1, NULL, 10 );
                       if ((tgem = strchr( tgem+1, '+' )) != NULL) {
                               wypos = strtol( tgem+1, NULL, 10 );
                       }
                 }
	  }
	  break;
	case 'k':
	  language_id = find_keyb_code( optarg );
	  if( language_id == -1 )
	    return 1;
          keylayout = dosKeybCodes[language_id].code;
	  break;
	case 'm':
	  motion = False;
	  break;
	case 'b':
	  orders = False;
	  break;
	case 'l':
	  licence = False;
	  break;
	case 'e':
	  use_encryption = False;
	  break;
	case '4':
	  w2k = False;
	  break;
	case 'D':
	  bpp = strtol (optarg, NULL, 10);
	  break;
	case 'P':
	  desktopsize_percent = strtol (optarg, NULL, 10);
	  if(!(desktopsize_percent>0 && desktopsize_percent <= 100)){
            ERROR("-P: invalid number\n");
            exit(1);
          }
          break;
	case 'B':
	  bitmap_compression = False;
	  break;
	case 'C':
	  desktop_save = False;
	  break;
        case 'K':
          grab_keyboard = False;
          break;
        case 'F':
          fullscreen = True;
          break;
        case 't':
	  tcp_port_rdp = strtol (optarg, NULL, 10);
	  break;
        case 'S':
          systembeep = False;
          break;
        case 'T':
	  switch (optarg[0]) {
	    case 'n':
	      backing_store = BS_NONE;
	      break;
	    case 'x':
	      backing_store = BS_XWIN;
	      break;
	    case 'p':
	      backing_store = BS_PIXMAP;
	      break;
	    default:
              ERROR("-T --bs: invalid parameter. Use 'x' for xwindow, 'p' for pixmap or 'n' for none.\n");
	      exit(1);
	  }
          break;
	case 'v':
          private_colormap = True;
	  break;
        case 'V':
          vncviewer = True;
          broken_x_kb = True;
          break;
        case 'N':
          broken_x_kb = True;
          break;
        case 'h':
        case '?':
	default:
	  usage (argv[0]);
	  return 1;
	}
    }
  if (argc - optind < 1)

    {
      usage (argv[0]);
      return 1;
    }

  if ((broken_x_kb) && (keylayout))
     init_keycodes(language_id);

  server = argv[optind];
  if (username == 0 )

    {
      pw = getpwuid (getuid ());
      if ((pw == NULL) || (pw->pw_name == NULL))

	{
	  STATUS ("Could not determine user name.\n");
	  return 1;
	}
      username = (char*)xmalloc(strlen(pw->pw_name) + 1);
      strcpy (username, pw->pw_name);
    }
  if (hostname == 0)
    {
      hostname = (char*)xmalloc(128);

      if (gethostname (hostname, 128) == -1)

	{
	  STATUS ("Could not determine host name.\n");
	  return 1;
	}
    }


  title = (char*)xmalloc(strlen(server) + 13);
  strcpy (title, "rdesktop - ");
  strcat (title, server);
#ifdef SERVER
  if (!rdp_listen (server, flags, domain, password, shell, directory))
    return 1;
  STATUS ("Connection successful.\n");
  if( rfb_connect(server, password) )
    {
      rdp_smain_loop ();
    }
  printf ("disconnecting\n");
  rfb_disconnect ();
#else /* SERVER */
  if (ui_create_window (title))

    {
      if (!rdp_connect (server, flags, domain, password, shell, directory))
      {
        ui_destroy_window ();
        return 1;
      }

      STATUS ("Connection successful.\n");
      rdp_main_loop ();
      ui_destroy_window ();
    }
  printf ("disconnecting\n");
#endif /* SERVER */
  rdp_disconnect ();

  if( domain != 0 )
        xfree(domain);
  if( password != 0 )
        xfree(password);
  if( shell != 0 )
        xfree(shell);
  if( directory != 0 )
        xfree(directory);
  if( title != 0 )
        xfree(title);
  return 0;
}


/* Generate a 32-byte random for the secure transport code. */
void
generate_random (uint8 * random)
{
  struct stat st;
  uint32 *r = (uint32 *) random;
  int fd;
  struct tms tms_struct;

  /* If we have a kernel random device, use it. */
  if ((fd = open ("/dev/urandom", O_RDONLY)) != -1)

    {
      read (fd, random, 32);
      close (fd);
      return;
    }

  /* Otherwise use whatever entropy we can gather - ideas welcome. */
  r[0] = (getpid ()) | (getppid () << 16);
  r[1] = (getuid ()) | (getgid () << 16);

  /* Tru64/OSF1 apparently needs a struct to times() or it will seg. fault. */
  r[2] = times (&tms_struct);	/* system uptime (clocks) */
  gettimeofday ((struct timeval *) &r[3], NULL);	/* sec and usec */
  stat ("/tmp", &st);
  r[5] = st.st_atime;
  r[6] = st.st_mtime;
  r[7] = st.st_ctime;
}



#ifdef DEBUG_LEAK

/* malloc; exit if out of memory */
void *xmalloc2(int l, char *s, int size)
{
	void *mem = malloc(size);
	if (mem == NULL)
	{
		ERROR("xmalloc %d\n", size);
		exit(1);
	}
	printf("A:%i:%s:%08X:%04i\n", l, s, mem, size);
	return mem;
}

/* realloc; exit if out of memory */
void *xrealloc2(int l, char *s, void *oldmem, int size)
{
	void *mem = realloc(oldmem, size);
	if (mem == NULL)
	{
		ERROR("xrealloc %d\n", size);
		exit(1);
	}
	printf("R:%i:%s:%08X:%08X:%04i\n", l, s, oldmem, mem, size);
	return mem;
}

/* free */
void xfree2(int l, char *s, void *mem)
{
	printf("F:%i:%s:%08X\n", l, s, mem);
	free(mem);
}

#else

/* malloc; exit if out of memory */
void *xmalloc(int size)
{
	void *mem = malloc(size);
	if (mem == NULL)
	{
		ERROR("xmalloc %d\n", size);
		exit(1);
	}
	return mem;
}

/* realloc; exit if out of memory */
void *xrealloc(void *oldmem, int size)
{
	void *mem = realloc(oldmem, size);
	if (mem == NULL)
	{
		ERROR("xrealloc %d\n", size);
		exit(1);
	}
	return mem;
}

/* free */
void xfree(void *mem)
{
	free(mem);
}

#endif

/* Produce a hex dump */
void
hexdump (unsigned char *p, unsigned int len)
{
  unsigned char *line = p;
  unsigned int thisline, offset = 0;
  int i;
  while (offset < len)

    {
      STATUS ("%04x ", offset);
      thisline = len - offset;
      if (thisline > 16)
	thisline = 16;
      for (i = 0; i < thisline; i++)
	STATUS ("%02x ", line[i]) for (; i < 16; i++)
	  STATUS ("   ");
      for (i = 0; i < thisline; i++)
	STATUS ("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');
      STATUS ("\n");
      offset += thisline;
      line += thisline;
    }
}




