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
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include "multiuser.h"

/* not static because also used in nodspecial.c */
int
__chmod_func (struct lockinfo *info, int mask, int *error)
{
#ifdef __pos__
  info->result = pOS_SetObjectProtectionName((void *)info->parent_lock, info->bstr, mask);
  *error = info->result == 0;
#else
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_SET_PROTECT;
  sp->sp_Pkt.dp_Arg1 = 0;
  sp->sp_Pkt.dp_Arg2 = info->parent_lock;
  sp->sp_Pkt.dp_Arg3 = info->bstr;
  sp->sp_Pkt.dp_Arg4 = mask;
  
  PutPacket(info->handler, sp);
  __wait_sync_packet(sp);
  
  info->result = sp->sp_Pkt.dp_Res1;
  *error = info->result != -1;
#endif
  return 1;
}


int
chmod(const char *name, mode_t mode)
{
  int result;
  usetup;
  long amiga_mode = 0;

  if (mode & S_IXUSR) amiga_mode |= FIBF_EXECUTE;
  if (mode & S_IWUSR) amiga_mode |= (FIBF_WRITE|FIBF_DELETE);
  if (mode & S_IRUSR) amiga_mode |= FIBF_READ;
#ifdef FIBF_GRP_EXECUTE
  if (mode & S_IXGRP) amiga_mode |= FIBF_GRP_EXECUTE;
  if (mode & S_IWGRP) amiga_mode |= FIBF_GRP_WRITE|FIBF_GRP_DELETE;
  if (mode & S_IRGRP) amiga_mode |= FIBF_GRP_READ;
  if (mode & S_IXOTH) amiga_mode |= FIBF_OTR_EXECUTE;
  if (mode & S_IWOTH) amiga_mode |= FIBF_OTR_WRITE|FIBF_OTR_DELETE;
  if (mode & S_IROTH) amiga_mode |= FIBF_OTR_READ;
#endif
  if (mode & S_ISTXT) amiga_mode |= FIBF_HOLD;
  if (mode & S_ISUID) amiga_mode |= muFIBF_SET_UID;
  if (mode & S_ISGID) amiga_mode |= muFIBF_SET_GID;

#ifndef __pos__
  /* RWED-permissions are "lo-active", if cleared they allow the operation */
  amiga_mode ^= FIBF_READ|FIBF_WRITE|FIBF_DELETE|FIBF_EXECUTE;
#endif

  result = __plock (name, __chmod_func, (void *)amiga_mode);
  if (result == 0)
    {
      errno = __ioerr_to_errno (IoErr());
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }
  else
    {
      struct file *fp;

      /* we walk thru our filetab to see, whether we have this file
       * open, and set its mode */
      ix_lock_base();
      for (fp = ix.ix_file_tab; fp < ix.ix_fileNFILE; fp++)
	if (fp->f_count && fp->f_name && !strcmp(fp->f_name, name))
	  {
	    fp->f_stb.st_mode = mode;
	    fp->f_stb_dirty |= FSDF_MODE;
	  }
      ix_unlock_base();
    }
  return result ? 0 : -1;
}


/* a signal proof SetProtection(), nothing more ;-) */
int
ix_chmod (char *name, int mode)
{
  int result;
  usetup;

  result = __plock (name, __chmod_func, (void *)mode) == -1 ? 0 : -1;
  if (result == 0)
    {
      errno = __ioerr_to_errno (IoErr ());
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }

  return result;
}
