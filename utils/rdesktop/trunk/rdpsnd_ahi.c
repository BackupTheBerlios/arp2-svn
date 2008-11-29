/* 
   rdesktop: A Remote Desktop Protocol client.
   Sound Channel Process Functions - Amiga/AHI
   Copyright (C) Martin Blom 2004-2005
   Copyright (C) Matthew Chapman 2003
   Copyright (C) GuoJunBo guojunbo@ict.ac.cn 2003
   Copyright (C) Michael Gernoth mike@zerfleddert.de 2003

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
#include "rdpsnd.h"

#include <devices/ahi.h>
#include <proto/exec.h>

#define MAX_QUEUE	10

int amiga_audio_unit   = 0;
int amiga_audio_signal = -1;

static short g_samplewidth;

static struct audio_packet
{
	struct stream s;
	uint16 tick;
	uint8 index;
} packet_queue[MAX_QUEUE];
static unsigned int queue_hi, queue_lo;

static struct MsgPort*    ahi_mp           = NULL;
static struct AHIRequest* ahi_iorequest    = NULL;
static APTR               ahi_iocopy       = NULL;
static BYTE               ahi_device       = -1;

static struct AHIRequest* ahi_io[ 2 ]      = { NULL, NULL };
static APTR               ahi_io_data[ 2 ] = { NULL, NULL };

static BOOL               ahi_swapaudio;
static ULONG              ahi_sampletype;
static ULONG              ahi_samplerate;
static ULONG              ahi_volume;
static ULONG              ahi_pan;

RD_BOOL ahi_out_open(void);
void ahi_out_close(RD_BOOL abort);
RD_BOOL ahi_out_format_supported(RD_WAVEFORMATEX * pwfx);
RD_BOOL ahi_out_set_format(RD_WAVEFORMATEX * pwfx);
void ahi_out_volume(uint16 left, uint16 right);
void ahi_out_write(STREAM s, uint16 tick, uint8 index);
void ahi_out_play(void);





RD_BOOL
ahi_out_open(void)
{
//  printf( "wave_open\n" );

  ahi_volume = 0x10000;
  ahi_pan    = 0x08000;
  
  ahi_mp = CreateMsgPort();

  if( ahi_mp != NULL )
  {
    amiga_audio_signal = ahi_mp->mp_SigBit;

    ahi_iorequest = (struct AHIRequest *)CreateIORequest( ahi_mp, sizeof( struct AHIRequest ) );

    if( ahi_iorequest != NULL )
    {
      ahi_iorequest->ahir_Version = 4;

      ahi_device = OpenDevice( AHINAME, amiga_audio_unit, (struct IORequest*) ahi_iorequest, 0 );

      if( ahi_device == 0 )
      {
	ahi_iocopy = AllocVec( sizeof( *ahi_iorequest ), MEMF_ANY );

	if( ahi_iocopy != NULL )
	{
	  bcopy( ahi_iorequest, ahi_iocopy, sizeof( *ahi_iorequest ) );

	  ahi_io[ 0 ] = ahi_iorequest;
	  ahi_io[ 1 ] = ahi_iocopy;

	  queue_lo = queue_hi = 0;

	  return True;
	}
      }
    }
  }

  ahi_out_close(True);
  return False;
}

void
ahi_out_close(RD_BOOL abort)
{
//  printf( "wave_close\n" );
  
  /* Ack all remaining packets */
  while (queue_lo != queue_hi)
  {
//    printf("killing\n");
    rdpsnd_send_completion(packet_queue[queue_lo].tick, packet_queue[queue_lo].index);
    free(packet_queue[queue_lo].s.data);
    queue_lo = (queue_lo + 1) % MAX_QUEUE;
  }

  if( ahi_io_data[ 0 ] != NULL )
  {
    if( abort ) {
//      printf("abort 0\n");
      AbortIO( (struct IORequest*) ahi_io[ 0 ] );
    }
    WaitIO( (struct IORequest*) ahi_io[ 0 ] );
    free( ahi_io_data[ 0 ] );
    ahi_io_data[ 0 ] = NULL;
  }

  if( ahi_io_data[ 1 ] != NULL )
  {
    if( abort ) {
//      printf("abort 1\n");
      AbortIO( (struct IORequest*) ahi_io[ 1 ] );
    }
    WaitIO( (struct IORequest*) ahi_io[ 1 ] );
    free( ahi_io_data[ 1 ] );
    ahi_io_data[ 1 ] = NULL;
  }

  if ( ahi_iocopy != NULL )
  {
    FreeVec( ahi_iocopy );
    ahi_iocopy = NULL;
  }

  if( ahi_device == 0 )
  {
    CloseDevice( (struct IORequest*) ahi_iorequest );
    ahi_device = -1;
  }

  if ( ahi_iorequest != NULL )
  {
    DeleteIORequest( (struct IORequest*) ahi_iorequest );
    ahi_iorequest = NULL;
  }

  if (amiga_audio_signal != -1)
  {
    DeleteMsgPort( ahi_mp );
    amiga_audio_signal = -1;
  }
}

RD_BOOL
ahi_out_format_supported(RD_WAVEFORMATEX * pwfx)
{
//  printf( "ahi_out_format_supported\n" );
  
  if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
    return False;
  if ((pwfx->nChannels != 1) && (pwfx->nChannels != 2))
    return False;
  if ((pwfx->wBitsPerSample != 8) && (pwfx->wBitsPerSample != 16))
    return False;
  
  return True;
}

RD_BOOL
ahi_out_set_format(RD_WAVEFORMATEX * pwfx)
{
  int test = 1;
  
//  printf( "ahi_out_set_format\n" );
  
  ahi_swapaudio = False;

  if (pwfx->wBitsPerSample == 8)
  {
    if( pwfx->nChannels == 1 ) 
      ahi_sampletype = AHIST_M8S;
    else
      ahi_sampletype = AHIST_S8S;
  }
  else if (pwfx->wBitsPerSample == 16)
  {
    if( pwfx->nChannels == 1 ) 
      ahi_sampletype = AHIST_M16S;
    else
      ahi_sampletype = AHIST_S16S;

    /* Do we need to swap the 16bit values? (Are we BigEndian) */
    ahi_swapaudio = !(*(uint8 *) (&test));
  }

  ahi_samplerate = pwfx->nSamplesPerSec;

  return True;
}

void
ahi_out_volume(uint16 left, uint16 right)
{
//  printf( "ahi_out_volume %x %x\n", left, right );

  ahi_volume = ( left + right ) / 2;

//  printf( "vol %x\n", ahi_volume );
  
  if( ahi_volume != 0 )
  {
    ahi_pan = ( right - left ) * 0x8000 / ahi_volume + 0x8000;
  }
  else
  {
    ahi_pan = 0x8000;
  }

  if( ahi_volume == 65535 )
  {
    ++ahi_volume;
  }
}    


void
ahi_out_write(STREAM s, uint16 tick, uint8 index)
{
  struct audio_packet *packet = &packet_queue[queue_hi];
  unsigned int next_hi = (queue_hi + 1) % MAX_QUEUE;

//  printf( "ahi_out_write\n" );
  
  if (next_hi == queue_lo)
  {
    error("No space to queue audio packet\n");
    return;
  }

  queue_hi = next_hi;

  packet->s = *s;
  packet->tick = tick;
  packet->index = index;
  packet->s.p += 4;

  /* we steal the data buffer from s, give it a new one */
  s->data = malloc(s->size);
  
  ahi_out_play();
}


void
ahi_out_play(void)
{
  struct audio_packet *packet;
  unsigned int i;
  STREAM out;
  struct AHIRequest* tmp_io;
  APTR tmp_data;

//  printf( "ahi_out_play\n" );
  
  while (1)
  {
    if( ahi_io_data[ 0 ] != NULL )
    {
//      printf( "completing io %08x\n", ahi_io[ 0 ] );
      
      if( CheckIO( (struct IORequest*) ahi_io[ 0 ] ) == NULL )
      {
//	printf( "io %08x still in progress\n", ahi_io[ 0 ] );
	return;
      }
      else
      {
	LONG err = WaitIO( (struct IORequest*) ahi_io[ 0 ] );

//	printf( "ioreq %08x (data=%08x) finished\n", ahi_io[0], ahi_io_data[0] );
	
	free( ahi_io_data[ 0 ] );
	ahi_io_data[ 0 ] = NULL;

	if( err != 0 )
	{
	  error( "IORequest %08x failed with error code %d\n", ahi_io[ 0 ], err );
	  return;
	}
      }
    }

//    printf( "status: %08x, %08x; %08x, %08x\n", ahi_io[ 0 ], ahi_io[ 1 ], ahi_io_data[ 0 ], ahi_io_data[ 1 ] );
    
    if (queue_lo == queue_hi)
    {
//      printf( "no more packets queued\n" );
      return;
    }

    packet = &packet_queue[queue_lo];
    out = &packet->s;
    queue_lo = (queue_lo + 1) % MAX_QUEUE;

//    printf( "picked up %d %d %08lx\n", packet->tick, packet->index, out->data );

    /* Swap the current packet */
    if (ahi_swapaudio)
    {
      uint8 swap;
      
      for (i = 0; (ssize_t) i < out->end - out->p; i += 2)
      {
	swap = *(out->p + i);
	*(out->p + i) = *(out->p + i + 1);
	*(out->p + i + 1) = swap;

//	printf( "%d ", *(WORD*)(out->p + i) );
      }
//      printf("\n");
    }

    ahi_io[ 0 ]->ahir_Std.io_Command = CMD_WRITE;
    ahi_io[ 0 ]->ahir_Std.io_Data    = out->p;
    ahi_io[ 0 ]->ahir_Std.io_Length  = out->end - out->p;
    ahi_io[ 0 ]->ahir_Std.io_Offset  = 0;
    ahi_io[ 0 ]->ahir_Frequency      = ahi_samplerate;
    ahi_io[ 0 ]->ahir_Type           = ahi_sampletype;
    ahi_io[ 0 ]->ahir_Volume         = ahi_volume;
    ahi_io[ 0 ]->ahir_Position       = ahi_pan;
    ahi_io[ 0 ]->ahir_Link           = ( ahi_io_data[ 1 ] != NULL ? ahi_io[ 1 ] : NULL );

/*     printf("started ioreq %08lx, data %08x, size: %d (link %08lx)\n", */
/* 	   ahi_io[ 0 ], out->p, out->end - out->p, ahi_io[0]->ahir_Link ); */
/*     printf( "rate: %d, vol: %x, pan: %x, type: %d\n", ahi_samplerate, ahi_volume, ahi_pan, ahi_sampletype ); */
    
    SendIO( (struct IORequest*) ahi_io[ 0 ] );
    
    rdpsnd_send_completion(packet->tick, packet->index);
//    printf( "sent ack for %d %d %08lx\n", packet->tick, packet->index, out->data );
    ahi_io_data[ 0 ] = out->data;

    tmp_io = ahi_io[ 0 ];
    ahi_io[ 0 ] = ahi_io[ 1 ];
    ahi_io[ 1 ] = tmp_io;

    tmp_data = ahi_io_data[ 0 ];
    ahi_io_data[ 0 ] = ahi_io_data[ 1 ];
    ahi_io_data[ 1 ] = tmp_data;
  }
}

struct audio_driver *
ahi_register(char *options)
{
  static struct audio_driver ahi_driver;
  memset(&ahi_driver, 0, sizeof(ahi_driver));

  ahi_driver.name = "ahi";
  ahi_driver.description =
    "AHI output driver, default device: 0";

  ahi_driver.add_fds = ahi_add_fds;
  ahi_driver.check_fds = ahi_check_fds;

  ahi_driver.wave_out_open = ahi_out_open;
  ahi_driver.wave_out_close = ahi_out_close;
  ahi_driver.wave_out_format_supported = ahi_out_format_supported;
  ahi_driver.wave_out_set_format = ahi_out_set_format;
  ahi_driver.wave_out_volume = ahi_out_volume;

  ahi_driver.need_byteswap_on_be = 0;
  ahi_driver.need_resampling = 0;

/*   if (options) */
/*   { */
/*     dsp_dev = xstrdup(options); */
/*   } */
/*   else */
/*   { */
/*     dsp_dev = getenv("AUDIODEV"); */

/*     if (dsp_dev == NULL) */
/*     { */
/*       dsp_dev = DEFAULTDEVICE; */
/*     } */
/*   } */

  return &ahi_driver;
}
