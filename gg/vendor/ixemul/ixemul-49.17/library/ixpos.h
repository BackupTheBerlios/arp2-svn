struct pOS_Library;
struct pOS_ExecBase;
struct pOS_DosBase;
struct pOS_Method;
struct pOS_Resource;
struct pOS_RawDoFmtData;
struct pOS_AsciiDoFmtData;
struct pOS_AsciiFmtData;
struct pOS_ClassGrp;
struct pOS_DosDevPathInfo;
struct pOS_ShellScript;
struct pOS_SegmentInfo;
struct IClass;
struct Isrvstr;
struct TagItem;
struct MemHeader;
struct SemaphoreMessage;
struct StackSwapStruct;
struct Interrupt;
struct StandardPacket;
struct InfoData;
struct WBStartup;

#include <pExec/Types.h>
#include <pExec/Task.h>
#include <pExec/MsgPort.h>
#include <pExec/Sema.h>
#include <pExec/Library.h>
#include <pExec/Memory.h>
#include <pExec/CallBack.h>
#include <pExec/TstTags.h>
#include <pExec/Diagnos.h>
#include <pExec/Device.h>
#include <pExec/Class.h>
#include <pExec/Interupt.h>
#include <pResource/IRQRes.h>

#include <pDos/DosTags.h>
#include <pDos/DosTypes.h>
#include <pDos/DosBase.h>
#include <pDos/DosDev.h>
#include <pDos/Date.h>
#include <pDos/DateTime.h>
#include <pDos/DosArgs.h>
#include <pDos/FIB.h>
#include <pDos/Files.h>
#include <pDos/InfoData.h>
#include <pDos/Lock.h>
#include <pDos/Notify.h>
#include <pDos/Pattern.h>
#include <pDos/Segment.h>
#include <pDos/Var.h>
#include <pDos/IOStruct.h>
#include <pDos/ScanDir.h>
#include <pDos/Process.h>
#include <pDos/Hunk.h>
#include <pDos/Shell.h>
#include <pDos/SegmInfo.h>

#undef pOS_CPP
#include <pProto/pExec2.h>
#include <pProto/pDOS2.h>

#include <pInline/pExec2.h>
#include <pDOS/Process.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <pInline/pDos2.h>

struct NOT_SUPPORTED;
struct RecordLock;
struct ExAllControl;
struct ExAllData;
struct CSource;
struct DosPacket;

#ifndef FROM_CRT0
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#endif

#undef TICKS_PER_SECOND
#define TICKS_PER_SECOND  pOS_TICKS_PER_SECOND

#ifndef FROM_CRT0

// assumes that timeout == 0
#undef  WaitForChar
#define WaitForChar(file, b)    ({ struct pOS_TimeVal tv = { 0 }; pOS_WaitForChar((void *)(file), &tv); })
#define Disable()               (pOS_ForbidIRQ())
#define Enable()                (pOS_PermitIRQ())
#define Forbid()                (pOS_ForbidSchedul())
#define Permit()                (pOS_PermitSchedul())
#define AllocMem(s, r)          ((APTR)pOS_AllocMem((s), (r)))
#define FreeMem(m, s)           (pOS_FreeMem((m),(s)))
#define FindTask(name)          ((APTR)pOS_FindTask((CHAR*)(name)))
#define SetTaskPri(t, p)        (pOS_SetTaskPriority((APTR)(t),(p)))
#define SetSignal(n, s)         (pOS_SetSignal((n),(s)))
#define SetProgramDir(lock)     ((BPTR)pOS_SetProgDir((APTR)(lock)))
#define GetProgramDir()         ((BPTR)pOS_GetProgDir())
#undef  GetProgramName
#define GetProgramName(n, s)    (pOS_GetProgramName((n), (s)))
#undef  SetProgramName
#define SetProgramName(name)    (pOS_SetProgramName(name))
#define Wait(signalSet)         (pOS_WaitSignal(signalSet))
#define Signal(task, signalSet) (pOS_SendSignal((APTR)(task),(signalSet)))
#define AllocSignal(signalNum)  (pOS_AllocSignal(signalNum))
#define FreeSignal(signalNum)   (pOS_FreeSignal(signalNum))
#define AddPort(port)           (pOS_AddPort((APTR)port))
#define RemPort(port)           (pOS_RemPort((APTR)port))
#define PutMsg(port, message)   (pOS_PutMsg((APTR)(port),(APTR)(message)))
#define GetMsg(port)            ((APTR)pOS_GetMsg((APTR)port))
#define ReplyMsg(message)       (pOS_ReplyMsg((APTR)message))
#define WaitPort(port)          ((APTR)pOS_WaitPort((APTR)port))
#define FindPort(name)          ((APTR)pOS_FindPort((CHAR*)name))
#define CloseLibrary(library)   (pOS_CloseLibrary((APTR)library))
#define OpenDevice(d, u, i, f)  (pOS_OpenDevice((CHAR*)(d),(u),(APTR)(i),(f),0))
#define CloseDevice(ioRequest)  (pOS_CloseDevice((APTR)ioRequest))
#define DoIO(ioRequest)         (pOS_DoIO((APTR)ioRequest))
#define SendIO(ioRequest)       (pOS_SendIO((APTR)ioRequest))
#define OpenLibrary(l, v)       ((APTR)pOS_OpenLibrary((CHAR*)(l),(v)))
#define InitSemaphore(sigSem)   (pOS_InitSemaphore((APTR)sigSem))
#define AttemptSemaphore(sgSem) (pOS_AttemptSemaphore((APTR)sgSem))
#define ObtainSemaphore(sigSem) (pOS_ObtainSemaphore((APTR)sigSem))
#define ReleaseSemaphore(s)     (pOS_ReleaseSemaphore((APTR)s))
#define AddSemaphore(sigSem)    (pOS_AddSemaphore((APTR)sigSem))
#define RemSemaphore(sigSem)    (pOS_RemSemaphore((APTR)sigSem))
#define CopyMemQuick(s, d, sz)  (pOS_CopyMem((APTR)(s),(d),(sz)))
#define CacheClearU()           (pOS_CacheClearU())
#define CreateMsgPort()         ((APTR)pOS_CreatePort(0,0))
#define DeleteMsgPort(port)     (pOS_DeletePort((APTR)port))
#define StackSwap(newStack)     (pOS_StackSwap((APTR)newStack))
#define AllocEntry(entry)       ((APTR)pOS_AllocEntry(NULL,(APTR)entry))
#define AddTask(task, i, f)     ((APTR)pOS_AddTask((APTR)(task),(i),(f)))
#define RemTask(task)           (pOS_RemTask((APTR)task))
#define CheckIO(ioRequest)      ((APTR)pOS_CheckIO((APTR)ioRequest))
#define WaitIO(ioRequest)       (pOS_WaitIO((APTR)ioRequest))
#define AbortIO(ioRequest)      (pOS_AbortIO((APTR)ioRequest))
#define CopyMem(s, d, sz)       (pOS_CopyMem((APTR)(s),(d),(sz)))
#define Open(name, accessMode)  ((BPTR)pOS_OpenFile(NULL,(CHAR*)(name),(accessMode)))
#define Close(file)             ({pOS_CloseFile((APTR)file); 0;})
#define Read(file, b, l)        (pOS_ReadFile((APTR)(file),(b),(l)))
#define Write(file, b, l)       (pOS_WriteFile((APTR)(file),(const APTR)(b),(l)))
#define Input()                 ((BPTR)pOS_GetStdInput())
#define Output()                ((BPTR)pOS_GetStdOutput())
#define Seek(file, p, o)        (pOS_SeekFile((APTR)(file),(p),(o)))
#define DeleteFile(name)        (pOS_DeleteObjectName(NULL,(CHAR*)name))
#define Rename(o, n)            (pOS_RenameObjectName(NULL,(CHAR*)(o),(CHAR*)(n)))
#define Lock(name, type)        ((BPTR)pOS_LockObject(NULL,(CHAR*)(name),(type)))
#define UnLock(lock)            (pOS_UnlockObject((APTR)lock))
#define DupLock(lock)           ((BPTR)pOS_DupObjectLock((APTR)lock))
#define Info(lock, p)           (pOS_GetDosInfoData((APTR)(lock),(APTR)(p)))
#define CreateDir(name)         ((BPTR)pOS_CreateDirectory(NULL,(CHAR*)name))
#define CurrentDir(lock)        ((BPTR)pOS_SetCurrentDirLock((APTR)lock))
#define IoErr()                 (pOS_GetIoErr())
#define LoadSeg(name)           ((BPTR)pOS_LoadSegmentA((CHAR*)name,NULL))
#define UnLoadSeg(seglist)      ({pOS_UnloadSegment((APTR)seglist); 1; })
#define SetComment(name, c)     (pOS_SetObjectCommentName(NULL,(CHAR*)(name),(CHAR*)(c)))
#define SetProtection(name, p)  (pOS_SetObjectProtectionName(NULL,(CHAR*)(name),(p)))
#define DateStamp(date)         ((APTR)pOS_GetDateStamp((APTR)date))
#define Delay(timeout)          (pOS_DosDelay(timeout))
#define ParentDir(lock)         ((BPTR)pOS_ParentObjectDir((APTR)lock))
#define IsInteractive(file)     (pOS_IsFileInteractive((APTR)file))
#define SelectInput(fh)         ((BPTR)pOS_SetStdInput((struct pOS_FileHandle*)fh))
#define SelectOutput(fh)        ((BPTR)pOS_SetStdOutput((struct pOS_FileHandle*)fh))
#define OpenFromLock(lock)      ((BPTR)pOS_OpenFileFromLock((APTR)lock))
#define ExamineFH(fh, fib)      (pOS_ExamineFH((APTR)(fh),(APTR)(fib)))
#define SetFileDate(name, date) (pOS_SetObjectDateName(NULL,(CHAR*)(name),(APTR)(date)))
#define NameFromLock(l, b, len) (pOS_NameFromObjectLock((APTR)(l),(CHAR*)(b),(len)))
#define NameFromFH(fh, b, len)  (pOS_NameFromFH((APTR)(fh),(CHAR*)(b),(len)))
#define SameLock(lock1, lock2)  (pOS_SameDosObject((APTR)(lock1),(APTR)(lock2)))
#define SetMode(fh, mode)       (pOS_SetDosScreenMode((APTR)(fh),(mode)))
#define SetIoErr(result)        (pOS_SetIoErr(result))
#define CreateNewProc(tags)     ((APTR)pOS_CreateProcessA((APTR)tags))
#define SetCurrentDirName(name) (pOS_SetCurrentDirName((CHAR*)name))
#define GetCurrentDirName(b, l) (pOS_GetCurrentDirName((CHAR*)(b),(l)))
#define IsFileSystem(name)      (pOS_IsFileSystemName(NULL,(CHAR*)name))
#define SetVar(name, b, s, f)   (pOS_SetVar((CHAR*)(name),(UBYTE*)(b),(s),(f),NULL))
#define GetVar(name, b, s, f)   (pOS_GetVar((CHAR*)(name),(UBYTE*)(b),(s),(f),NULL))
#define DeleteVar(name, flags)  (pOS_DeleteVar((CHAR*)(name),(flags),NULL))
#define FindVar(name, type)     ((APTR)pOS_FindVar((CHAR*)(name),(type),NULL))
#define SameDevice(l1, l2)      (pOS_SameDosDevice((APTR)(l1),(APTR)(l2)))
#define AddPart(d, f, s)        (pOS_AddPart((CHAR*)(d),(CHAR*)(f),(s)))
#define UnGetC(fh, ch)          (pOS_FileUnGetC((APTR)(fh),(ch)))
#define FGetC(fh)               (pOS_FileGetC((APTR)fh))
#define Flush(fh)               (pOS_FileFlush((APTR)fh))
#define AddHead(list, node)     (pOS_ListAddHead((struct pOS_List*)(list),(APTR)(node)))
#define AddTail(list, node)     (pOS_ListAddTail((struct pOS_List*)(list),(APTR)(node)))
#define Remove(node)            (pOS_ListRemove((struct pOS_Node*)node))
#define Supervisor(f)           (pOS_Supervisor(f))

#endif  /* !FROM_CRT0 */
