/* $Id$ */

/*
     NallePUH -- Paula utan henne -- A minimal Paula emulator.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "CompilerSpecific.h"

#include <classes/window.h>
#include <devices/ahi.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <gadgets/listbrowser.h>
#include <intuition/gadgetclass.h>
#include <libraries/resource.h>

#include <clib/alib_protos.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/listbrowser.h>
#include <proto/locale.h>
#include <proto/resource.h>
#include <proto/utility.h>

#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

#include <stdio.h>
#include <stdlib.h>

#include "NallePUH.h"
#include "PUH.h"

static struct MsgPort*      AHImp     = NULL;
static struct AHIRequest*   AHIio     = NULL;
static BYTE                 AHIDevice = IOERR_OPENFAIL;

struct Library*       AHIBase         = NULL;
struct Library*       MMUBase         = NULL;
struct IntuitionBase* IntuitionBase   = NULL;
struct LocaleBase*    LocaleBase      = NULL;
struct Library*       ResourceBase    = NULL;
struct Library*       ListBrowserBase = NULL;

extern UBYTE pooh11_sb[];
extern ULONG pooh11_sblen;

static BOOL
OpenLibs( void );

static void
CloseLibs( void );

static BOOL 
OpenAHI( void );

static void 
CloseAHI( void );

static BOOL
ShowGUI( struct PUHData* pd );

static BOOL
HandleGUI( Object* window,
           struct Gadget** gadgets,
           struct PUHData* pd );


static void
Test( void* data, ULONG length );

void __chkabort( void )
{
  // Disable automatic ctrl-c handling
}

ULONG
RefreshSetGadgetAttrsA(struct Gadget *g, struct Window *w, struct Requester *r, struct TagItem *tags)
{
	ULONG retval;
	BOOL changedisabled = FALSE;
	BOOL disabled = FALSE;

	if (w)
	{
		if (FindTagItem(GA_Disabled,tags))
		{
			changedisabled = TRUE;
 			disabled = g->Flags & GFLG_DISABLED;
 		}
 	}
	retval = SetGadgetAttrsA(g,w,r,tags);
	if (w && (retval || (changedisabled && disabled != (g->Flags & GFLG_DISABLED))))
	{
		RefreshGList(g,w,r,1);
		retval = 1;
	}
	return retval;
}

ULONG
RefreshSetGadgetAttrs(struct Gadget *g, struct Window *w, struct Requester *r, Tag tag1, ...)
{
	return RefreshSetGadgetAttrsA(g,w,r,(struct TagItem *) &tag1);
}

int
main( int   argc,
      char* argv[] )
{
  int   rc        = 0;
  BOOL  gui_mode  = FALSE;

  ULONG mode_id   = 0;
  ULONG frequency = 0;
  ULONG level     = 0;


  if( ! OpenLibs() )
  {
    return 20;
  }

  if( argc == 1 && ResourceBase != NULL )
  {
    // Gui mode
    
    gui_mode = TRUE;
  }
  else if( argc != 4 )
  {
    printf( "Usage: %s [0x]<AHI mode ID> <Frequency> <Level>\n", argv[ 0 ] );
    printf( "Level can be 0 (no patches), 1 (ROM patches) or 2 (appl. patches)\n" );
    return 10;
  }

  if( ! gui_mode )
  {
    char* mode_ptr;
    char* freq_ptr;
    char* levl_ptr;

    mode_id   = strtol( argv[ 1 ], &mode_ptr, 0 );
    frequency = strtol( argv[ 2 ], &freq_ptr, 0 );
    level     = strtol( argv[ 3 ], &levl_ptr, 0 );

    if( *mode_ptr != 0 || *freq_ptr != 0 || *levl_ptr != 0 )
    {
      printf( "All arguments must be numbers.\n" );
      return 10;
    }

    if( level > 2 )
    {
      printf( "Invalid value for Level.\n" );
      return 10;
    }
  }

  if( ! OpenAHI() )
  {
    printf( "Unable to open ahi.device version 4.\n" );
    rc = 20;
  }

  if( rc == 0 )
  {
    struct PUHData* pd;

    pd = AllocPUH();
    
    if( pd == NULL )
    {
      rc = 20;
    }
    else
    {
      if( gui_mode )
      {
        if( ! ShowGUI( pd ) )
        {
          printf( "Failed to create GUI.\n" );
          rc = 20;
        }
      }
      else
      {
        ULONG flags = 0;
        
        printf( "Using mode ID 0x%08lx, %ld Hz.\n", mode_id, frequency );
        
        switch( level )
        {
          case 0:
            printf( "No patches.\n" );

            flags = PUHF_NONE;
            break;

          case 1:
            printf( "ROM patches.\n" );

            flags = PUHF_PATCH_ROM;
            break;
            
          case 2:
            printf( "ROM and application patches.\n" );

            flags = PUHF_PATCH_ROM | PUHF_PATCH_APPS;
            break;
        }

        if( ! InstallPUH( flags,
                          mode_id, frequency,
                          pd ) )
        {
          rc = 20;
        }
        else
        {
          if( ! ActivatePUH( pd ) )
          {
            rc = 20;
          }
          else
          {
            //Test( (struct Custom*) 0xdff000 );

            printf( "Waiting for CTRL-C...\n" );
            Wait( SIGBREAKF_CTRL_C );
            printf( "Got it.\n" );
        
            DeactivatePUH( pd );
          }
        
          UninstallPUH( pd );
        }
      }

      FreePUH( pd );
    }
  }


  CloseAHI();
  CloseLibs();

  return rc;
}



#if 0

BYTE samples[] = 
{
  0, 90, 127, 90, 0, -90, -127, -90
};





/******************************************************************************
** Test ***********************************************************************
******************************************************************************/

ASMCALL SAVEDS static void
TestInt( REG( d1, UWORD           active_ints ),
         REG( a0, struct Custom*  custom ),
         REG( a1, ULONG           data ),
         REG( a5, void*           me ),
         REG( a6, struct ExecBase* SysBase ) )
{
  KPrintF( "TestInt: active_ints=%04lx, custom=%08lx, "
           "data=%08lx, me=%08lx, SysBase=%08lx\n",
           active_ints, custom, data, me, SysBase );

  // Clear interrupts
  WriteWord( &custom->intreq, INTF_AUD0 | INTF_AUD1 | INTF_AUD2 | INTF_AUD3 ); 
}


static void
Test( struct Custom* custom )
{
  void*             chip = NULL;
  struct Interrupt* old  = NULL;
  struct Interrupt  new  =
  {
    {
      NULL, NULL,
      NT_INTERRUPT,
      0,
      "Nalle PUH audio test interrupt"
    },
    0xdeadc0de,
    (void(*)(void)) TestInt
  };

  printf( "Testing with custom set to 0x%08lx\n", (ULONG) custom );

  chip = AllocVec( sizeof( samples ), MEMF_CHIP );

  CopyMem( samples, chip, sizeof( samples ) );

  old = SetIntVector( INTB_AUD0, &new );

  WriteLong( &custom->aud[ 0 ].ac_ptr, (ULONG) chip );
  WriteWord( &custom->aud[ 0 ].ac_len, sizeof( samples ) / 2 );
  WriteWord( &custom->aud[ 0 ].ac_per, 447 );
  WriteWord( &custom->aud[ 0 ].ac_vol, 64 );

  WriteWord( &custom->dmacon, DMAF_SETCLR | DMAF_AUD0 | DMAF_MASTER );

  Delay( 50 );

  // No-op
  WriteWord( &custom->aud[0].ac_dat, 0xdead );

  // Delayed
  WriteWord( &custom->dmacon, DMAF_AUD0 );
  WriteWord( &custom->aud[0].ac_dat, 0xcafe );

  // Invoke
  WriteWord( &custom->intena, INTF_SETCLR | INTF_AUD0 );

  Delay( 10 );

  // Invoke directly
  WriteWord( &custom->aud[0].ac_dat, 0xc0de );

  // Restore
  WriteWord( &custom->intena, INTF_AUD0 );

  SetIntVector( INTB_AUD0, old );

  FreeVec( chip );
}

#endif

/******************************************************************************
** OpenLibs *******************************************************************
******************************************************************************/

static BOOL
OpenLibs( void )
{
  IntuitionBase   = (struct IntuitionBase*) OpenLibrary( "intuition.library", 39 );
  LocaleBase      = (struct LocaleBase*) OpenLibrary( "locale.library", 39 );
  ResourceBase    = OpenLibrary( "resource.library", 44 );
  ListBrowserBase = OpenLibrary( "listbrowser.gadget", 44 );
  MMUBase         = OpenLibrary( "mmu.library", 41 );

  if( IntuitionBase  == NULL || 
      LocaleBase     == NULL ||
      ResourceBase   == NULL ||
      ListBrowserBase == NULL )
  {
    CloseLibrary( (struct Library*) IntuitionBase );
    CloseLibrary( (struct Library*) LocaleBase );
    CloseLibrary( ResourceBase );
    CloseLibrary( ListBrowserBase );
    
    IntuitionBase   = NULL;
    LocaleBase      = NULL;
    ResourceBase    = NULL;
    ListBrowserBase = NULL;
    
    printf( "The GUI requires AmigaOS 3.5.\n" );
  }

  if( MMUBase == NULL )
  {
    printf( "Unable to open mmu.library version 41.\n" );
    CloseLibs();
    return FALSE;
  }
  
  return TRUE;
}


/******************************************************************************
** CloseLibs ******************************************************************
******************************************************************************/

static void
CloseLibs( void )
{
  CloseLibrary( (struct Library*) IntuitionBase );
  CloseLibrary( (struct Library*) LocaleBase );
  CloseLibrary( ResourceBase );
  CloseLibrary( ListBrowserBase );

  // We must not close mmu.library if we have pached applications running!
  //  CloseLibrary( MMUBase );
}

/******************************************************************************
** OpenAHI ********************************************************************
******************************************************************************/

/* Opens and initializes the device. */

static BOOL
OpenAHI( void )
{
  BOOL rc = FALSE;

  AHImp = CreateMsgPort();

  if( AHImp != NULL )
  {
    AHIio = (struct AHIRequest *) CreateIORequest( AHImp, 
                                                   sizeof( struct AHIRequest ) );

    if( AHIio != NULL ) 
    {
      AHIio->ahir_Version = 4;
      AHIDevice = OpenDevice( AHINAME,
                              AHI_NO_UNIT,
                              (struct IORequest *) AHIio,
                              NULL );
                              
      if( AHIDevice == 0 )
      {
        AHIBase = (struct Library *) AHIio->ahir_Std.io_Device;
        rc = TRUE;
      }
    }
  }

  return rc;
}


/******************************************************************************
** CloseAHI *******************************************************************
******************************************************************************/

/* Closes the device, cleans up. */

static void
CloseAHI( void )
{
  if( AHIDevice == 0 )
  {
    CloseDevice( (struct IORequest *) AHIio );
  }

  DeleteIORequest( (struct IORequest *) AHIio );
  DeleteMsgPort( AHImp );

  AHIBase   = NULL;
  AHImp     = NULL;
  AHIio     = NULL;
  AHIDevice = IOERR_OPENFAIL;
}


/******************************************************************************
** ShowGUI ********************************************************************
******************************************************************************/

static BOOL
ShowGUI( struct PUHData* pd )
{
  BOOL            rc = FALSE;

  struct Catalog* catalog;
  struct Screen*  screen;
  struct MsgPort* idcmp_port;
  struct MsgPort* app_port;
  RESOURCEFILE    resource;
  Object*         window;
  struct Gadget** gadgets;

  catalog = OpenCatalogA( NULL, "NallePUH.catalog",NULL);
  
  screen = LockPubScreen( NULL );
  
  if( screen != NULL )
  {
    idcmp_port = CreateMsgPort();
    
    if( idcmp_port != NULL )
    {
      app_port = CreateMsgPort();
      
      if( app_port != NULL )
      {
        resource = RL_OpenResource( RCTResource, screen, catalog );
        
        if( resource != NULL )
        {
          window = RL_NewObject( resource, WIN_1_ID,
                                 WINDOW_SharedPort, (ULONG) idcmp_port,
                                 WINDOW_AppPort,    (ULONG) app_port,
                                 TAG_DONE );
                                 
          if( window != NULL )
          {
            gadgets = (struct Gadget **) RL_GetObjectArray( resource, 
                                                            window, 
                                                            GROUP_2_ID );
            if( gadgets != NULL )
            {
              DoMethod( window, WM_OPEN );
              
              rc = HandleGUI( window, gadgets, pd );
              
              DoMethod( window, WM_CLOSE);
            }
          }
          RL_CloseResource( resource );
        }
        
        DeleteMsgPort( app_port );
      }
      
      DeleteMsgPort( idcmp_port );
    }

    UnlockPubScreen( NULL, screen );
  }
  
  CloseCatalog( catalog );

  return rc;
}


/******************************************************************************
** HandleGUI ********************************************************************
******************************************************************************/

ASMCALL SAVEDS static ULONG
FilterFunc( REG( a0, struct Hook*                  hook ),
            REG( a1, ULONG                         mode_id ),
            REG( a2, struct AHIAudioModeRequester* req ) )
{
  // Remove all Paula modes (hardcoded mode IDs suck.)

  if( ( mode_id & 0xffff0000  ) == 0x00020000 )
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}


static struct Hook FilterHook =
{
  { NULL, NULL },
  (HOOKFUNC) FilterFunc,
  NULL,
  NULL
};


static BOOL
HandleGUI( Object*         window,
           struct Gadget** gadgets,
           struct PUHData* pd )
{
  BOOL           rc             = FALSE;
  BOOL           quit           = FALSE;

  struct Window* win_ptr        = NULL;
  ULONG          window_signals = 0;

  ULONG          audio_mode     = 0;
  ULONG          frequency      = 0;

  void*          chip           = NULL;
  
  struct Custom* custom         = 0xdff000;

  chip = AllocVec( pooh11_sblen, MEMF_CHIP );

  if( chip == NULL )
  {
    printf( "Failed to alloc chip memory.\n" );
    return FALSE;
  }

  CopyMem( pooh11_sb, chip, pooh11_sblen );


  GetAttr( WINDOW_SigMask, window, &window_signals );
  GetAttr( WINDOW_Window,  window, (ULONG *) &win_ptr );

#if 0
                struct Node* node;
                
                DoGadgetMethod( gadgets[ GAD_MESSAGES ], win_ptr, NULL,
                                LBM_ADDNODE,
                                NULL,
                                LBNA_Column, 0,
                                LBNCA_Text, "JAG ÄR EN APA!",
                                LBNCA_CopyText, TRUE,
                                TAG_DONE );

                node = AllocListBrowserNode( 1,
                                             LBNCA_Text, "JAG ÄR EN APA!",
                                             LBNCA_CopyText, TRUE,
                                             TAG_DONE );
                                             
#endif


  while( ! quit )
  {
    ULONG mask;
    
    mask = Wait( window_signals | SIGBREAKF_CTRL_C );
    
    if( mask & SIGBREAKF_CTRL_C )
    {
      quit = TRUE;
      rc   = TRUE;
      break;
    }

    if( mask & window_signals )
    {
      ULONG input_flags = 0;
      ULONG code        = 0;
      
      while( ( input_flags = DoMethod( window, WM_HANDLEINPUT, &code ) ) 
             != WMHI_LASTMSG )
      {
        switch( input_flags & WMHI_CLASSMASK)
        {
          case WMHI_CLOSEWINDOW:
            quit = TRUE;
            rc   = TRUE;
            break;

          case WMHI_ICONIFY:
            DoMethod( window, WM_ICONIFY );
            GetAttr( WINDOW_Window,  window, (ULONG *) &win_ptr );
            break;
            
          case WMHI_UNICONIFY:
            DoMethod( window, WM_OPEN );
            GetAttr( WINDOW_Window,  window, (ULONG *) &win_ptr );
            break;

          case WMHI_GADGETUP:
          {
            switch( input_flags & RL_GADGETMASK )
            {
              case GAD_MODE_SELECT:
              {
                struct AHIAudioModeRequester* req = NULL;
                
                struct TagItem                filter_tags[] =
                {
                  { AHIDB_Realtime,    TRUE },
                  { AHIDB_MaxChannels, 4    },
                  { TAG_DONE,          0    }
                };


                req = AHI_AllocAudioRequest( 
                    AHIR_Window,         (ULONG) win_ptr,
                    AHIR_SleepWindow,    TRUE,
                    AHIR_InitialAudioID, audio_mode,
                    AHIR_InitialMixFreq, frequency,
                    AHIR_DoMixFreq,      TRUE,
                    AHIR_DoDefaultMode,  TRUE,
                    AHIR_FilterFunc,     (ULONG) &FilterHook,
                    AHIR_FilterTags,     (ULONG) filter_tags,
                    TAG_DONE );

                if( req == NULL )
                {
                  quit = TRUE;
                  rc   = FALSE;
                }
                else
                {
                  if( AHI_AudioRequest( req, TAG_DONE ) )
                  {
                    char buffer[ 256 ];

                    audio_mode = req->ahiam_AudioID;
                    frequency  = req->ahiam_MixFreq;
                    
                    if( AHI_GetAudioAttrs( audio_mode, NULL,
                                           AHIDB_BufferLen, 255,
                                           AHIDB_Name,      (ULONG) buffer,
                                           TAG_DONE ) )
                    {
                      RefreshSetGadgetAttrs( gadgets[ GAD_MODE_INFO ], win_ptr, NULL,
                                      STRINGA_TextVal, (ULONG) buffer,
                                      TAG_DONE );

                      RefreshSetGadgetAttrs( gadgets[ GAD_INSTALL ], win_ptr, NULL,
                                      GA_Disabled, FALSE,
                                      TAG_DONE );
                    }
                  }

                  AHI_FreeAudioRequest( req );
                }

                break;
              }

              case GAD_INSTALL:
              {
                ULONG flags      = 0;
                ULONG patch_rom  = 0;
                ULONG patch_apps = 0;
                ULONG toggle_led = 0;

                GetAttr( GA_Selected, gadgets[ GAD_PATCH_ROM  ], &patch_rom );
                GetAttr( GA_Selected, gadgets[ GAD_PATCH_APPS ], &patch_apps );
                GetAttr( GA_Selected, gadgets[ GAD_TOGGLE_LED ], &toggle_led );

                if( patch_rom )
                {
                  flags |= PUHF_PATCH_ROM;
                }

                if( patch_apps )
                {
                  flags |= PUHF_PATCH_APPS;
                }

                if( toggle_led )
                {
                  flags |= PUHF_TOGGLE_LED;
                }
                
                if( ! InstallPUH( flags,
                          audio_mode, frequency,
                          pd ) )
                {
                  printf( "Unable to install PUH.\n" );
                }
                else
                {
                  RefreshSetGadgetAttrs( gadgets[ GAD_PATCH_ROM ], win_ptr, NULL,
                                  GA_Disabled, TRUE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_PATCH_APPS ], win_ptr, NULL,
                                  GA_Disabled, TRUE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_TOGGLE_LED ], win_ptr, NULL,
                                  GA_Disabled, TRUE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_MODE_SELECT ], win_ptr, NULL,
                                  GA_Disabled, TRUE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_INSTALL ], win_ptr, NULL,
                                  GA_Disabled, TRUE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_UNINSTALL ], win_ptr, NULL,
                                  GA_Disabled, FALSE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_ACTIVATE ], win_ptr, NULL,
                                  GA_Disabled, FALSE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_DEACTIVATE ], win_ptr, NULL,
                                  GA_Disabled, TRUE,
                                  TAG_DONE );
                }

                break;
              }


              case GAD_UNINSTALL:
              {
                UninstallPUH( pd );
                
                RefreshSetGadgetAttrs( gadgets[ GAD_PATCH_ROM ], win_ptr, NULL,
                                GA_Disabled, FALSE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_PATCH_APPS ], win_ptr, NULL,
                                GA_Disabled, FALSE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_TOGGLE_LED ], win_ptr, NULL,
                                GA_Disabled, FALSE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_MODE_SELECT ], win_ptr, NULL,
                                GA_Disabled, FALSE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_INSTALL ], win_ptr, NULL,
                                GA_Disabled, FALSE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_UNINSTALL ], win_ptr, NULL,
                                GA_Disabled, TRUE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_ACTIVATE ], win_ptr, NULL,
                                GA_Disabled, TRUE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_DEACTIVATE ], win_ptr, NULL,
                                GA_Disabled, TRUE,
                                TAG_DONE );
                break;
              }
              

              case GAD_ACTIVATE:
              {
                if( ! ActivatePUH( pd ) )
                {
                  printf( "Unable to activate PUH\n" );
                }
                else
                {
                  RefreshSetGadgetAttrs( gadgets[ GAD_ACTIVATE ], win_ptr, NULL,
                                  GA_Disabled, TRUE,
                                  TAG_DONE );

                  RefreshSetGadgetAttrs( gadgets[ GAD_DEACTIVATE ], win_ptr, NULL,
                                  GA_Disabled, FALSE,
                                  TAG_DONE );
                }

                break;
              }


              case GAD_DEACTIVATE:
              {
                DeactivatePUH( pd );

                RefreshSetGadgetAttrs( gadgets[ GAD_ACTIVATE ], win_ptr, NULL,
                                GA_Disabled, FALSE,
                                TAG_DONE );

                RefreshSetGadgetAttrs( gadgets[ GAD_DEACTIVATE ], win_ptr, NULL,
                                GA_Disabled, TRUE,
                                TAG_DONE );

                break;
              }
              
              case GAD_TEST:
              {
                WriteWord( &custom->dmacon, DMAF_AUD0 );

                Delay( 1 );  // The infamous DMA-wait! ;-)

                WriteLong( &custom->aud[ 0 ].ac_ptr, (ULONG) chip );
                WriteWord( &custom->aud[ 0 ].ac_len, pooh11_sblen / 2);
                WriteWord( &custom->aud[ 0 ].ac_per, 161 );
                WriteWord( &custom->aud[ 0 ].ac_vol, 64 );

                WriteWord( &custom->dmacon, DMAF_SETCLR | DMAF_AUD0 );

                WriteWord( &custom->aud[ 0 ].ac_len, 1 );
                break;
              }
            }
            
            break;
          }

          default:
            break;
        }
      }
    }
  }
  
  WriteWord( &custom->dmacon, DMAF_AUD0 );
  WriteWord( &custom->aud[ 0 ].ac_vol, 0 );

  FreeVec( chip );

  return rc;
}
