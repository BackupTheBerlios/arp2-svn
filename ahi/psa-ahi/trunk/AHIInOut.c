/* $Id$ */

/*** Include files ************************************************************/

#include <exec/errors.h>
#include <exec/types.h>
#include <exec/ports.h>
#include <devices/ahi.h>
#include <dos/dos.h>

#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include "CompilerSpecific.h"
#include "DriverTypes.h"

/*** Definitions *************************************************************/

#define PORTNAME        "AHIInOut"

struct Context
{
  struct Task*              PSATask;
  ULONG                     PSASignalMask;
  struct AHIEffMasterVolume MasterVol;
  ULONG                     MasterVolRC;
  struct Hook               SoundHook;
  struct AHIAudioCtrl*      AudioCtrl;
  BOOL                      Playing;
  BOOL                      Recording;
  BOOL                      Sound0Loaded;
  BOOL                      Sound1Loaded;
  UWORD                     CurrentSound;
  UWORD                     NextSound;
};


/*** Prototypes **************************************************************/

void kprintf( char* fmt, ... );

static BOOL Init( void );
static void ShutDown( void );

static BOOL OpenAHI( void );
static void CloseAHI( void );

static BOOL Construct( struct Context* this );
static void Destruct( struct Context* this );
static BOOL AllocAudio( ULONG mode, ULONG freq, struct Context* this );
static void FreeAudio( struct Context* this );

static HOOKCALL ULONG
SoundFunc( REG( a0, struct Hook *hook ),
           REG( a2, struct AHIAudioCtrl *actrl ),
           REG( a1, struct AHISoundMessage *smsg ) );


/*** Global variables ********************************************************/

static struct MsgPort*      AHImp     = NULL;
static struct AHIRequest*   AHIio     = NULL;
static BYTE                 AHIDevice = IOERR_OPENFAIL;

struct Library* AHIBase = NULL;


/******************************************************************************
** main ***********************************************************************
******************************************************************************/

int
main( void )
{
  ExternalIOMessage *pEIOsg;

  struct MsgPort    *port;
  ULONG         ulSignal,ulPortSignal;

  BYTE        *pbBuff1,*pbBuff2;

  WORD		*pwRecBuff1 = NULL ,*pwRecBuff2 = NULL;
  WORD        *pwRecTmp1,*pwRecTmp2;
  WORD 		wDummySmp = 0;
  BOOL 		fRecording;

  BYTE 		bMode = -1;

  LONG        lSize = 0;
  LONG        lCurBuff = 0;
  ULONG         ulClock;
  WORD        wChannels;
  WORD        wDataType;
  struct Task     *ptaskAL16 = NULL;
  ULONG         ulSigMaskAL16 = 0;

  LONG lTmp;

  struct Context context;
  
  /* Open 'ahi.device'. */

  if( !Init() )
  {
    return RETURN_FAIL;
  }


  /* Init the hardware. If there is any problem quit without setting up
     the message port */

  if( !Construct( &context ) )
  {
    ShutDown();
    return RETURN_FAIL;
  }

  /* Set up the message port */

  port = CreatePort( PORTNAME, 0 );

  if( port != NULL )
  {
    BOOL fQuit      = FALSE;
    BOOL fPlaying   = FALSE;
    BOOL fRecording = FALSE;

    ulPortSignal = 1 << port->mp_SigBit;

    while (!fQuit)
    {
      /* Wait for messages from AL16 (commands) and for signals from
         your board (the board must signal your driver each time a
         buffer has been completed).
         In this demo the timer.device signals us every DELAY seconds */

kprintf( "Waiting for signals: %08lx.\n", ulPortSignal | SIGBREAKF_CTRL_C );

      ulSignal = Wait(ulPortSignal | SIGBREAKF_CTRL_C );

kprintf( "Got signal: %08lx\n", ulSignal );

      if( ulSignal & SIGBREAKF_CTRL_C )
      {
        fQuit = TRUE;
      }
      else if(ulSignal & ulPortSignal) /* Command from AL16... */
      {

kprintf( "PSA signal\n" );

        while( (pEIOsg = (ExternalIOMessage *) GetMsg(port)) )
        {
kprintf( "Got message!\n" );
          switch(pEIOsg->lCommand)
          {
            case EIO_CMD_INFO:

              /* Tell AL16 your name (please center the name
                 into an 11 char string). Fill free to choose
                 a name for your driver. */
              strncpy(pEIOsg->mpchDrvName,"AHI I/O 1.0",12);
              pEIOsg->lError = EIO_ERR_NOERROR;
              ReplyMsg((struct Message *)pEIOsg);

              kprintf("EIO_CMD_INFO received\n");
 
              break;

            case EIO_CMD_PLAY:
            {
              ULONG sample_type = -1;
              ULONG sample_freq = -1;

kprintf( "EIO_CMD_PLAY\n" );
              pEIOsg->lError = EIO_ERR_NOERROR;

              context.PSATask       = pEIOsg->ptaskSwapper;
              context.PSASignalMask = pEIOsg->ulSignalMask;

              sample_freq = pEIOsg->ulClock;

              if( pEIOsg->wDataType == EIO_DAT_16LS )
              {
                if( pEIOsg->wChannels == 1 )
                {
                  sample_type = AHIST_M16S;
                }
                else if( pEIOsg->wChannels == 2)
                {
                  sample_type = AHIST_S16S;
                }
                else
                {
                  pEIOsg->lError = EIO_ERR_CHANNELS;
                }
              }
              else
              {
                pEIOsg->lError = EIO_ERR_DATATYPE;
              }

              if( context.Sound0Loaded )
              {
                AHI_UnloadSound( 0, context.AudioCtrl );
                context.Sound0Loaded = FALSE;
              }

              if( context.Sound1Loaded )
              {
                AHI_UnloadSound( 1, context.AudioCtrl );
                context.Sound1Loaded = FALSE;
              }

              if( pEIOsg->fOnePage )
              {
                struct AHISampleInfo si;
                
                si.ahisi_Type    = sample_type;
                si.ahisi_Address = pEIOsg->pbBuffer1;
                si.ahisi_Length  = pEIOsg->lBufferSize 
                                   / AHI_SampleFrameSize( sample_type );

kprintf( "Loadsound 0: %lx, %ld\n", si.ahisi_Address, si.ahisi_Length );
                if( AHI_LoadSound( 0,
                                   AHIST_DYNAMICSAMPLE,
                                   &si,
                                   context.AudioCtrl ) != AHIE_OK )
                {
                  pEIOsg->lError = EIO_ERR_HARDFAIL;
                }
                else
                {
                  context.Sound0Loaded = TRUE;
                }
                
                context.CurrentSound = 0;
                context.NextSound    = 0;
              }
              else
              {
                struct AHISampleInfo si;
                
                si.ahisi_Type    = sample_type;
                si.ahisi_Address = pEIOsg->pbBuffer1;
                si.ahisi_Length  = pEIOsg->lBufferSize 
                                   / AHI_SampleFrameSize( sample_type );

kprintf( "Loadsound 0: %lx, %ld\n", si.ahisi_Address, si.ahisi_Length );
                if( AHI_LoadSound( 0,
                                   AHIST_DYNAMICSAMPLE,
                                   &si,
                                   context.AudioCtrl ) != AHIE_OK )
                {
                  pEIOsg->lError = EIO_ERR_HARDFAIL;
                }
                else
                {
                  context.Sound0Loaded = TRUE;
                  
                  si.ahisi_Address = pEIOsg->pbBuffer2;

kprintf( "Loadsound 1: %lx, %ld\n", si.ahisi_Address, si.ahisi_Length );
                  if( AHI_LoadSound( 1,
                                     AHIST_DYNAMICSAMPLE,
                                     &si,
                                     context.AudioCtrl ) != AHIE_OK )
                  {
                    pEIOsg->lError = EIO_ERR_HARDFAIL;
                  }
                  else
                  {
                    context.Sound1Loaded = TRUE;
                  }

                }

                context.CurrentSound = 1;
                context.NextSound    = 0;
              }

              if( pEIOsg->lError == EIO_ERR_NOERROR )
              {
kprintf( "AHI_Play\n" );
                context.Playing = TRUE;
                AHI_Play( context.AudioCtrl,
                          AHIP_BeginChannel, 0,
                          AHIP_Freq,         sample_freq,
                          AHIP_Vol,          0x10000,
                          AHIP_Pan,          0x8000,
                          AHIP_Sound,        0,
                          AHIP_EndChannel,   0,
                          TAG_DONE );

kprintf( "AHI_ControlAudio(play)\n" );
                if( AHI_ControlAudio( context.AudioCtrl,
                                      AHIC_Play, TRUE,
                                      TAG_DONE ) != AHIE_OK )
                {
                  context.Playing = FALSE;
                  pEIOsg->lError  = EIO_ERR_HARDFAIL;
                }
              }

              ReplyMsg( (struct Message *) pEIOsg );
              break;
            }

            case EIO_CMD_REC:
              /* Get parameters from AL16...*/
              ptaskAL16     = pEIOsg->ptaskSwapper;
              ulSigMaskAL16   = pEIOsg->ulSignalMask;

              /* NEW! */
              pwRecBuff1    = (WORD *)pEIOsg->pbBuffer3;
              pwRecBuff2    = (WORD *)pEIOsg->pbBuffer4;

              lSize       = pEIOsg->lBufferSize;
//              fOnePage    = pEIOsg->fOnePage;
              wDataType     = pEIOsg->wDataType;
              ulClock     = pEIOsg->ulClock;
              wChannels     = pEIOsg->wChannels;
              wDummySmp = 0;

              lCurBuff = 1;

//              bMode = MODE_REC;


//              pEIOsg->lError = EIO_ERR_NOERROR;
              pEIOsg->lError = EIO_ERR_UNKNOWNCMD;
              ReplyMsg((struct Message *)pEIOsg);

              kprintf("EIO_CMD_REC received (%luHz)",ulClock);
              if(wDataType == EIO_DAT_16LS)
                kprintf("(16 bit)\n");
#if 0
              kprintf("Recording buffer %ld\n",lCurBuff);

              pwRecTmp1 = pwRecBuff1;
              pwRecTmp2 = pwRecBuff2;
              

              /* Simulated recording */
              for(lTmp = 0;lTmp<(lSize/2);lTmp++)
                  *pwRecTmp1++ = wDummySmp;

              wDummySmp++;


              /* Only for demo: ask the timer.device to
                 signal us in DELAY seconds. Please remove
                 this and let your board signal us when it
                 completed buffer 1 */

              ptimerequest->tr_time.tv_secs = DELAY;
              ptimerequest->tr_time.tv_micro= 0;

              SendIO((struct IORequest *)ptimerequest);

              fRecording = TRUE;
#endif
              break;

            case EIO_CMD_STOP:
              pEIOsg->lError = EIO_ERR_NOERROR;
              ReplyMsg((struct Message *)pEIOsg);
              kprintf("EIO_CMD_STOP  received\n");

kprintf( "AHI_ControlAudio(stop)\n" );
              if( context.Playing )
              {
                context.Playing = FALSE;
                AHI_ControlAudio( context.AudioCtrl,
                                  AHIC_Play, FALSE,
                                  TAG_DONE );
              }

              if( context.Recording )
              {
                context.Recording = FALSE;
                AHI_ControlAudio( context.AudioCtrl,
                                  AHIC_Record, FALSE,
                                  TAG_DONE );
              }

              if( context.Sound0Loaded )
              {
                AHI_UnloadSound( 0, context.AudioCtrl );
                context.Sound0Loaded = FALSE;
              }

              if( context.Sound1Loaded )
              {
                AHI_UnloadSound( 1, context.AudioCtrl );
                context.Sound1Loaded = FALSE;
              }

              break;

            case EIO_CMD_SETPARAMS:

              pEIOsg->lError = EIO_ERR_NOERROR;
              ReplyMsg((struct Message *)pEIOsg);
              kprintf("EIO_CMD_SETPARAMS  received\n");

              /* Parameters passing removed. Doesn't
                 apply to generic boards and AHI as
                 well */

              break;


            case EIO_CMD_QUIT:

              pEIOsg->lError = EIO_ERR_NOERROR;
              ReplyMsg((struct Message *)pEIOsg);

              fQuit = TRUE;

              break;

            default:
              pEIOsg->lError = EIO_ERR_UNKNOWNCMD;
              ReplyMsg((struct Message *)pEIOsg);
              break;
          }
        }
      }
    }
  }

printf( "Exiting... " );

  if( port != NULL )
  {
    DeletePort( port );
  }
kprintf( "e1\n" );
  Destruct( &context );
kprintf( "e2\n" );
  ShutDown();
kprintf( "e3\n" );
  return 0;
}

/******************************************************************************
** Init ***********************************************************************
******************************************************************************/

static BOOL
Init( void )
{
  if( OpenAHI() )
  {
    return TRUE;
  }

  return FALSE;
}


/******************************************************************************
** ShutDown *******************************************************************
******************************************************************************/

static void
ShutDown( void )
{
  CloseAHI();
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
  if( AHIDevice != NULL )
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
** Construct ******************************************************************
******************************************************************************/

static BOOL
Construct( struct Context* this )
{
  memset( this, 0, sizeof( struct Context ) );

  this->SoundHook.h_Entry = SoundFunc;
  this->SoundHook.h_Data  = this;

  return AllocAudio( AHI_DEFAULT_ID, 
                     AHI_DEFAULT_FREQ, 
                     this );
}


/******************************************************************************
** Destruct *******************************************************************
******************************************************************************/

static void
Destruct( struct Context* this )
{
  FreeAudio( this );
}


/******************************************************************************
** AllocAudio *****************************************************************
******************************************************************************/

/* Allocates the audio hardware. May be called several times in a row
   to change the audio mode. */

static BOOL
AllocAudio( ULONG mode, 
            ULONG freq,
            struct Context* this )
{
  FreeAudio( this );

  this->AudioCtrl = AHI_AllocAudio( AHIA_AudioID,   mode,
                                    AHIA_MixFreq,   freq,
                                    AHIA_Channels,  1,
                                    AHIA_Sounds,    2,
                                    AHIA_SoundFunc, (ULONG) &this->SoundHook,
                                    TAG_DONE );

  if( this->AudioCtrl != NULL )
  {
    /* Raise volume to use the full dynamic range */

    this->MasterVol.ahie_Effect   = AHIET_MASTERVOLUME;
    this->MasterVol.ahiemv_Volume = 0x20000;
    this->MasterVolRC = AHI_SetEffect( &this->MasterVol, 
                                       this->AudioCtrl );
    return TRUE;
  }

  return FALSE;
}


/******************************************************************************
** FreeAudio ******************************************************************
******************************************************************************/

/* Deallocates the audio hardware. */

static void
FreeAudio( struct Context* this )
{
  if( this->MasterVolRC == AHIE_OK && this->AudioCtrl != NULL )
  {
    this->MasterVol.ahie_Effect = AHIET_CANCEL | AHIET_MASTERVOLUME;
    AHI_SetEffect( &this->MasterVol, 
                   this->AudioCtrl );
  }

  AHI_FreeAudio( this->AudioCtrl );
  this->AudioCtrl = NULL;
}

/******************************************************************************
** SoundFunc ******************************************************************
******************************************************************************/

static HOOKCALL ULONG
SoundFunc( REG( a0, struct Hook*            hook ),
           REG( a2, struct AHIAudioCtrl*    actrl ),
           REG( a1, struct AHISoundMessage* smsg ) )
{
  struct Context* this = (struct Context*) hook->h_Data;
  UWORD           snd;

  if( this->Playing )
  {
    snd                = this->NextSound;
    this->NextSound    = this->CurrentSound;
    this->CurrentSound = snd;

    AHI_SetSound( 0, this->NextSound, 0, 0, this->AudioCtrl, AHISF_NONE );
    Signal( this->PSATask, this->PSASignalMask );
    kprintf( "Queued sound %ld\n", this->NextSound );
  }

  return 0;
}
