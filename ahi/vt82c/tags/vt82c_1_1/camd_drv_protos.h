#ifndef AHI_Drivers_Common_camd_drv_protos_h
#define AHI_Drivers_Common_camd_drv_protos_h

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#include <midi/camddevices.h>

BOOL Init( APTR SysBase );

VOID Expunge( ULONG dummy );

struct MidiPortData* OpenPort( struct MidiDeviceData *data,
			       LONG portnum,
			       APTR transmitfunc,
			       APTR  recievefunc,
			       APTR userdata );

VOID ClosePort( struct MidiDeviceData *data,
		LONG portnum );

VOID ActivateXmit( APTR  userdata,
		   ULONG portnum );

ULONG Interrupt( APTR data );

#endif /* AHI_Drivers_Common_camd_drv_protos_h */
