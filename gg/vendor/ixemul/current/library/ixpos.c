
#define _KERNEL
#include "ixemul.h"
#ifdef __pos__
#include <pProto/pList.h>
#endif

ino_t get_unique_id(BPTR lock, void *fh)
{
#ifdef __pos__
  struct pOS_DosIOReq IOReq;

  if (lock)
    pOS_InitDosIOReq(((struct pOS_FileLock *)lock)->fl_DosDev, &IOReq);
  else
    pOS_InitDosIOReq(((struct pOS_FileHandle *)fh)->fh_DosDev, &IOReq);
  IOReq.dr_Command = DOSCMD_GetUniqueID;
  IOReq.dr_U.dr_GetUniqueID.drgq_Lock = (void *)lock;
  IOReq.dr_U.dr_GetUniqueID.drgq_FH = fh;
  pOS_DoIO((struct pOS_IORequest*)&IOReq);
  if (IOReq.dr_Error2 == 0)
    return (ino_t)IOReq.dr_U.dr_GetUniqueID.drgq_Result;
  return (ino_t)(lock ? (ino_t)lock : (ino_t)fh);
#else
  ino_t result;

  if (fh)
    lock = DupLockFromFH(CTOBPTR(fh));
  if (lock)
    /* The fl_Key field of the FileLock structure is always unique. This is
       also true for filesystems like AFS and AFSFloppy. Thanks to Michiel
       Pelt (the AFS author) for helping me with this. */
    result = (ino_t)((struct FileLock *)(BTOCPTR(lock)))->fl_Key;
  else
    result = (ino_t)fh;
  if (fh)
    UnLock(lock);
  return result;
#endif
}

#ifdef __pos__

LONG Examine( BPTR lock, struct FileInfoBlock *fileInfoBlock )
  {
    SLONG Res=pOS_ExamineObject((APTR)lock,(APTR)fileInfoBlock);
    if(fileInfoBlock->fib_DirEntryType & FINFENTYP_File)
	 fileInfoBlock->fib_DirEntryType |=  0x80;
    else fileInfoBlock->fib_DirEntryType &= ~0x80;
    return(Res);
  }

LONG ExNext( BPTR lock, struct FileInfoBlock *fileInfoBlock )
  {
    SLONG Res=pOS_ExNextObject((APTR)lock,(APTR)fileInfoBlock);
    if(fileInfoBlock->fib_DirEntryType & FINFENTYP_File)
	 fileInfoBlock->fib_DirEntryType |=  0x80;
    else fileInfoBlock->fib_DirEntryType &= ~0x80;
    return(Res);
  }

#endif
