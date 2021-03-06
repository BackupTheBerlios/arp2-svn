#ifndef AHI_Drivers_AROS_DriverData_h
#define AHI_Drivers_AROS_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/oss.h>

#include "DriverBase.h"

struct AROSBase
{
    struct DriverBase driverbase;
    struct Library*   dosbase;
    struct Library*   ossbase;
};

#define DRIVERBASE_SIZEOF (sizeof (struct AROSBase))

#define DOSBase  ((struct DosLibrary*)    AROSBase->dosbase)
#define OSSBase	 (AROSBase->ossbase)

struct AROSData
{
    struct DriverData   driverdata;
    UBYTE		flags;
    UBYTE		pad1;
    BYTE		mastersignal;
    BYTE		slavesignal;
    struct Process*	mastertask;
    struct Process*	slavetask;
    struct AROSBase*	ahisubbase;
    APTR		mixbuffer;
};


#endif /* AHI_Drivers_AROS_DriverData_h */
