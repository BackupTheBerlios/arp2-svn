/*
     vt82c - CAMD driver for VIA 82Cxxx codec
     Copyright (C) 2003 Martin Blom <martin@blom.org>

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

#include <devices/timer.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <libraries/openpci.h>
#include <midi/camddevices.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/openpci.h>

#include "camdstubs.h"
#include "version.h"

struct PortInfo
{
    struct pci_dev*     PCIDev;
    ULONG               IOBase;
    struct Interrupt    Interrupt;
    struct Interrupt    TimerInterrupt;
    
    struct MsgPort      MsgPort;
    struct timerequest* TimerRequest;
    BOOL                TimerRequestActive;
    
    BOOL                InterruptAdded;
    BOOL                TimerOpened;

    APTR                TransmitFunc;
    APTR                ReceiveFunc;
    APTR                UserData;
};

#define TIMER_DELAY_US          1000


#define CONFIG_REG              0x42
#define PNP_REG                 0x43

#define CONFIGF_MIDI_ENABLE     0x02
#define CONFIGF_MIDI_IRQDISABLE 0x40
#define CONFIGF_MIDI_PNP        0x80

#define DATA_PORT               0
#define COMMAND_PORT            1
#define STATUS_PORT             1

#define STATUSF_OUTPUT          0x40
#define STATUSF_INPUT           0x80

#define COMMAND_RESET           0xff
#define DATA_ACKNOWLEDGE        0xfe
#define COMMAND_UART_MODE       0x3f

#define MPU401_OUTPUT_READY(b)  ((pci_inb((b)+STATUS_PORT) & STATUSF_OUTPUT) == 0)
#define MPU401_INPUT_READY(b)   ((pci_inb((b)+STATUS_PORT) & STATUSF_INPUT) == 0)

#define MPU401_CMD(c,b)         pci_outb(c, (b)+COMMAND_PORT)
#define MPU401_READ(b)          pci_inb( (b)+DATA_PORT )
#define MPU401_WRITE(v,b)       pci_outb(v, (b)+DATA_PORT )

VOID
_Expunge( ULONG dummy );

VOID
_ClosePort( struct MidiDeviceData *data, LONG portnum );



/*** Module entry (exactly 4 bytes!!) *****************************************/

__asm("
  moveq #-1,d0
  rts
");

/*** Identification data must follow directly *********************************/

static const struct MidiDeviceData MidiDeviceData =
{
  MDD_Magic,
  "vt82c",
  "VIA 82Cxxx CAMD MIDI driver " VERS " @2003 Martin Blom",
  VERSION, REVISION,
  gwInit,
  gwExpunge,
  gwOpenPort,
  gwClosePort,
  1,        // For some braindamaged reason, camd.library V40 reads
	    // this value BEFORE calling Init(). :-(
  1         // Use new-style if using camd.library V40
};

/*** Global data **************************************************************/

static struct ExecBase*    SysBase         = NULL;
static struct DosLibrary*  DOSBase         = NULL;
static struct Library*     OpenPciBase     = NULL;
static ULONG               CAMDv40         = FALSE;
static const char          VersionString[] = "$VER: vt82c " VERS "\r\n";

static struct PortInfo     PortInfos[ 1 ];

/*** Debug code ***************************************************************/

static UWORD rawputchar_m68k[] = 
{
  0x2C4B,             // MOVEA.L A3,A6
  0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
  0x4E75              // RTS
};


static void
KPrintFArgs( UBYTE* fmt, 
             ULONG* args )
{
  RawDoFmt( fmt, args, (void(*)(void)) rawputchar_m68k, SysBase );
}

#define KPrintF( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  KPrintFArgs( (fmt), _args );     \
})


/*** CAMD callbacks ***********************************************************/

static ULONG
TransmitFunc( struct PortInfo* pi )
{
  ULONG res;

  __asm volatile (
    "movel %1,a2
     movel %2,a0
     jsr   (a0)
     swapw d0
     movew d1,d0
     swapw d0"
    : "=r"(res)
    : "m" (pi->UserData), "m" (pi->TransmitFunc)
    : "a0", "a2" );

  return res;
}


static VOID
ReceiveFunc( ULONG            input_byte,
	     struct PortInfo* pi )
{
  __asm volatile (
    "movel %0,d0
     movel %1,a2
     movel %2,a0
     jsr   (a0)"
    : 
    : "m" (input_byte), "m" (pi->UserData), "m" (pi->ReceiveFunc)
    : "d0", "a0", "a2" );
}


/*** Interrupt ****************************************************************/

ULONG __interrupt
_Interrupt( struct PortInfo* pi )
{
  struct timerequest* tr;
  
/*   KPrintF( "Got interrupt on port 0x%08lx, status %02lx\n", */
/* 	   (ULONG) pi, pci_inb(pi->IOBase+STATUS_PORT) ); */

  /*** Handle the input (before output) ***/
  
  while( MPU401_INPUT_READY( pi->IOBase ) )
  {
    ULONG b = MPU401_READ( pi->IOBase );

#ifdef DEBUG
    KPrintF( "\t%02lx\n", b );
#endif
    
    ReceiveFunc( b, pi );
  }


  /*** Handle the output ***/
  
  tr = (struct timerequest*) GetMsg( &pi->MsgPort );

  if( tr != NULL )
  {
    ULONG b;
    
    Disable();
    pi->TimerRequestActive = FALSE;
    Enable();

    while( TRUE )
    {
      if( ! MPU401_OUTPUT_READY( pi->IOBase ) )
      {
	// Not ready -- do it later

	Disable();

	if( ! pi->TimerRequestActive )
	{
	  pi->TimerRequestActive = TRUE;
	
	  pi->TimerRequest->tr_node.io_Command = TR_ADDREQUEST;
	  pi->TimerRequest->tr_time.tv_micro   = TIMER_DELAY_US;

	  BeginIO( (struct IORequest *) pi->TimerRequest );
#ifdef DEBUG
	  KPrintF(">\n");
#endif
	}

	Enable();
	break;
      }

      b = TransmitFunc( pi );

#ifdef DEBUG
      KPrintF( "%08lx\n", b );
#endif
      
      if( ( b & 0x00008100 ) == 0x0000 )
      {
	MPU401_WRITE( b & 255, pi->IOBase );
      }

      if( ( !CAMDv40 && ( b & 0x00ff0000 ) != 0 ) ||
	  ( b & 0x00008100 ) != 0 )
      {
	break;
      }
    }   
  }
  
  return FALSE;
};

/*** ActivateXmit *************************************************************/

VOID
_ActivateXmit( APTR  userdata,
	       ULONG portnum )
{
  struct PortInfo* pi;
  
  // In the original CAMD, there is no port number :-(

  portnum &= 255;

#ifdef DEBUG
  KPrintF( "_ActiavteXmit( %08lx, %ld )\n", (ULONG) userdata, portnum );
#endif
  
  if( !CAMDv40 )
  {
    for( portnum = 0; portnum < MidiDeviceData.NPorts; ++portnum )
    {
      if( userdata == PortInfos[ portnum ].UserData )
      {
	break;
      }
    }

    if( portnum == MidiDeviceData.NPorts )
    {
      return;
    }
  }

  pi = &PortInfos[ portnum & 255 ];

  Disable();

  if( ! pi->TimerRequestActive )
  {
    pi->TimerRequestActive = TRUE;
    
    pi->TimerRequest->tr_node.io_Command = TR_ADDREQUEST;
    pi->TimerRequest->tr_time.tv_micro   = 0;

    BeginIO( (struct IORequest *) pi->TimerRequest );
#ifdef DEBUG
    KPrintF(">\n");
#endif
  }

  Enable();
}

struct MidiPortData MidiPortData =
{
  gwActivateXmit
};


/*** Init *********************************************************************/

BOOL
_Init( struct ExecBase* sysbase )
{
#ifdef __AROS__
  SysBase = sysbase;
#else
  // sysbase is not valid in the original CAMD anyway
  SysBase = *(struct ExecBase**) 4;
#endif

#ifdef DEBUG
  KPrintF( "_Init( 0x%08lx )\n", (ULONG) sysbase );
#endif
  
  DOSBase = (struct DosLibrary*) OpenLibrary( DOSNAME, 37 );

  if( DOSBase == NULL )
  {
    return FALSE;
  }
    
  OpenPciBase = OpenLibrary( "openpci.library", 1 );

  if( OpenPciBase == NULL )
  {
    _Expunge( 0 );
    return FALSE;
  }
  
  return TRUE;
}


/*** Expunge ******************************************************************/

VOID
_Expunge( ULONG dummy )
{
#ifdef DEBUG
  KPrintF( "_Expunge()\n" );
#endif
  
  CloseLibrary( (struct Library*) DOSBase );
  CloseLibrary( OpenPciBase );
}


/*** OpenPort *****************************************************************/

struct MidiPortData*
_OpenPort( struct MidiDeviceData* data,
	   LONG                   portnum,
	   APTR                   transmitfunc,
	   APTR                   receivefunc,
	   APTR                   userdata )
{
  static struct Library* camdbase = NULL;
  struct PortInfo*       pi = &PortInfos[ portnum & 255 ];

  int i;
  int port;
  int conf_reg;
  int pnp_reg;
  int command_word;

  portnum &= 255;
  
#ifdef DEBUG
  KPrintF( "_OpenPort( 0x%08lx, %ld, 0x%08lx, 0x%08lx, 0x%08lx )\n",
	   (ULONG) data, portnum,
	   (ULONG) transmitfunc, (ULONG) receivefunc, (ULONG) userdata );
#endif
  
  if( camdbase == NULL )
  {
    camdbase = OpenLibrary( "camd.library", 0 );

    if( camdbase != NULL )
    {
      CAMDv40 = ( camdbase->lib_Version >= 40 );
    }

    // Close library but leave pointer set, so we never execute this
    // code again.
    CloseLibrary( camdbase );
  }

  NewList( &pi->MsgPort.mp_MsgList );

  pi->Interrupt.is_Node.ln_Type = NT_INTERRUPT;
  pi->Interrupt.is_Node.ln_Pri  = 0;
  pi->Interrupt.is_Node.ln_Name = (STRPTR) "vt82c receiver";
  pi->Interrupt.is_Code         = (void(*)(void)) &gwInterrupt;
  pi->Interrupt.is_Data         = pi;

  pi->TimerInterrupt.is_Node.ln_Type = NT_INTERRUPT;
  pi->TimerInterrupt.is_Node.ln_Pri  = 0;
  pi->TimerInterrupt.is_Node.ln_Name = (STRPTR) "vt82c transmitter";
  pi->TimerInterrupt.is_Code         = (void(*)(void)) &gwInterrupt;
  pi->TimerInterrupt.is_Data         = pi;

  pi->MsgPort.mp_Node.ln_Type   = NT_MSGPORT;
  pi->MsgPort.mp_Flags          = PA_SOFTINT;
  pi->MsgPort.mp_SigTask        = (struct Task *) &pi->TimerInterrupt;

  
  pi->TransmitFunc         = transmitfunc;
  pi->ReceiveFunc          = receivefunc;
  pi->UserData             = userdata;

  pi->TimerRequest = CreateIORequest( &pi->MsgPort, sizeof( struct timerequest ) );

  if( pi->TimerRequest == NULL )
  {
    goto error;
  }

  if( OpenDevice( TIMERNAME, UNIT_MICROHZ,
		  (struct IORequest *) pi->TimerRequest, 0 ) )
  {
    goto error;
  }

  pi->TimerOpened = TRUE;


  pi->PCIDev = NULL;
  port = 0;

  do
  {
    pi->PCIDev = pci_find_device( 0x1106, 0x3058, pi->PCIDev );
  } while( pi->PCIDev != NULL && --port >= 0 );

  if( pi->PCIDev == NULL )
  {
    goto error;
  }

  pi->InterruptAdded = pci_add_intserver( &pi->Interrupt, pi->PCIDev );

  if( !pi->InterruptAdded )
  {
    goto error;
  }

  /*** Enable IO space */

  command_word = pci_read_config_word( PCI_COMMAND, pi->PCIDev );
  command_word |= PCI_COMMAND_IO;
  pci_write_config_word( PCI_COMMAND, command_word, pi->PCIDev );


  /*** Disable MIDI interrupts */

  conf_reg = pci_read_config_byte( CONFIG_REG, pi->PCIDev );
  conf_reg |= CONFIGF_MIDI_IRQDISABLE | CONFIGF_MIDI_ENABLE;
  conf_reg &= ~CONFIGF_MIDI_PNP;
  pci_write_config_byte( CONFIG_REG, conf_reg, pi->PCIDev );

  pnp_reg = pci_read_config_byte( PNP_REG, pi->PCIDev );

  pi->IOBase = 0xEFFFF000 + 0x300 + ( ( pnp_reg >> 2 ) & 0x03 ) * 16;


  /*** Flush MPU */

  while( MPU401_INPUT_READY( pi->IOBase ) )
  {
    MPU401_READ( pi->IOBase );
  }


  /*** Reset it (try two times) */

  for( i = 0; i < 2; ++i )
  {
    if( ! MPU401_OUTPUT_READY( pi->IOBase ) )
    {
      Delay( 2 );
    }

    if( ! MPU401_OUTPUT_READY( pi->IOBase ) )
    {
      KPrintF( "MPU-401 not ready for reset command.\n" );
      goto error;
    }

    MPU401_CMD( COMMAND_RESET, pi->IOBase );

    if( ! MPU401_INPUT_READY( pi->IOBase ) )
    {
      Delay( 2 );
    }

    if( ! MPU401_INPUT_READY( pi->IOBase ) )
    {
      if( i == 1 )
      {
	KPrintF( "MPU-401 not responding to reset command.\n" );
	goto error;
      }
      else continue;
    }

    if( MPU401_READ( pi->IOBase ) != DATA_ACKNOWLEDGE )
    {
      if( i == 1 )
      {
	KPrintF( "MPU-401 did not acknowledge reset command.\n" );
	goto error;
      }
      else continue;
    }
  }


  /*** Flush again */

  while( MPU401_INPUT_READY( pi->IOBase ) )
  {
    MPU401_READ( pi->IOBase );
  }

    
  /*** Enter UART mode */
    
  if( ! MPU401_OUTPUT_READY( pi->IOBase ) )
  {
    Delay( 2 );
  }

  if( ! MPU401_OUTPUT_READY( pi->IOBase ) )
  {
    KPrintF( "MPU-401 not ready for UART mode command.\n" );
    goto error;
  }

  MPU401_CMD( COMMAND_UART_MODE, pi->IOBase );

  if( ! MPU401_INPUT_READY( pi->IOBase ) )
  {
    Delay( 2 );
  }

  if( ! MPU401_INPUT_READY( pi->IOBase ) )
  {
    KPrintF( "MPU-401 not responding to UART mode command.\n" );
    goto error;
  }

  if( MPU401_READ( pi->IOBase ) != DATA_ACKNOWLEDGE )
  {
    KPrintF( "MPU401 did not acknowledge UART mode command.\n" );
    goto error;
  }


  /*** Enable interrupts */
    
  conf_reg &= ~CONFIGF_MIDI_IRQDISABLE;
  pci_write_config_byte( CONFIG_REG, conf_reg, pi->PCIDev );
  
  return &MidiPortData;

error:
  _ClosePort( data, portnum );
  return NULL;
}


/*** ClosePort ****************************************************************/

VOID
_ClosePort( struct MidiDeviceData *data, LONG portnum )
{
  struct PortInfo* pi = &PortInfos[ portnum & 255 ];

  portnum &= 255;

#ifdef DEBUG
  KPrintF( "_ClosePort( 0x%08lx, %ld )\n", (ULONG) data, portnum );
#endif

  if( pi->InterruptAdded )
  {
    pci_rem_intserver( &pi->Interrupt, pi->PCIDev );
    pi->InterruptAdded = FALSE;
  }

  if( pi->TimerRequest != NULL )
  {
    // TODO: Abort and cancel active request here somehow ..
    
    if( pi->TimerOpened )
    {
      CloseDevice( (struct IORequest*) pi->TimerRequest );
    }

    DeleteIORequest( pi->TimerRequest );
  }
}


/*** Test code ****************************************************************/

#ifdef TEST

int main( void )
{
  if( gwInit( *(struct ExecBase**) 4 ) )
  {
    struct MidiPortData* mpd = gwOpenPort( (struct MidiDeviceData*)
					   &MidiDeviceData,
					   MidiDeviceData.NPorts - 1,
					   NULL, NULL, (APTR) 0xc0deda1a );

    if( mpd != NULL )
    {
      gwClosePort( (struct MidiDeviceData*) &MidiDeviceData,
		   MidiDeviceData.NPorts - 1 );
    }
      
    gwExpunge( 0 );
  }
};

#endif
