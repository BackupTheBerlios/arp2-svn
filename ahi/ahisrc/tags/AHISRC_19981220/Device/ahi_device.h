/* :ts=8 */

/* $Id$
* $Log$
* Revision 4.4  1997/12/21 17:41:50  lcs
* Major source cleanup, moved some functions to separate files.
*
* Revision 4.3  1997/05/03 19:59:56  lcs
* Fixed a race condition (happened with small CMD_WRITE's).
*
*/

/*** New Style Device definitions ***/

#define NSCMD_DEVICEQUERY   0x4000

#define NSDEVTYPE_UNKNOWN       0   /* No suitable category, anything */
#define NSDEVTYPE_GAMEPORT      1   /* like gameport.device */
#define NSDEVTYPE_TIMER         2   /* like timer.device */
#define NSDEVTYPE_KEYBOARD      3   /* like keyboard.device */
#define NSDEVTYPE_INPUT         4   /* like input.device */
#define NSDEVTYPE_TRACKDISK     5   /* like trackdisk.device */
#define NSDEVTYPE_CONSOLE       6   /* like console.device */
#define NSDEVTYPE_SANA2         7   /* A >=SANA2R2 networking device */
#define NSDEVTYPE_AUDIOARD      8   /* like audio.device */
#define NSDEVTYPE_CLIPBOARD     9   /* like clipboard.device */
#define NSDEVTYPE_PRINTER       10  /* like printer.device */
#define NSDEVTYPE_SERIAL        11  /* like serial.device */
#define NSDEVTYPE_PARALLEL      12  /* like parallel.device */

struct NSDeviceQueryResult
{
  /*
  ** Standard information
  */
  ULONG   DevQueryFormat;         /* this is type 0               */
  ULONG   SizeAvailable;          /* bytes available              */

  /*
  ** Common information (READ ONLY!)
  */
  UWORD   DeviceType;             /* what the device does         */
  UWORD   DeviceSubType;          /* depends on the main type     */
  UWORD   *SupportedCommands;     /* 0 terminated list of cmd's   */

  /* May be extended in the future! Check SizeAvailable! */
};

#define DRIVE_NEWSTYLE  (0x4E535459L)   /* 'NSTY' */
#define NSCMD_TD_READ64     0xc000
#define NSCMD_TD_WRITE64    0xc001
#define NSCMD_TD_SEEK64     0xc002
#define NSCMD_TD_FORMAT64   0xc003


/*** My own stuff ***/

#define AHI_PRI		50	/* Priority for the device process */

#define PLAYERFREQ	100	/* How often the PlayerFunc is called */

#define AHICMD_END	CMD_NONSTD

#define AHICMD_WRITTEN	(0x8000 | CMD_WRITE)

#define ahir_Extras	ahir_Private[0]
#define GetExtras(req)	((struct Extras *) req->ahir_Private[0])
#define NOCHANNEL	65535

struct Extras
{
	UWORD	Channel;
	UWORD	Sound;
	LONG	BuffSamples;
	LONG	Count;
};

/* Voice->Flags definitions */

/* Set by the interrupt when a new sound has been started */
#define VB_STARTED		0
#define VF_STARTED		(1<<0)

struct Voice
{
	UWORD			 NextSound;
	UBYTE			 Flags;
	UBYTE			 Pad;
	Fixed			 NextVolume;
	Fixed			 NextPan;
	ULONG			 NextFrequency;
	ULONG			 NextOffset;
	ULONG			 NextLength;
	struct AHIRequest	*NextRequest;
	
	struct AHIRequest	*QueuedRequest;
	struct AHIRequest	*PlayingRequest;
};

/* Special Offset values */

#define FREE	-1	/* Channel is not playing anything */
#define	MUTE	-2	/* Channel will be muted when current sound is finished */
#define PLAY	-3	/* Channel will play more when current sound is finished */

#define	MAXSOUNDS	128
#define SOUND_FREE	0
#define SOUND_IN_USE	1

struct AHIDevUnit
{
	struct Unit		 Unit;
	UBYTE			 UnitNum;
	BYTE			 PlaySignal;
	BYTE			 RecordSignal;
	BYTE			 SampleSignal;
	struct Process		*Process;
	struct Process		*Master;
	struct Hook		 PlayerHook;
	struct Hook 		 RecordHook;
	struct Hook 		 SoundHook;
	struct Hook		 ChannelInfoHook;
	
	struct AHIEffChannelInfo *ChannelInfoStruct;

	WORD			 RecordOffDelay;
	WORD			*RecordBuffer;
	ULONG			 RecordSize;

	BOOL			 IsPlaying;	// Currently playing (or want to)
	BOOL			 IsRecording;	// Currently recording
	BOOL			 ValidRecord;	// The record buffer contains valid data
	BOOL			 FullDuplex;	// Mode is full duplex
	UWORD			 StopCnt;	// CMD_STOP count

	struct SignalSemaphore	 ListLock;
	struct MinList		 ReadList;
	struct MinList		 PlayingList;
	struct MinList		 SilentList;
	struct MinList		 WaitingList;

	struct MinList		 RequestQueue;	// Not locked by ListLock!

	struct Voice		*Voices;

	struct AHIAudioCtrl	*AudioCtrl;
	ULONG			 AudioMode;
	ULONG			 Frequency;
	UWORD			 Channels;
	UWORD			 Pad2;
	Fixed			 MonitorVolume;
	Fixed			 InputGain;
	Fixed			 OutputVolume;
	ULONG			 Input;
	ULONG			 Output;

	UBYTE			 Sounds[MAXSOUNDS];
};

/*
** Message passed to the Unit Process at
** startup time.
*/
struct StartupMessage
{
	struct Message		 Msg;
	struct AHIDevUnit	*Unit;
};
