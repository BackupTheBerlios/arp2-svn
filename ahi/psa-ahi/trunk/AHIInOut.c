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

/*** Definitions, prototypes and global variables ****************************/

static BOOL Init(void);
static void ShutDown(void);

static BOOL OpenAHI( void );
static void CloseAHI( void );

static BOOL AllocAudio( ULONG mode, ULONG freq );
static void FreeAudio( void );

static HOOKCALL ULONG
SoundFunc( REG( a0, struct Hook *hook ),
           REG( a2, struct AHIAudioCtrl *actrl ),
           REG( a1, struct AHISoundMessage *smsg ) );


static struct MsgPort*      AHImp     = NULL;
static struct AHIRequest*   AHIio     = NULL;
static BYTE                 AHIDevice = IOERR_OPENFAIL;

static struct AHIAudioCtrl* AudioCtrl = NULL;

static struct AHIEffMasterVolume MasterVol =
{
  AHIET_MASTERVOLUME,
  0x20000
};

static ULONG MasterVolRC = AHIE_UNKNOWN;

static struct Hook SoundHook =
{
  0,0,
  SoundFunc,
  NULL,
  NULL,
};

struct Library* AHIBase = NULL;

#define PORTNAME        "AHIInOut"

#define MODE_PLAY 0
#define MODE_REC  1

struct MyData
{
  UWORD CurrentSound;
  UWORD NextSound;
};


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

  /* Init the hardware. If there is any problem quit without setting up
     the message port */

  if( !Init() )
  {
    return RETURN_FAIL;
  }

  /* Set up the message port */

  port = CreatePort(PORTNAME,0);
  
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

printf( "Waiting for signals: %08lx.\n", ulPortSignal | ulTimerSigMask | SIGBREAKF_CTRL_C );

      ulSignal = Wait(ulPortSignal | ulTimerSigMask | SIGBREAKF_CTRL_C );

printf( "Got signal: %08lx\n", ulSignal );

      if( ulSignal & SIGBREAKF_CTRL_C )
      {
        fQuit = TRUE;
      }
      else if(ulSignal & ulPortSignal) /* Command from AL16... */
      {

printf( "PSA signal\n" );

        while( (pEIOsg = (ExternalIOMessage *) GetMsg(port)) )
        {
printf( "Got message!\n" );
          switch(pEIOsg->lCommand)
          {
            case EIO_CMD_INFO:

              /* Tell AL16 your name (please center the name
                 into an 11 char string). Fill free to choose
                 a name for your driver. */
              strncpy(pEIOsg->mpchDrvName,"AHI I/O 1.0",12);
              pEIOsg->lError = EIO_ERR_NOERROR;
              ReplyMsg((struct Message *)pEIOsg);

              printf("EIO_CMD_INFO received\n");
 
              break;

            case EIO_CMD_PLAY:
              /* Get parameters from AL16...*/
              ptaskAL16     = pEIOsg->ptaskSwapper;
              ulSigMaskAL16   = pEIOsg->ulSignalMask;
              pbBuff1     = pEIOsg->pbBuffer1;
              pbBuff2     = pEIOsg->pbBuffer2;
              lSize       = pEIOsg->lBufferSize;
              fOnePage    = pEIOsg->fOnePage;
              wDataType     = pEIOsg->wDataType;
              ulClock     = pEIOsg->ulClock;
              wChannels     = pEIOsg->wChannels;


              lCurBuff = 1;

              bMode = MODE_PLAY;

              pEIOsg->lError = EIO_ERR_NOERROR;
              ReplyMsg((struct Message *)pEIOsg);

              printf("EIO_CMD_PLAY received (%luHz)",ulClock);
              if(wDataType == EIO_DAT_16LS)
                printf("(16 bit)\n");
              else if(wDataType == EIO_DAT_8LU)
                printf("(8 bit)\n");


              /* Ask AL16 to immediatly fill next buffer */
              Signal(ptaskAL16,ulSigMaskAL16);
              printf("Sending signal...\n");


              printf("Playing buffer %ld\n",lCurBuff);
              /* Only for demo: ask the timer.device to
                 signal us in DELAY seconds. Please remove
                 this and let your board signal us when it
                 completed buffer 1 */
              ptimerequest->tr_time.tv_secs = DELAY;
              ptimerequest->tr_time.tv_micro= 0;

              SendIO((struct IORequest *)ptimerequest);


              fPlaying = TRUE;

              break;

            case EIO_CMD_REC:
              /* Get parameters from AL16...*/
              ptaskAL16     = pEIOsg->ptaskSwapper;
              ulSigMaskAL16   = pEIOsg->ulSignalMask;

              /* NEW! */
              pwRecBuff1    = (WORD *)pEIOsg->pbBuffer3;
              pwRecBuff2    = (WORD *)pEIOsg->pbBuffer4;

              lSize       = pEIOsg->lBufferSize;
              fOnePage    = pEIOsg->fOnePage;
              wDataType     = pEIOsg->wDataType;
              ulClock     = pEIOsg->ulClock;
              wChannels     = pEIOsg->wChannels;
              wDummySmp = 0;

              lCurBuff = 1;

              bMode = MODE_REC;


//              pEIOsg->lError = EIO_ERR_NOERROR;
              pEIOsg->lError = EIO_ERR_UNKNOWNCMD;
              ReplyMsg((struct Message *)pEIOsg);

              printf("EIO_CMD_REC received (%luHz)",ulClock);
              if(wDataType == EIO_DAT_16LS)
                printf("(16 bit)\n");
#if 0
              printf("Recording buffer %ld\n",lCurBuff);

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
              printf("EIO_CMD_STOP  received\n");


              /* Only for demo:ask the timer.device to
                 abort the operation. Please remove this
                 and stop the board playback. If you can't
                 stop it immediatly try to turn the volume
                 to zero... then stop it as soon as possible. */
              AbortIO((struct IORequest *)ptimerequest);
              WaitIO((struct IORequest *)ptimerequest);

              fPlaying = FALSE;
              fRecording = FALSE;

              break;

            case EIO_CMD_SETPARAMS:

              pEIOsg->lError = EIO_ERR_NOERROR;
              ReplyMsg((struct Message *)pEIOsg);
              printf("EIO_CMD_SETPARAMS  received\n");

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
      if(ulSignal & ulTimerSigMask) /* Here goes drivers code */
      {
printf( "Timer signal.\n" );
        GetMsg(pportTimer);

        if(bMode == MODE_PLAY)
        {
          if(fPlaying == FALSE)
          {
          /* this interrupt is late... sometimes happens */

          while(GetMsg(pportTimer))
          {
          }
          }
          else
          {
          Signal(ptaskAL16,ulSigMaskAL16);
          printf("Sending signal...\n");

          if(fOnePage)
          {
            /* restart playback from the same buffer
               (always buffer 1) if the whole recording
               fits in one page (buffer) */
            printf("Handling buffer %ld\n",lCurBuff);
          }
          else
          {
            if(lCurBuff == 1)
            {
              lCurBuff = 2;
              printf("Handling buffer %ld\n",lCurBuff);
            }
            else
            {
              lCurBuff = 1;
              printf("Handling buffer %ld\n",lCurBuff);
            }
          }
          /* Only for demo: ask the timer.device to
             signal us in DELAY seconds. Please remove
             this and let your board signal us when it
             completed buffer 1 */

          ptimerequest->tr_node.io_Command = TR_ADDREQUEST;
          ptimerequest->tr_node.io_Message.mn_ReplyPort = pportTimer;
          ptimerequest->tr_time.tv_secs = DELAY;
          ptimerequest->tr_time.tv_micro= 0;

          SendIO((struct IORequest *)ptimerequest);
         }
         }
         else if (bMode == MODE_REC)
         {
         if(fRecording == FALSE)
         {
          /* this interrupt is late... sometimes happens */

          while(GetMsg(pportTimer))
          {
          }
         }
         else
         {
          Signal(ptaskAL16,ulSigMaskAL16);
          printf("Sending signal...\n");

          if(lCurBuff == 1)
          {
              lCurBuff = 2;
              printf("Recording buffer %ld\n",lCurBuff);

              pwRecTmp2 = pwRecBuff2;
              for(lTmp = 0;lTmp<(lSize/2);lTmp++)
                 *pwRecTmp2++ = wDummySmp;

              wDummySmp++;
          }
          else
          {
              lCurBuff = 1;
              printf("Recording buffer %ld\n",lCurBuff);


              pwRecTmp1 = pwRecBuff1;
              for(lTmp = 0;lTmp<(lSize/2);lTmp++)
                 *pwRecTmp1++ = wDummySmp;

              wDummySmp++;
          }

          /* Only for demo: ask the timer.device to
             signal us in DELAY seconds. Please remove
             this and let your board signal us when it
             completed buffer 1 */

          ptimerequest->tr_node.io_Command = TR_ADDREQUEST;
          ptimerequest->tr_node.io_Message.mn_ReplyPort = pportTimer;
          ptimerequest->tr_time.tv_secs = DELAY;
          ptimerequest->tr_time.tv_micro= 0;

          SendIO((struct IORequest *)ptimerequest);
          }
        }
      }
    }
  }

printf( "Exiting... " );
  if(port)
    DeletePort(port);

  ShutDown();

printf( "Done.\n" );
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
    return AllocAudio( AHI_DEFAULT_ID, AHI_DEFAULT_FREQ );
  }

  return FALSE;
}


/******************************************************************************
** ShutDown *******************************************************************
******************************************************************************/

static void
ShutDown( void )
{
  FreeAudio();
  CloseAHI();
}


/******************************************************************************
** OpenAHI ********************************************************************
******************************************************************************/

/* Opens and initializes the device */

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

/* Closes the device, cleans up */

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
** AllocAudio *****************************************************************
******************************************************************************/

/* Allocates the audio hardware. May be called several times in a row
   to change the audio mode. */

static BOOL
AllocAudio( ULONG mode, ULONG freq )
{
  FreeAudio();

  AudioCtrl = AHI_AllocAudio( AHIA_AudioID,   mode,
                              AHIA_MixFreq,   freq,
                              AHIA_Channels,  1,
                              AHIA_Sounds,    2,
                              AHIA_SoundFunc, (ULONG) &SoundHook,
                              TAG_DONE );

  if( AudioCtrl != NULL )
  {
    /* Raise volume to use the full dynamic range */

    MasterVol.ahie_Effect &= ~AHIET_CANCEL;
    MasterVolRC = AHI_SetEffect( &MasterVol, AudioCtrl );
    return TRUE;
  }

  return FALSE;
}


/******************************************************************************
** FreeAudio ******************************************************************
******************************************************************************/

/* Deallocates the audio hardware */

static void
FreeAudio( void )
{
  if( MasterVolRC == AHIE_OK && AudioCtrl != NULL )
  {
    MasterVol.ahie_Effect |= AHIET_CANCEL;
    AHI_SetEffect( &MasterVol, AudioCtrl );
  }

  AHI_FreeAudio( AudioCtrl );
  AudioCtrl = NULL;
}
