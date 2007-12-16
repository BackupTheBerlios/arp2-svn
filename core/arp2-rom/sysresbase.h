#ifndef arp2_rom_sysresbase_h
#define arp2_rom_sysresbase_h

#include <exec/libraries.h>
#include <exec/interrupts.h>

struct SysResBase {
    struct Library   lib;
    UWORD            pad;
    
    // The following variable is used within the syscalls to find the
    // current task/process and set Result2 (like SetIOErr()). Its
    // offset must not change.
    struct ExecBase* sysbase;
    
    struct Interrupt exter_int;
};

#endif /* arp2_rom_sysresbase_h */
