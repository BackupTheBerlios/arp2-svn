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

#include <string.h>

#include "CompilerSpecific.h"
#include "DriverTypes.h"

/*** Definitions *************************************************************/

#define PORTNAME            "AHIInOut"

struct Context
{
  struct Task*              PlayTask;
  struct Task*              RecordTask;
  ULONG                     PlaySignalMask;
  ULONG                     RecordSignalMask;

  struct Hook               SoundHook;
  struct Hook               RecordHook;

  ULONG                     Mode;
  ULONG                     Frequency;
  ULONG                     CanRecord;
  ULONG                     FullDuplex;
  ULONG                     RecordSamples;
  struct AHIAudioCtrl*      AudioCtrl;
  struct AHIEffMasterVolume MasterVol;
  ULONG                     MasterVolRC;

  BOOL                      Playing;
  BOOL                      Recording;

  BOOL                      Sound0Loaded;
  BOOL                      Sound1Loaded;
  UWORD                     CurrentSound;
  UWORD                     NextSound;

  WORD*                     RecordBuffer1;
  WORD*                     RecordBuffer2;
  ULONG                     RecordBufferType;
  ULONG                     RecordBufferLength;
  ULONG                     RecordBufferOffset;
};


/*** Prototypes **************************************************************/

//void kprintf( char* fmt, ... );
#define kprintf( ... )

static BOOL Init( void );
static void ShutDown( void );

static BOOL OpenAHI( void );
static void CloseAHI( void );

static BOOL Construct( struct Context* this );
static void Destruct( struct Context* this );
static BOOL AllocAudio( ULONG mode, ULONG freq, struct Context* this );
static void FreeAudio( struct Context* this );

static HOOKCALL ULONG
SoundFunc( REG( a0, struct Hook*            hook ),
           REG( a2, struct AHIAudioCtrl*    actrl ),
           REG( a1, struct AHISoundMessage* msg ) );

static HOOKCALL ULONG
RecordFunc( REG( a0, struct Hook*             hook ),
            REG( a2, struct AHIAudioCtrl*     actrl ),
            REG( a1, struct AHIRecordMessage* msg ) );



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

  struct MsgPort* port;
  ULONG           ulSignal;
  ULONG           ulPortSignal;

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

kprintf("EIO_CMD_INFO received\n");
 
              break;

            case EIO_CMD_PLAY:
            {
              ULONG sample_type = -1;
              ULONG sample_freq = -1;

kprintf( "EIO_CMD_PLAY\n" );

              pEIOsg->lError         = EIO_ERR_NOERROR;

              context.PlayTask       = pEIOsg->ptaskSwapper;
              context.PlaySignalMask = pEIOsg->ulSignalMask;

              sample_freq            = pEIOsg->ulClock;

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

              /* Re-allocate audio using new frequency if it has changed. */

              if( pEIOsg->lError == EIO_ERR_NOERROR 
                  && context.Frequency != sample_freq )
              {
                if( !AllocAudio( context.Mode,
                                 sample_freq, 
                                 &context ) )
                {
                  pEIOsg->lError = EIO_ERR_HARDFAIL;
                }
              }

              if( pEIOsg->lError == EIO_ERR_NOERROR )
              {
                AHI_UnloadSound( 0, context.AudioCtrl );
                AHI_UnloadSound( 1, context.AudioCtrl );

                context.Sound0Loaded = FALSE;
                context.Sound1Loaded = FALSE;

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

              break;
            }

            case EIO_CMD_REC:
            {
              ULONG sample_type = -1;
              ULONG sample_freq = -1;

kprintf( "EIO_CMD_REC\n" );

              pEIOsg->lError           = EIO_ERR_NOERROR;

              context.RecordTask       = pEIOsg->ptaskSwapper;
              context.RecordSignalMask = pEIOsg->ulSignalMask;

              sample_freq              = pEIOsg->ulClock;

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

              /* Re-allocate audio using new frequency if it has changed. */

              if( pEIOsg->lError == EIO_ERR_NOERROR 
                  && context.Frequency != sample_freq )
              {
                if( !AllocAudio( context.Mode, 
                                 sample_freq, 
                                 &context ) )
                {
                  pEIOsg->lError = EIO_ERR_HARDFAIL;
                }
              }

              /* Make sure the mode is fit for recording */

              if( pEIOsg->lError == EIO_ERR_NOERROR && !context.CanRecord )
              {
                pEIOsg->lError = EIO_ERR_HARDFAIL;
              }

              if( pEIOsg->lError == EIO_ERR_NOERROR )
              {
                context.RecordBuffer1      = (WORD*) pEIOsg->pbBuffer3;
                context.RecordBuffer2      = (WORD*) pEIOsg->pbBuffer4;
                context.RecordBufferType   = sample_type;
                context.RecordBufferLength = pEIOsg->lBufferSize
                                           / AHI_SampleFrameSize( sample_type );
                context.RecordBufferOffset = 0;

                if( context.RecordSamples > context.RecordBufferLength )
                {
                  pEIOsg->lError = EIO_ERR_BUFFERSIZE;
                }

                context.Recording = TRUE;

                if( AHI_ControlAudio( context.AudioCtrl,
                                      AHIC_Record, TRUE,
                                      TAG_DONE ) != AHIE_OK )
                {
                  context.Recording = FALSE;
                  pEIOsg->lError    = EIO_ERR_HARDFAIL;
                }
              }

              break;
            }

            case EIO_CMD_STOP:
              pEIOsg->lError = EIO_ERR_NOERROR;
kprintf("EIO_CMD_STOP  received\n");

              AHI_ControlAudio( context.AudioCtrl,
                                AHIC_Play,   FALSE,
                                AHIC_Record, FALSE,
                                TAG_DONE );

              context.Playing   = FALSE;
              context.Recording = FALSE;

              AHI_UnloadSound( 0, context.AudioCtrl );
              AHI_UnloadSound( 1, context.AudioCtrl );

              context.Sound0Loaded = FALSE;
              context.Sound1Loaded = FALSE;

              break;

            case EIO_CMD_SETPARAMS:
              pEIOsg->lError = EIO_ERR_NOERROR;

              /* Parameters passing removed. Doesn't
                 apply to generic boards and AHI as
                 well */

              break;


            case EIO_CMD_QUIT:
              pEIOsg->lError = EIO_ERR_NOERROR;
              fQuit = TRUE;
              break;

            default:
              pEIOsg->lError = EIO_ERR_UNKNOWNCMD;
              break;
          }

kprintf( "Replying message (%ld)\n", pEIOsg->lError );
          ReplyMsg((struct Message *)pEIOsg);
        }
      }
    }
  }

kprintf( "Exiting... " );

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

/* Initializes the Context structure. */

static BOOL
Construct( struct Context* this )
{
  memset( this, 0, sizeof( struct Context ) );

  this->SoundHook.h_Entry  = SoundFunc;
  this->SoundHook.h_Data   = this;

  this->RecordHook.h_Entry = RecordFunc;
  this->RecordHook.h_Data  = this;

  /* Pre-allocate the audio hardware, so that nobody steals it... */

  return AllocAudio( AHI_DEFAULT_ID,
                     AHI_DEFAULT_FREQ,
                     this );
}


/******************************************************************************
** Destruct *******************************************************************
******************************************************************************/

/* Deallocates everything in the Context structure. */

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
                                    AHIA_RecordFunc, (ULONG) &this->RecordHook,
                                    TAG_DONE );

  if( this->AudioCtrl != NULL )
  {
    this->Frequency    = freq;

    if( AHI_GetAudioAttrs( AHI_INVALID_ID, this->AudioCtrl,
                           AHIDB_AudioID,          (ULONG) &this->Mode,
                           AHIDB_MaxRecordSamples, (ULONG) &this->RecordSamples,
                           AHIDB_Record,           (ULONG) &this->CanRecord,
                           AHIDB_FullDuplex,       (ULONG) &this->FullDuplex,
                           TAG_DONE ) )
    {
      /* Raise volume to use the full dynamic range */

      this->MasterVol.ahie_Effect   = AHIET_MASTERVOLUME;
      this->MasterVol.ahiemv_Volume = 0x20000;
      this->MasterVolRC = AHI_SetEffect( &this->MasterVol, 
                                         this->AudioCtrl );
      return TRUE;
    }
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

/* Called by AHI each time a sound has been started */

static HOOKCALL ULONG
SoundFunc( REG( a0, struct Hook*            hook ),
           REG( a2, struct AHIAudioCtrl*    actrl ),
           REG( a1, struct AHISoundMessage* msg ) )
{
  struct Context* this = (struct Context*) hook->h_Data;
  UWORD           snd;

  if( this->Playing )
  {
    snd                = this->CurrentSound;;
    this->CurrentSound = this->NextSound;
    this->NextSound    = snd;

    AHI_SetSound( 0, snd, 0, 0, this->AudioCtrl, AHISF_NONE );
    Signal( this->PlayTask, this->PlaySignalMask );
kprintf( "Queued sound %ld\n", snd );
  }

  return 0;
}

/******************************************************************************
** RecordFunc *****************************************************************
******************************************************************************/

/* Called by AHI each time there is new data available */

static HOOKCALL ULONG
RecordFunc( REG( a0, struct Hook*             hook ),
            REG( a2, struct AHIAudioCtrl*     actrl ),
            REG( a1, struct AHIRecordMessage* msg ) )
{
  struct Context* this = (struct Context*) hook->h_Data;

  if( this->Recording && msg->ahirm_Type == AHIST_S16S )
  {
    int   i;
    int   len;
    WORD* src;
    WORD* dst;
kprintf( ". " );
//kprintf( "Recording hook\n" );
    src = msg->ahirm_Buffer;

    len = min( this->RecordBufferLength - this->RecordBufferOffset,
               msg->ahirm_Length );

    while( len > 0 )
    {
//kprintf( "Copying %ld samples\n", len );
      switch( this->RecordBufferType )
      {
        case AHIST_M16S:
          /* Copy left channel only */

          dst = &this->RecordBuffer1[ this->RecordBufferOffset ];

          for( i = 0; i < len; i++ )
          {
            *dst++ = *src;
            src   += 2;
          }
          break;

        case AHIST_S16S:
          /* Copy left channel only */

          dst = &this->RecordBuffer1[ this->RecordBufferOffset * 2 ];

          for( i = 0; i < len; i++ )
          {
            *dst++ = *src++;
            *dst++ = *src++;
          }
          break;
      }

      this->RecordBufferOffset += len;

      if( this->RecordBufferOffset >= this->RecordBufferLength )
      {
        WORD* buf;
        buf                 = this->RecordBuffer1;
        this->RecordBuffer1 = this->RecordBuffer2;
        this->RecordBuffer2 = buf;
      
        this->RecordBufferOffset = 0;

        Signal( this->RecordTask, this->RecordSignalMask );
//kprintf( "Swapped to buffer %lx\n", buf );

        len = min( this->RecordBufferLength,
                   msg->ahirm_Length - len );
      }
      else
      {
        /* End while loop */
        len = 0;
      }
    }
  }

  return 0;
}
