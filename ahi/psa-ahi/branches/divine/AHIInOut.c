
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <stdio.h>
#include <string.h>
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include "DriverTypes.h"


#define PORTNAME "AHIInOut"

BOOL Init(void);
void ShutDown(void);

/********************** Only for demo **********************/
#define DELAY    6

struct timerequest *InitTimer(void);
void ShutDownTimer(void);

struct MsgPort      *pportTimer;
struct timerequest  *ptimerequest;
ULONG               ulTimerSigMask;

#define MODE_PLAY 0
#define MODE_REC  1
/***********************************************************/


int
main( void )
{
    ExternalIOMessage *pEIOsg;

    struct MsgPort      *port;
    BOOL                fQuit = FALSE,fPlaying = FALSE,fOnePage = FALSE;
    ULONG               ulSignal,ulPortSignal;

    BYTE                *pbBuff1,*pbBuff2;

    WORD		*pwRecBuff1 = NULL ,*pwRecBuff2 = NULL;
    WORD                *pwRecTmp1,*pwRecTmp2;
    WORD 		wDummySmp = 0;
    BOOL 		fRecording;

    BYTE 		bMode = -1;

    LONG                lSize = 0;
    LONG                lCurBuff = 0;
    ULONG               ulClock;
    WORD                wChannels;
    WORD                wDataType;
    struct Task         *ptaskAL16 = NULL;
    ULONG               ulSigMaskAL16 = 0;

    LONG lTmp;

    /* Init the hardware. If there is any problem quit without setting up
       the message port */

    if (!Init())
        return RETURN_FAIL;

    /* Set up the message port */
    port = CreatePort(PORTNAME,0);
    
    if( port != NULL )
    {
        ulPortSignal = 1 << port->mp_SigBit;
        
        fQuit = FALSE;
        fPlaying = FALSE;
        fRecording = FALSE;

        while (!fQuit)
        {
            /* Wait for messages from AL16 (commands) and for signals from
               your board (the board must signal your driver each time a
               buffer has been completed).
               In this demo the timer.device signals us every DELAY seconds */

            ulSignal = Wait(ulPortSignal | ulTimerSigMask | SIGBREAKF_CTRL_C );

            if( ulSignal & SIGBREAKF_CTRL_C )
            {
              fQuit = TRUE;
            }
            else if(ulSignal & ulPortSignal) /* Command from AL16... */
            {
                while( (pEIOsg = (ExternalIOMessage *) GetMsg(port)) )
                {
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
                            ptaskAL16       = pEIOsg->ptaskSwapper;
                            ulSigMaskAL16   = pEIOsg->ulSignalMask;
                            pbBuff1         = pEIOsg->pbBuffer1;
                            pbBuff2         = pEIOsg->pbBuffer2;
                            lSize           = pEIOsg->lBufferSize;
                            fOnePage        = pEIOsg->fOnePage;
                            wDataType       = pEIOsg->wDataType;
                            ulClock         = pEIOsg->ulClock;
                            wChannels       = pEIOsg->wChannels;


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
                            ptaskAL16       = pEIOsg->ptaskSwapper;
                            ulSigMaskAL16   = pEIOsg->ulSignalMask;

                            /* NEW! */
                            pwRecBuff1      = (WORD *)pEIOsg->pbBuffer3;
                            pwRecBuff2      = (WORD *)pEIOsg->pbBuffer4;

                            lSize           = pEIOsg->lBufferSize;
                            fOnePage        = pEIOsg->fOnePage;
                            wDataType       = pEIOsg->wDataType;
                            ulClock         = pEIOsg->ulClock;
                            wChannels       = pEIOsg->wChannels;
                            wDummySmp = 0;

                            lCurBuff = 1;

                            bMode = MODE_REC;


                            pEIOsg->lError = EIO_ERR_NOERROR;
                            ReplyMsg((struct Message *)pEIOsg);

                            printf("EIO_CMD_REC received (%luHz)",ulClock);
                            if(wDataType == EIO_DAT_16LS)
                                printf("(16 bit)\n");

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
    if(port)
        DeletePort(port);

    ShutDown();
    
    return 0;
}

BOOL Init(void)
{
    /* Init hardware here */

    ptimerequest = InitTimer();

    if( ptimerequest != NULL )
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

void ShutDown(void)
{
    ShutDownTimer();
}

struct timerequest *InitTimer(void)
{
    struct timerequest *ptimerequest;

    /* Only for demo */

    pportTimer = CreatePort(0,0);
    if( pportTimer != NULL )
    {
        ptimerequest = (struct timerequest *)CreateExtIO(pportTimer, sizeof(struct timerequest));

        if( ptimerequest != NULL )
        {
            if(0 == OpenDevice("timer.device",UNIT_VBLANK,(struct IORequest *)ptimerequest,0))
            {
                ptimerequest->tr_node.io_Command = TR_ADDREQUEST;
                ptimerequest->tr_node.io_Message.mn_ReplyPort = pportTimer;
                ptimerequest->tr_time.tv_secs = DELAY;
                ptimerequest->tr_time.tv_micro= 0;

                ulTimerSigMask = 1L << pportTimer->mp_SigBit;

                return(ptimerequest);
            }
        }
    }
    return(NULL);
}


void ShutDownTimer(void)
{
    /* Only for demo */

    if(ptimerequest != 0)
    {
        if(pportTimer)
        {
            DeletePort(pportTimer);
        }
        CloseDevice((struct IORequest *)ptimerequest);
        DeleteExtIO((struct IORequest *)ptimerequest);
    }
}
