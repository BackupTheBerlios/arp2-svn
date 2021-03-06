/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  __ioerr_to_errno.c,v 1.1.1.1 1994/04/04 04:30:09 amiga Exp
 *
 *  __ioerr_to_errno.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:09  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <errno.h>

int __ioerr_to_errno(int ioerr)
{
  int err;

  switch (ioerr)
    {
#ifdef __pos__
      case DOSERR_Unknown:                      err = EIO;              break;
      case DOSERR_Aborted:                      err = EIO;              break;
      case DOSERR_NoMem:                        err = ENOMEM;           break;
      case DOSERR_BadTemplate1:                 err = EINVAL;           break;
      case DOSERR_BadTemplate2:                 err = EINVAL;           break;
      case DOSERR_BadTemplateLogic:             err = EINVAL;           break;
      case DOSERR_TooManyArgs:                  err = E2BIG;            break;
      case DOSERR_RequiredArgMissing:           err = EINVAL;           break;
      case DOSERR_KeyNeedsArg:                  err = EINVAL;           break;
      case DOSERR_NeedsKeyword:                 err = EINVAL;           break;
      case DOSERR_UnknownArg:                   err = EINVAL;           break;
      case DOSERR_NameToLong:                   err = E2BIG;            break;
      case DOSERR_FileIsNotLoadModul:           err = ENOEXEC;          break;
      case DOSERR_ObjectInUse:                  err = EEXIST;           break;
      case DOSERR_ObjectExists:                 err = EEXIST;           break;
      case DOSERR_DirNotFound:                  err = ENOENT;           break;
      case DOSERR_ObjectNotFound:               err = ENOENT;           break;
      case DOSERR_NotImplemented:               err = ENOSYS;           break;
      case DOSERR_WrongObjectType:              err = ENOTDIR;          break;
      case DOSERR_DiskNotValidated:             err = EIO;              break;
      case DOSERR_DiskWriteProtected:           err = EROFS;            break;
      case DOSERR_RenameAcrossDevices:          err = EXDEV;            break;
      case DOSERR_DirectoryNotEmpty:            err = ENOTEMPTY;        break;
      case DOSERR_DeviceNotMounted:             err = ENXIO;            break;
      case DOSERR_SeekFailure:                  err = ESPIPE;           break;
      case DOSERR_CommentTooLong:               err = ENAMETOOLONG;     break;
      case DOSERR_DiskFull:                     err = ENOSPC;           break;
      case DOSERR_DeleteProtected:              err = EACCES;           break;
      case DOSERR_WriteProtected:               err = EACCES;           break;
      case DOSERR_ReadProtected:                err = EACCES;           break;
      case DOSERR_NotDosDisk:                   err = ENXIO;            break;
      case DOSERR_NoDiskInDrive:                err = ENXIO;            break;
      case DOSERR_NoMoreEntries:                err = ENOENT;           break;
      case DOSERR_ReadWriteError:               err = EIO;              break;
      case DOSERR_ProgAborted:                  err = EINTR;            break;
      case DOSERR_NoFileSystem:                 err = ENXIO;            break;
      case DOSERR_NoProcessNum:                 err = EPROCLIM;         break;
      case DOSERR_SyntaxError:                  err = EINVAL;           break;
      case DOSERR_TwiceIOE:                     err = EMFILE;           break;
      case DOSERR_ArgLineTooLong:               err = ENAMETOOLONG;     break;
      case DOSERR_NoDupFHWithBuffer:            err = EIO;              break;
      case DOSERR_UnknownHunk:                  err = EFTYPE;           break;
      case DOSERR_TwiceMounted:                 err = EMFILE;           break;
      case DOSERR_Timeout:                      err = EINTR;            break;
      case DOSERR_NoChips:                      err = ENXIO;            break;
      case DOSERR_NoGfxMem:                     err = ENXIO;            break;
      case DOSERR_UnknownMode:                  err = ENXIO;            break;
      case DOSERR_NotAvailable:                 err = ENXIO;            break;
      case DOSERR_NoScreen:                     err = ENXIO;            break;
      case DOSERR_NoDisplay:                    err = ENXIO;            break;
      case DOSERR_FalseWH:                      err = EINVAL;           break;
      case DOSERR_NoAccess:                     err = EACCES;           break;
#else
      case ERROR_NO_FREE_STORE:                 err = ENOMEM;           break;
      case ERROR_TASK_TABLE_FULL:               err = EAGAIN;           break;
      case ERROR_LINE_TOO_LONG:                 err = E2BIG;            break;
      case ERROR_INVALID_RESIDENT_LIBRARY:      err = ENOEXEC;          break;
      case ERROR_FILE_NOT_OBJECT:               err = ENOEXEC;          break;
      case ERROR_OBJECT_IN_USE:                 err = EEXIST;           break;
      case ERROR_OBJECT_EXISTS:                 err = EEXIST;           break;
      case ERROR_BAD_STREAM_NAME:               err = ENOENT;           break;
      case ERROR_DIR_NOT_FOUND:                 err = ENOENT;           break;
      case ERROR_OBJECT_NOT_FOUND:              err = ENOENT;           break;
      case ERROR_ACTION_NOT_KNOWN:              err = ENODEV;           break;
      case ERROR_NO_DEFAULT_DIR:                err = ENOTDIR;          break;
      case ERROR_OBJECT_TOO_LARGE:              err = ENOMEM;           break;
      case ERROR_INVALID_COMPONENT_NAME:        err = EIO;              break;
      case ERROR_INVALID_LOCK:                  err = ENODEV;           break;
      case ERROR_TOO_MANY_LEVELS:               err = ELOOP;            break;
      case ERROR_OBJECT_WRONG_TYPE:             err = ENOTDIR;          break;
      case ERROR_DISK_NOT_VALIDATED:            err = EIO;              break;
      case ERROR_DISK_WRITE_PROTECTED:          err = EROFS;            break;
      case ERROR_RENAME_ACROSS_DEVICES:         err = EXDEV;            break;
      case ERROR_DIRECTORY_NOT_EMPTY:           err = ENOTEMPTY;        break;
      case ERROR_DEVICE_NOT_MOUNTED:            err = ENXIO;            break;
      case ERROR_SEEK_ERROR:                    err = ESPIPE;           break;
      case ERROR_COMMENT_TOO_BIG:               err = ENAMETOOLONG;     break;
      case ERROR_DISK_FULL:                     err = ENOSPC;           break;
      case ERROR_DELETE_PROTECTED:              err = EACCES;           break;
      case ERROR_WRITE_PROTECTED:               err = EACCES;           break;
      case ERROR_READ_PROTECTED:                err = EACCES;           break;
      case ERROR_NOT_A_DOS_DISK:                err = ENXIO;            break;
      case ERROR_NO_DISK:                       err = ENXIO;            break;
      case ERROR_NO_MORE_ENTRIES:               err = ENOENT;           break;
#endif
      /* catch-all for illegal accesses to NIL: */
      case 4242:                                err = EPERM;            break;
      /* catch-all for illegal accesses to /dev/[pt]tyXX */
      case 5252:                                err = EPERM;            break;
      /* catch-all for illegal accesses to / */
      case 6262:                                err = EPERM;            break;
      default:                                  err = EIO;              break;
    }
  KPRINTF (("ioerr = %ld, err = %ld\n", ioerr, err));
  return err;
}
