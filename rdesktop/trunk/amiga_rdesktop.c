/*
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

#include <dos/dos.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>

#include <proto/asl.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/wb.h>

#include <ctype.h>		/* isalpha */
#include <stdarg.h>		/* va_list va_start va_end */
#include <unistd.h>		/* read close getuid getgid getpid getppid gethostname */
#include <pwd.h>		/* getpwuid */

extern int amiga_left;
extern int amiga_top;
extern STRPTR amiga_pubscreen_name;
extern ULONG amiga_screen_id;
extern struct DiskObject* amiga_icon;

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

#ifdef WITH_RDPSND
extern Bool g_rdpsnd;
extern int  amiga_audio_unit;
#endif

extern RDPDR_DEVICE g_rdpdr_device[];
extern uint32 g_num_devices;
extern char *g_rdpdr_clientname;

static const char version[] = "$VER: RDesktop 1.3.1cvs-"
#ifdef __MORPHOS__
                              "MorphOS"
#else
                              "AmigaOS"
#endif
                              " (19.12.2004)"
                              "©2001-2004 Martin Blom; "
                              "©1999-2004 Matthew Chapman et al.";

struct Library*       AslBase       = NULL;
struct Library*       CyberGfxBase  = NULL;
struct GfxBase*       GfxBase       = NULL;
struct Library*       IconBase      = NULL;
struct IntuitionBase* IntuitionBase = NULL;
struct Library*       LayersBase    = NULL;
struct UtilityBase*   UtilityBase   = NULL;
struct Library*       WorkbenchBase = NULL;

static const char template[] =
"SERVER/A,USER/K,DOMAIN/K,PASSWORD/K,CLIENT/K,CONSOLE/S,FRENCH/S,"
"NOENC=NOENCRYPTION/S,SHELL/K,DIR=DIRECTORY/K,LEFT/K/N,TOP/K/N,"
"W=WIDTH/K/N,H=HEIGHT/K/N,D=DEPTH/K/N,PUBSCREEN/K,TITLE/K,"
"FS=FULLSCREEN/S,SCREENMODE/K/N,AUDIO/K/N,REMOTEAUDIO/S,"
"BITMAPSONLY/S,NOMOUSE/S,EXP=EXPERIENCE/K,RDP4/S";

static LONG  def_size  = 0;
static LONG  def_depth = 0;

#define a_first_arg a_server
static STRPTR a_server     = NULL;
static STRPTR a_user       = NULL;
static STRPTR a_domain     = NULL;
static STRPTR a_password   = NULL;
static STRPTR a_client     = NULL;
static ULONG  a_console    = FALSE;
static ULONG  a_french     = FALSE;
static ULONG  a_noenc      = FALSE;
static STRPTR a_shell      = NULL;
static STRPTR a_dir        = NULL;
static ULONG* a_left       = NULL;
static ULONG* a_top        = NULL;
static LONG*  a_width      = &def_size;
static LONG*  a_height     = &def_size;
static ULONG* a_depth      = &def_depth;
static STRPTR a_pubscreen  = NULL;
static STRPTR a_title      = NULL;
static ULONG  a_fullscreen = FALSE;
static ULONG* a_screenmode = NULL;
static ULONG* a_audio      = NULL;
static ULONG  a_remoteaudio = FALSE;
static ULONG  a_bitmapsonly = FALSE;
static ULONG  a_nomouse     = FALSE;
static STRPTR a_experience  = "56K";
static ULONG  a_rdp4        = FALSE;

static struct WBStartup*  wb_msg = NULL;

static Bool
read_password(char *password, int size)
{
  Bool ret = False;  
  char *p;

  fprintf(stderr, "Password: \e[8m");

  if (fgets(password, size, stdin) != NULL)
  {
    ret = True;

    /* strip final newline */
    p = strchr(password, '\n');
    if (p != NULL)
      *p = 0;
  }

  fprintf(stderr, "\e[28m\n");

  return ret;
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

enum { UI_INIT = 1, RDPSND_INIT, RDPDR_INIT, RDP_CONNECT, UI_CREATE_WINDOW };

static int startup = 0;

static void
cleanup(void)
{
  if( amiga_icon != NULL )
  {
    FreeDiskObject( amiga_icon );
  }
  
  switch (startup)
  {
    case UI_CREATE_WINDOW:
      cache_destroy();
      ui_destroy_window();

    case RDP_CONNECT:
      DEBUG(("Disconnecting...\n"));
      rdp_disconnect();

    case RDPDR_INIT:
    case RDPSND_INIT:
#ifdef WITH_RDPSND
      rdpsnd_deinit();
#endif

    case UI_INIT:
      ui_deinit();
  }

  CloseLibrary( CyberGfxBase );
  CyberGfxBase = NULL;

  CloseLibrary( AslBase );
  AslBase = NULL;
  
  CloseLibrary( (struct Library*) GfxBase );
  GfxBase = NULL;
  
  CloseLibrary( (struct Library*) IntuitionBase );
  IntuitionBase = NULL;

  CloseLibrary( IconBase );
  IconBase = NULL;
  
  CloseLibrary( LayersBase );
  LayersBase = NULL;

  CloseLibrary( (struct Library*) UtilityBase );
  UtilityBase = NULL;

  CloseLibrary( WorkbenchBase );
  WorkbenchBase = NULL;
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

  char pubscreen[64];
  struct RDArgs* rdargs = NULL;
  char* wbargs = NULL;
  int rc = RETURN_OK;

  atexit(cleanup);

  AslBase = OpenLibrary( AslName, 39 );
  
  if( AslBase == NULL )
  {
    error( "Unable to open '%s'.\n", AslName );
    return RETURN_FAIL;
  }

  GfxBase = (struct GfxBase*) OpenLibrary( "graphics.library", 39 );
  
  if( GfxBase == NULL )
  {
    error( "Unable to open '%s'.\n", "graphics.library" );
    return RETURN_FAIL;
  }

  IconBase = OpenLibrary("icon.library", 0);

  if( IconBase == NULL )
  {
    error( "Unable to open '%s'.\n", "icon.library" );
  }
      
  IntuitionBase = (struct IntuitionBase*) OpenLibrary( "intuition.library", 39 );
  
  if( IntuitionBase == NULL )
  {
    error( "Unable to open '%s'.\n", "intuition.library" );
    return RETURN_FAIL;
  }

  LayersBase = OpenLibrary( "layers.library", 39 );
  
  if( LayersBase == NULL )
  {
    error( "Unable to open '%s'.\n", "layers.library" );
    return RETURN_FAIL;
  }


  UtilityBase = (struct UtilityBase*) OpenLibrary( "utility.library", 37 );
  
  if( UtilityBase == NULL )
  {
    error( "Unable to open '%s'.\n", "utility.library" );
    return RETURN_FAIL;
  }

  WorkbenchBase = OpenLibrary( "workbench.library", 37 );
  
  if( WorkbenchBase == NULL )
  {
    error( "Unable to open '%s'.\n", "workbench.library" );
    return RETURN_FAIL;
  }
  
  CyberGfxBase = OpenLibrary( "cybergraphics.library", 40 );

  // Don't care if we're unable to open it

  
  flags = RDP_LOGON_NORMAL;
  prompt_password = False;
  domain[0] = password[0] = shell[0] = directory[0] = 0;
  strcpy(keymapname, "en-us");
  g_embed_wnd = 0;

  g_num_devices = 0;

  pw = getpwuid(getuid());
  if ((pw != NULL) && (pw->pw_name != NULL))
  {
    a_user = pw->pw_name;
  }

  if (gethostname(fullhostname, sizeof(fullhostname)) != -1)
  {
    p = strchr(fullhostname, '.');
    if (p != NULL)
      *p = 0;

    a_client = fullhostname;
  }

  if (argc == 0)
  {
    struct DiskObject* icon;
    BPTR cd;
    int length = 1; // space for one nul character
    int i, t;

    cd = CurrentDir( 0 );
    
    wb_msg = (struct WBStartup*) argv;

    for( i = wb_msg->sm_NumArgs - 1; i >= 0; --i )
    {
      CurrentDir( wb_msg->sm_ArgList[ i ].wa_Lock );

      icon = GetDiskObject( wb_msg->sm_ArgList[ i ].wa_Name );

      if( amiga_icon == NULL )
      {
	amiga_icon = icon;
      }
      
      if( icon != NULL && icon->do_ToolTypes != NULL )
      {
	for( t = 0;  icon->do_ToolTypes[ t ] != NULL; ++t )
	{
	  char* tt = icon->do_ToolTypes[ t ];

	  if( isalpha( tt[ 0 ] ) && strncmp( tt, "WINDOW", 6 ) != 0 )
	  {
	    // Add length + space
	    length = length + strlen( tt ) + 1;
	  }
	}

	if( icon != amiga_icon )
	{
	  FreeDiskObject( icon );
	}
      }
    }

    wbargs = malloc( length );
    wbargs[0] = 0;

    if( wbargs != NULL )
    {
      for( i = wb_msg->sm_NumArgs - 1; i >= 0; --i )
      {
	CurrentDir( wb_msg->sm_ArgList[ i ].wa_Lock );

	icon = GetDiskObject( wb_msg->sm_ArgList[ i ].wa_Name );
	
	if( icon != NULL && icon->do_ToolTypes != NULL )
	{
	  for( t = 0;  icon->do_ToolTypes[ t ] != NULL; ++t )
	  {
	    char* tt = icon->do_ToolTypes[ t ];
	    
	    if( isalpha( tt[ 0 ] ) && Strnicmp( tt, "WINDOW", 6 ) != 0 )
	    {
	      int tt_len;
	      char* args = wbargs;

	      for( tt_len = 0; isalnum( tt[ tt_len ] ); ++tt_len );

	      while( 1 )
	      {
		if( Strnicmp( args, tt, tt_len ) == 0 &&
		    ! isalnum( tt[ tt_len ] ) )
		{
		  break;
		}
		else
		{
		  while( *args != 0 )
		  {
		    ++args;

		    if( *args == '\n' )
		    {
		      ++args;
		      break;
		    }
		  }

		  if( *args == 0 )
		  {
		    strcat( args, tt );
		    strcat( args, "\n" ); // Marker!
		    break;
		  }
		}
	      }
	    }
	  }
	  
	  FreeDiskObject( icon );
	}
      }

      // Fix for ReadArgs: replace all but the last LF with a space
      length = strlen( wbargs );

      for( i = 0; i < length - 1; ++i )
      {
	if( wbargs[ i ] == '\n' )
	{
	  wbargs[ i ] = ' ';
	}
      }

      rdargs = AllocDosObjectTags( DOS_RDARGS, TAG_DONE );
	
      if( rdargs != NULL )
      {
	rdargs->RDA_Source.CS_Buffer = wbargs;
	rdargs->RDA_Source.CS_Length = length;
	rdargs->RDA_Source.CS_CurChr = 0;
      }
    }

    CurrentDir( cd );
  }

  if( amiga_icon == NULL )
  {
    BPTR cd = CurrentDir( GetProgramDir() );
    char name[ 256 ];
    
    if( GetProgramName( name, sizeof( name ) ) ) {
      amiga_icon = GetDiskObject( name );
    }

    CurrentDir( cd );
  }
  
  rdargs = ReadArgs(template, (LONG*) &a_first_arg, rdargs);

  if (rdargs == NULL)
  {
    char txt[128];
		
    Fault(IoErr(), NULL, txt, sizeof(txt));
    error("%s\n", txt);
    return RETURN_ERROR;
  }

  
  if (a_user == NULL)
  {
    error("Could not determine username.\nYou have to specify it manually.\n");
    rc = RETURN_ERROR;
  }

  if (a_client == NULL)
  {
    error("Could not determine client name.\nYou have to specify it manually.\n");
    rc = RETURN_ERROR;
  }

  if (a_server != NULL) STRNCPY(server, a_server, sizeof(server));
  if (a_user != NULL)   STRNCPY(g_username, a_user, sizeof(g_username));	
  if (a_domain != NULL) STRNCPY(domain, a_domain, sizeof(domain));
  if (a_shell != NULL)  STRNCPY(shell, a_shell, sizeof(shell));
  if (a_dir != NULL)    STRNCPY(directory, a_dir, sizeof(directory));
  if (a_password != NULL)
  {
    STRNCPY(password, a_password, sizeof(password));
    flags |= RDP_LOGON_AUTO;
  }
  if (a_client != NULL) STRNCPY(hostname, a_client, sizeof(hostname));

  if (a_pubscreen != NULL)
  {
    STRNCPY(pubscreen, a_pubscreen, sizeof(pubscreen));
    amiga_pubscreen_name = pubscreen;
  }

  if (a_screenmode != NULL)
  {
    amiga_screen_id = *a_screenmode;
    a_fullscreen = TRUE;
  }

  g_fullscreen      = a_fullscreen;

  if (a_left != NULL) amiga_left = *a_left;
  if (a_top != NULL)  amiga_top  = *a_top;

  if (a_fullscreen && (a_left != NULL || a_top != NULL || a_pubscreen != NULL))
  {
    warning( "The LEFT, TOP and PUBSCREEN arguments are ignored in fullscreen mode.\n");
  }
	
  g_width           = *a_width;
  g_height          = *a_height;
	
  g_orders          = ! a_bitmapsonly;
  g_encryption      = ! a_french;
  packet_encryption = ! a_noenc;
  g_sendmotion      = ! a_nomouse;
#ifdef WITH_RDPSND
  if( a_audio != NULL) amiga_audio_unit = *a_audio;
#endif
  if (a_remoteaudio)
  {
    flags |= RDP_LOGON_LEAVE_AUDIO;

    if (a_audio != NULL)
    {
      warning( "The AUDIO argument is ignored when REMOTEAUDIO is used.\n");
    }
  }
#ifdef WITH_RDPSND
  else
  {
    g_rdpsnd = True;
  }
#endif
	
  if( a_title != NULL) STRNCPY(g_title, a_title, sizeof(g_title));

  g_server_bpp = *a_depth;

  if (g_server_bpp < 0 ||
      (g_server_bpp > 8 && g_server_bpp != 16 && g_server_bpp != 15
       && g_server_bpp != 24))
  {
    error("invalid server bpp\n");
    rc = RETURN_ERROR;
  }

  if (a_experience != NULL)
  {
    toupper_str(a_experience);
		
    if (strcmp("28K8", a_experience) == 0)
    {
      g_rdp5_performanceflags =
	RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
	RDP5_NO_MENUANIMATIONS | RDP5_NO_THEMING;
    }
    else if (strcmp("56K", a_experience) == 0)
    {
      g_rdp5_performanceflags =
	RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
	RDP5_NO_MENUANIMATIONS;
    }
    else if (strcmp("DSL", a_experience) == 0)
    {
      g_rdp5_performanceflags = RDP5_NO_WALLPAPER;
    }
    else if (strcmp("LAN", a_experience) == 0)
    {
      g_rdp5_performanceflags = RDP5_DISABLE_NOTHING;
    }
    else
    {
      error("Illegal EXPERIENCE value: %s\n", a_experience);
      rc = RETURN_ERROR;
    }
  }
		
  g_console_session = a_console;
  g_use_rdp5 = ! a_rdp4;

  if (server[0] == 0)
  {
    error("No server specified\n");
    rc = RETURN_ERROR;
  }

  parse_server_and_port(server);

  if (strcmp(password, "-") == 0)
  {
    read_password(password, sizeof(password));
  }

  if (g_title[0] == 0)
  {
    strcpy(g_title, "RDesktop - ");
    strncat(g_title, server, sizeof(g_title) - sizeof("rdesktop - "));
  }

  FreeArgs( rdargs );
  free( wbargs );

#define PRINTS( s ) //printf( "%s: %s\n", #s, s );
#define PRINTI( i ) //printf( "%s: %d\n", #i, i );
	
  PRINTI( amiga_left );
  PRINTI( amiga_top );
  PRINTI( amiga_screen_id );
  PRINTS( amiga_pubscreen_name );
 
  PRINTS(  g_title );
  PRINTS(  g_username );
  PRINTS(  hostname );
  PRINTS(  keymapname );
  PRINTI(  keylayout );
  PRINTI(  g_width );
  PRINTI(  g_height );
  PRINTI(  tcp_port_rdp );
  PRINTI(  g_server_bpp );
  PRINTI(  g_win_button_size );
  PRINTI(  g_bitmap_compression );
  PRINTI(  g_sendmotion );
  PRINTI(  g_orders );
  PRINTI(  g_encryption );
  PRINTI(  packet_encryption );
  PRINTI(  g_desktop_save );
  PRINTI(  g_fullscreen );
  PRINTI(  g_grab_keyboard );
  PRINTI(  g_hide_decorations );
  PRINTI(  g_use_rdp5 );
  PRINTI(  g_console_session );
  PRINTI(  g_numlock_sync );
  PRINTI(  g_owncolmap );
  PRINTI(  g_ownbackstore );
  PRINTI(  g_embed_wnd );
  PRINTI(  g_rdp5_performanceflags );
	
  if (rc != RETURN_OK)
  {
    return rc;
  }
	
  if (!ui_init())
    return RETURN_FAIL;

  startup = UI_INIT;
	
#ifdef WITH_RDPSND
  if (g_rdpsnd)
  {
    rdpsnd_init();
    startup = RDPSND_INIT;
  }
#endif
  rdpdr_init();
  startup = RDPDR_INIT;

  if (!rdp_connect(server, flags, domain, password, shell, directory))
    return RETURN_FAIL;

  startup = RDP_CONNECT;
	
  /* By setting encryption to False here, we have an encrypted login 
     packet but unencrypted transfer of other packets */
  if (!packet_encryption)
    g_encryption = False;

  DEBUG(("Connection successful.\n"));
  memset(password, 0, sizeof(password));

  cache_create();
  if (ui_create_window())
  {
    startup = UI_CREATE_WINDOW;
    rdp_retval = rdp_main_loop();
  }
	
  if (True == rdp_retval)
    return RETURN_OK;
  else
    return RETURN_WARN;
}

void
amiga_req(char* prefix, char* txt);

static void
showmsg(char* prefix, char* format, va_list ap)
{
  char txt[256];

  vsnprintf(txt, sizeof(txt), format, ap);
  va_end(ap);

  if( wb_msg == NULL || IntuitionBase == NULL )
  {
    fprintf(stderr, "%s: %s", prefix, txt);
  }
  else
  {
    int lf = strlen( txt ) - 1;

    if( lf >= 0 &&  txt[ lf ] == '\n' )
    {
      txt[ lf ] = '\0';
    }
    
    amiga_req( prefix, txt );
  }
}

/* report an error */
void
error(char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  showmsg( "ERROR", format, ap );
  va_end(ap);
}

/* report a warning */
void
warning(char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  showmsg( "WARNING", format, ap );
  va_end(ap);
}

/* report an unimplemented protocol feature */
void
unimpl(char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  showmsg( "NOT IMPLEMENTED", format, ap );
  va_end(ap);
}



#ifdef __ixemul__

/* This function is missing from ixemul.library */

#include <stdio.h>
#include <stdarg.h>

void __eprintf(const char *format,...) /* for asserts */
{
  va_list args;
  va_start(args,format);
  vfprintf(stderr,format,args);
  va_end(args);
  abort();
}

#endif
