/*
  rdesktop: A Remote Desktop Protocol client.
  Amiga front-end etc
  Copyright (C) Martin Blom 2004-2005

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
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/wb.h>

#ifdef __amigaos4__
# include <proto/bsdsocket.h>
# include <proto/usergroup.h>
# include <proto/amissl.h>
#endif

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

#ifndef ForeachNode
#define ForeachNode(list,node) for (node = ((struct List*) list)->lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
#endif

const char version[] = "$VER: RDesktop 1.3.1cvs-"
#ifdef __MORPHOS__
                              "MorphOS"
#else
                              "AmigaOS"
#endif
                              " (13.01.2005)"
                              "©2001-2005 Martin Blom; "
                              "©1999-2004 Matthew Chapman et al.";

#ifdef __amigaos4__
struct LayersIFace    *ILayers        = NULL;
struct CyberGfxIFace  *ICyberGfx      = NULL;
struct SocketIFace    *ISocket        = NULL;
struct UserGroupIFace *IUserGroup     = NULL;
struct AmiSSLIFace    *IAmiSSL        = NULL;
uint32 AmiSSL_initialized = FALSE;
#else
struct Library*        AslBase        = NULL;
struct GfxBase*        GfxBase        = NULL;
struct Library*        IFFParseBase   = NULL;
struct Library*        IconBase       = NULL;
struct Library*        KeymapBase     = NULL;
struct KeyMapResource* KeymapResource = NULL;
struct IntuitionBase*  IntuitionBase  = NULL;
struct UtilityBase*    UtilityBase    = NULL;
struct Library*        WorkbenchBase  = NULL;
#endif
struct Library*        CyberGfxBase   = NULL;
struct Library*        LayersBase     = NULL;

static const char template[] =
"SERVER/A,USER/K,DOMAIN/K,PASSWORD/K,CLIENT/K,CONSOLE/S,FRENCH/S,"
"NOENC=NOENCRYPTION/S,SHELL/K,DIR=DIRECTORY/K,LEFT/K/N,TOP/K/N,"
"W=WIDTH/K/N,H=HEIGHT/K/N,D=DEPTH/K/N,PUBSCREEN/K,TITLE/K,"
"FS=FULLSCREEN/S,SCREENMODE/K/N,AUDIO/K/N,REMOTEAUDIO/S,"
"BITMAPSONLY/S,NOMOUSE/S,EXP=EXPERIENCE/K,RDP4/S,KEYMAP/K/N";

static const char wbtemplate[] =
"SERVER/A/K,USER/K,DOMAIN/K,PASSWORD/K,CLIENT/K,CONSOLE/S,FRENCH/S,"
"NOENC=NOENCRYPTION/S,SHELL/K,DIR=DIRECTORY/K,LEFT/K/N,TOP/K/N,"
"W=WIDTH/K/N,H=HEIGHT/K/N,D=DEPTH/K/N,PUBSCREEN/K,TITLE/K,"
"FS=FULLSCREEN/S,SCREENMODE/K/N,AUDIO/K/N,REMOTEAUDIO/S,"
"BITMAPSONLY/S,NOMOUSE/S,EXP=EXPERIENCE/K,RDP4/S,KEYMAP/K/N,IGNORE/F";

static LONG  def_size  = 0;
static LONG  def_depth = 0;

static struct
{
    STRPTR a_server;
    STRPTR a_user;
    STRPTR a_domain;
    STRPTR a_password;
    STRPTR a_client;
    ULONG  a_console;
    ULONG  a_french;
    ULONG  a_noenc;
    STRPTR a_shell;
    STRPTR a_dir;
    ULONG* a_left;
    ULONG* a_top;
    LONG*  a_width;
    LONG*  a_height;
    ULONG* a_depth;
    STRPTR a_pubscreen;
    STRPTR a_title;
    ULONG  a_fullscreen;
    ULONG* a_screenmode;
    ULONG* a_audio;
    ULONG  a_remoteaudio;
    ULONG  a_bitmapsonly;
    ULONG  a_nomouse;
    STRPTR a_experience;
    ULONG  a_rdp4;
    ULONG* a_keymap;
    STRPTR a_ignore;
} a_args =
{
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   FALSE,
   FALSE,
   FALSE,
   NULL,
   NULL,
   NULL,
   NULL,
   &def_size,
   &def_size,
   &def_depth,
   NULL,
   NULL,
   FALSE,
   NULL,
   NULL,
   FALSE,
   FALSE,
   FALSE,
   "56K",
   FALSE,
   &def_depth,
   NULL
};

static struct WBStartup*  wb_msg = NULL;


// Windows keymap codes:
// rdesktop-*/doc/keymap-names.txt
// http://www.science.co.il/Language/Locale-Codes.asp
//
// Amiga keymap codes:
// No idea. Mail me if the following table is incorrect!

static struct {
    char const* name;
    int         win_code;
} keymap_codes[] = {
  { "1251Q_US_RUS",	0x00419 },		// ??? Russian
  { "1251_GB1_RUS",	0x00419 },		// ??? Russian
  { "1251_GB_RUS",	0x00419 },		// ??? Russian
  { "a",		0x00409 },		// ??? 
  { "be",		0x0080c },		// ??? French (Belgium)
  { "br",		0x00416 },		// ??? Portuguese (Brazil)
  { "br2",		0x00416 },		// ??? Portuguese (Brazil)
  { "br3-ABNT2",	0x00416 },		// ??? Portuguese (Brazil)
  { "cat",		0x00403 },		// ??? Catalan
  { "cdn",		0x00c0c },		// ??? French Canadian
  { "cdn2",		0x00c0c },		// ???
  { "ch1",		0x0100c },		// Swiss French
  { "ch2",		0x00807 },		// Swiss German
  { "d",		0x00407 },		// German (Standard)
  { "d_pc",		0x00407 },		// German (Standard)
  { "dk",		0x00406 },		// Danish
  { "e",		0x00c0a },		// ??? Spanish (Modern)
  { "f",		0x0040c },		// French
  { "gb",		0x00809 },		// British
  { "gr",		0x00408 },		// ??? Greek
  { "i",		0x00410 },		// Italian
  { "n",		0x00414 },		// Norwegian
  { "oe",		0x00409 },		// ???
  { "po",		0x00816 },		// Portuguese (Portugal)
  { "Russian",		0x00419 },		// ??? Russian
  { "s",		0x0041d },		// Swedish
  { "si",		0x00409 },		// ???
  { "su",		0x0040b },		// Finnish
  { "türkçe",		0x0041f },		// ??? Turkish (Q type)
  { "usa",		0x00409 },		// United States 101
  { "usa0",		0x00409 },		// United States 101
  { "usa1",		0x00409 },		// United States 101
  { "usa2",		0x20409 },		// United States-Dvorak
  { "usa3",		0x00409 },		// United States 101
  { NULL, 		0       }
};

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

enum { UI_INIT          = 1,
       RDPSND_INIT      = 2,
       RDPDR_INIT       = 4,
       RDP_CONNECT      = 8,
       UI_CREATE_WINDOW = 16};

static int startup = 0;

static void
cleanup(void)
{
  if( amiga_icon != NULL )
  {
    FreeDiskObject( amiga_icon );
  }

  if (startup & RDPSND_INIT) {
#ifdef WITH_RDPSND
      rdpsnd_deinit();
#endif
  }

  if (startup & UI_CREATE_WINDOW) {
    cache_destroy();
    ui_destroy_window();
  }

  if (startup & RDP_CONNECT) {  
    DEBUG(("Disconnecting...\n"));
    rdp_disconnect();
  }

  if (startup & UI_INIT) {
    ui_deinit();
  }
  
#ifdef __amigaos4__
  if (IAmiSSL)
  {
    struct Library *LibBase = ((struct Interface *)IAmiSSL)->Data.LibBase;

    if (AmiSSL_initialized)
    {
       CleanupAmiSSL(TAG_DONE);
    }
    DropInterface((struct Interface *)IAmiSSL);
    IAmiSSL = NULL;
    CloseLibrary(LibBase);
  }
  if (ISocket)
  {
    struct Library *LibBase = ((struct Interface *)ISocket)->Data.LibBase;

    DropInterface((struct Interface *)ISocket);
    ISocket = NULL;
    CloseLibrary(LibBase);
  }
  if (IUserGroup)
  {
    struct Library *LibBase = ((struct Interface *)IUserGroup)->Data.LibBase;

    DropInterface((struct Interface *)IUserGroup);
    IUserGroup = NULL;
    CloseLibrary(LibBase);
  }

  if (ICyberGfx)
  {
     DropInterface((struct Interface *)ICyberGfx);
     ICyberGfx = NULL;
  }
#endif
  CloseLibrary( CyberGfxBase );
  CyberGfxBase = NULL;

#ifndef __amigaos4__
  CloseLibrary( AslBase );
  AslBase = NULL;
  
  CloseLibrary( (struct Library*) GfxBase );
  GfxBase = NULL;
  
  CloseLibrary( (struct Library*) IntuitionBase );
  IntuitionBase = NULL;

  CloseLibrary( IFFParseBase );
  IFFParseBase = NULL;

  CloseLibrary( IconBase );
  IconBase = NULL;

  CloseLibrary( KeymapBase );
  KeymapBase = NULL;
#endif
  
#ifdef __amigaos4__
  DropInterface((struct Interface *)ILayers);
#endif
  CloseLibrary( LayersBase );
  LayersBase = NULL;

#ifndef __amigaos4__
  CloseLibrary( (struct Library*) UtilityBase );
  UtilityBase = NULL;

  CloseLibrary( WorkbenchBase );
  WorkbenchBase = NULL;
#endif
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
  struct Node* node;
  struct KeyMap* default_keymap;
  

  char pubscreen[64];
  struct RDArgs* rdargs = NULL;
  char* wbargs = NULL;
  int rc = RETURN_OK;

  atexit(cleanup);

#ifndef __amigaos4__
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

  KeymapBase = OpenLibrary("keymap.library", 36);

  if( KeymapBase == NULL )
  {
    error( "Unable to open '%s'.\n", "keymap.library" );
  }

  KeymapResource = OpenResource("keymap.resource"); 

  if( KeymapResource == NULL )
  {
    error( "Unable to open '%s'.\n", "keymap.resource" );
  }
 

  IconBase = OpenLibrary("icon.library", 0);

  if( IconBase == NULL )
  {
    error( "Unable to open '%s'.\n", "icon.library" );
  }
      
  IFFParseBase = OpenLibrary("iffparse.library", 39);

  if( IFFParseBase == NULL )
  {
    error( "Unable to open '%s'.\n", "iffparse.library" );
  }

  IntuitionBase = (struct IntuitionBase*) OpenLibrary( "intuition.library", 39 );
  
  if( IntuitionBase == NULL )
  {
    error( "Unable to open '%s'.\n", "intuition.library" );
    return RETURN_FAIL;
  }
#endif
#ifdef __amigaos4__
{
  struct Library *LibBase;

  LibBase = OpenLibrary("bsdsocket.library", 3L);
  if (NULL != LibBase)
  {
     ISocket = (struct SocketIFace *)GetInterface(LibBase, "main", 1, NULL);
     if (!ISocket) CloseLibrary(LibBase);
  }
  if (!LibBase || !ISocket)
  {
     error( "Unable to open '%s'.\n", "bsdsocket.library" );
     return RETURN_FAIL;
  }

  LibBase = OpenLibrary("usergroup.library", 0L);
  if (LibBase)
  {
     IUserGroup = (struct UserGroupIFace *)GetInterface(LibBase, "main", 1, NULL);
     if (!IUserGroup) CloseLibrary(LibBase);
  }
  if (!LibBase || !IUserGroup)
  {
     error( "Unable to open '%s'.\n", "usergroup.library" );
     return RETURN_FAIL;
  }

  LibBase = OpenLibrary("amissl.library", 1);
  if (LibBase)
  {
     IAmiSSL = (struct AmiSSLIFace *)GetInterface(LibBase, "main", 1, NULL);
     if (!IAmiSSL) CloseLibrary(LibBase);
  }
  if (!LibBase || !IAmiSSL)
  {
     error( "Unable to open '%s'.\n", "amissl.library" );
     return RETURN_FAIL;
  }

  if (!InitAmiSSL(AmiSSL_Version, AmiSSL_CurrentVersion,
                  AmiSSL_Revision, AmiSSL_CurrentRevision,
                  /* AmiSSL_VersionOverride, TRUE,*/
                  AmiSSL_SocketBase, ((struct Interface *)ISocket)->Data.LibBase,
                  TAG_DONE))
  {
     AmiSSL_initialized = TRUE;
  } else {
     error( "Unable to initialize AmiSSL\n" );
     return RETURN_FAIL;
  }

  SetErrnoPtr(&errno, 4);
}
#endif

  LayersBase = OpenLibrary( "layers.library", 39 );
#ifdef __amigaos4__
  ILayers = (struct LayersIFace *)GetInterface(LayersBase, "main", 1, NULL);
  if (!ILayers)
  {
     CloseLibrary(LayersBase);
     LayersBase = NULL;
  }
#endif
  
  if( LayersBase == NULL )
  {
    error( "Unable to open '%s'.\n", "layers.library" );
    return RETURN_FAIL;
  }


#ifndef __amigaos4__
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
#endif
  
  CyberGfxBase = OpenLibrary( "cybergraphics.library", 40 );
#ifdef __amigaos4__
  if (CyberGfxBase)
  {
    ICyberGfx = (struct CyberGfxIFace *)GetInterface(CyberGfxBase, "main", 1, NULL);
    if (!ICyberGfx)
    {
      CloseLibrary(CyberGfxBase);
      CyberGfxBase = NULL;
    }
  }
#endif

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
    a_args.a_user = pw->pw_name;
  }

  if (gethostname(fullhostname, sizeof(fullhostname)) != -1)
  {
    p = strchr(fullhostname, '.');
    if (p != NULL)
      *p = 0;

    a_args.a_client = fullhostname;
  }

  Forbid();
  // According to RKRM, it's ok to check this against the keymap list
  default_keymap = AskKeyMapDefault();

  ForeachNode (&KeymapResource->kr_List, node) {
    struct KeyMapNode* keymapnode = (struct KeyMapNode*) node;

    if (default_keymap == &keymapnode->kn_KeyMap) {
      int i;

      for (i = 0; keymap_codes[i].name != NULL; ++i) {
	if (Stricmp(keymapnode->kn_Node.ln_Name, keymap_codes[i].name) == 0) {
	  keylayout = keymap_codes[i].win_code;
	}
      }
    }
  }
  Permit();
  
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

  if (0 == argc)
  {
     rdargs = ReadArgs(wbtemplate, (LONG*) &a_args, rdargs);
  } else {
     rdargs = ReadArgs(template, (LONG*)&a_args, rdargs);
  }

  if (rdargs == NULL)
  {
    char txt[128];
		
    Fault(IoErr(), NULL, txt, sizeof(txt));
    error("%s\n", txt);
    return RETURN_ERROR;
  }

  
  if (a_args.a_user == NULL)
  {
    error("Could not determine username.\nYou have to specify it manually.\n");
    rc = RETURN_ERROR;
  }

  if (a_args.a_client == NULL)
  {
    error("Could not determine client name.\nYou have to specify it manually.\n");
    rc = RETURN_ERROR;
  }

  if (a_args.a_server != NULL) STRNCPY(server, a_args.a_server, sizeof(server));
  if (a_args.a_user != NULL)   STRNCPY(g_username, a_args.a_user, sizeof(g_username));	
  if (a_args.a_domain != NULL) STRNCPY(domain, a_args.a_domain, sizeof(domain));
  if (a_args.a_shell != NULL)  STRNCPY(shell, a_args.a_shell, sizeof(shell));
  if (a_args.a_dir != NULL)    STRNCPY(directory, a_args.a_dir, sizeof(directory));
  if (a_args.a_password != NULL)
  {
    STRNCPY(password, a_args.a_password, sizeof(password));
    flags |= RDP_LOGON_AUTO;
  }
  if (a_args.a_client != NULL) STRNCPY(hostname, a_args.a_client, sizeof(hostname));

  if (a_args.a_pubscreen != NULL)
  {
    STRNCPY(pubscreen, a_args.a_pubscreen, sizeof(pubscreen));
    amiga_pubscreen_name = pubscreen;
  }

  if (a_args.a_screenmode != NULL)
  {
    amiga_screen_id = *a_args.a_screenmode;
    a_args.a_fullscreen = TRUE;
  }

  g_fullscreen      = a_args.a_fullscreen;

  if (a_args.a_left != NULL) amiga_left = *a_args.a_left;
  if (a_args.a_top != NULL)  amiga_top  = *a_args.a_top;

  if (a_args.a_fullscreen && (a_args.a_left != NULL || a_args.a_top != NULL || a_args.a_pubscreen != NULL))
  {
    warning( "The LEFT, TOP and PUBSCREEN arguments are ignored in fullscreen mode.\n");
  }
	
  g_width           = *a_args.a_width;
  g_height          = *a_args.a_height;
	
  g_orders          = ! a_args.a_bitmapsonly;
  g_encryption      = ! a_args.a_french;
  packet_encryption = ! a_args.a_noenc;
  g_sendmotion      = ! a_args.a_nomouse;
#ifdef WITH_RDPSND
  if( a_args.a_audio != NULL) amiga_audio_unit = *a_args.a_audio;
#endif
  if (a_args.a_remoteaudio)
  {
    flags |= RDP_LOGON_LEAVE_AUDIO;

    if (a_args.a_audio != NULL)
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
	
  if( a_args.a_title != NULL) STRNCPY(g_title, a_args.a_title, sizeof(g_title));

  g_server_bpp = *a_args.a_depth;

  if (g_server_bpp < 0 ||
      (g_server_bpp > 8 && g_server_bpp != 16 && g_server_bpp != 15
       && g_server_bpp != 24))
  {
    error("invalid server bpp\n");
    rc = RETURN_ERROR;
  }

  if (a_args.a_experience != NULL)
  {
    toupper_str(a_args.a_experience);
		
    if (strcmp("28K8", a_args.a_experience) == 0)
    {
      g_rdp5_performanceflags =
	RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
	RDP5_NO_MENUANIMATIONS | RDP5_NO_THEMING;
    }
    else if (strcmp("56K", a_args.a_experience) == 0)
    {
      g_rdp5_performanceflags =
	RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
	RDP5_NO_MENUANIMATIONS;
    }
    else if (strcmp("DSL", a_args.a_experience) == 0)
    {
      g_rdp5_performanceflags = RDP5_NO_WALLPAPER;
    }
    else if (strcmp("LAN", a_args.a_experience) == 0)
    {
      g_rdp5_performanceflags = RDP5_DISABLE_NOTHING;
    }
    else
    {
      error("Illegal EXPERIENCE value: %s\n", a_args.a_experience);
      rc = RETURN_ERROR;
    }
  }
		
  g_console_session = a_args.a_console;
  g_use_rdp5 = ! a_args.a_rdp4;

  if (a_args.a_keymap != NULL) keylayout = *a_args.a_keymap;

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

  startup |= UI_INIT;
	
#ifdef WITH_RDPSND
  if (g_rdpsnd)
  {
    rdpsnd_init();
    startup |= RDPSND_INIT;
  }
#endif
  rdpdr_init();
  startup |= RDPDR_INIT;

  if (!rdp_connect(server, flags, domain, password, shell, directory))
    return RETURN_FAIL;

  startup |= RDP_CONNECT;
	
  /* By setting encryption to False here, we have an encrypted login 
     packet but unencrypted transfer of other packets */
  if (!packet_encryption)
    g_encryption = False;

  DEBUG(("Connection successful.\n"));
  memset(password, 0, sizeof(password));

  cache_create();
  if (ui_create_window())
  {
    startup |= UI_CREATE_WINDOW;
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
