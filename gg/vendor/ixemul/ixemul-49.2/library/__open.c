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
#include <string.h>
#include "ixemul.h"

#include <utility/tagitem.h>
#include <dos/dostags.h>

struct open_vec {
  int action;
  struct FileHandle *fh;
};

static int
__open_func (struct lockinfo *info, struct open_vec *ov, int *error)
{
#ifdef __pos__
  info->result = (BPTR)pOS_OpenFile((void *)info->parent_lock, info->bstr, ov->action);
  ov->fh = (void *)info->result;
  *error = info->result == 0;
#else
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ov->action;
  sp->sp_Pkt.dp_Arg1 = CTOBPTR (ov->fh);
  sp->sp_Pkt.dp_Arg2 = info->parent_lock;
  sp->sp_Pkt.dp_Arg3 = info->bstr;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  
  info->result = sp->sp_Pkt.dp_Res1;
  *error = info->result != -1;

  /* handler is supposed to supply this, but if it didn't... */
  if (!*error && !ov->fh->fh_Type) ov->fh->fh_Type = info->handler;
#endif
 
  /* continue if we failed because of symlink - reference */
  return 1;
}

BPTR
__open (char *name, int action)
{
#ifndef __pos__
  unsigned long *fh;
#endif
  struct open_vec ov;
  BPTR res;
  struct Process *this_proc = NULL;
  APTR oldwin = NULL;
  char buf[32], *s, *p = buf;

  if ((name[0] == '/' && strncmp(name, "/dev/", 5)) ||
      index(name, ':'))
  {
    s = (name[0] == '/') ? name + 1 : name;
    while (*s && *s != '/' && *s != ':')
      *p++ = *s++;
    *p++ = ':';
    *p = '\0';
    if (ix.ix_flags & ix_no_insert_disk_requester)
    {
      this_proc = (struct Process *)SysBase->ThisTask;
#ifdef __pos__
      oldwin = (APTR)((struct pOS_Process *)this_proc)->pr_ReqWindow;
      ((struct pOS_Process *)this_proc)->pr_ReqWindow = (APTR)-1;
#else
      oldwin = this_proc->pr_WindowPtr;
      this_proc->pr_WindowPtr = (APTR)-1;
#endif
    }
    if (!IsFileSystem(buf))     /* if NAME is not a file system but */
				/* something like a pipe or console, then */
				/* do NOT use __plock() but call Open directly */
    {
      if (!*s)
	res = Open(buf, action);
      else if (*s == '/')
      {
	*s = ':';
	res = Open(name + 1, action);
	*s = '/';
      }
      else
	res = Open(name, action);
      if (ix.ix_flags & ix_no_insert_disk_requester)
	this_proc->pr_WindowPtr = oldwin;
      return res;
    }
    if (ix.ix_flags & ix_no_insert_disk_requester)
      this_proc->pr_WindowPtr = oldwin;
  }

#ifndef __pos__  
  /* needed to get berserk ReadArgs() to behave sane, GRRR */
  fh = AllocDosObjectTags (DOS_FILEHANDLE, ADO_FH_Mode, action, TAG_DONE);
  if (! fh)
    return 0;
  ov.fh = (struct FileHandle *) fh;
#endif
  ov.action = action;

  res = __plock (name, __open_func, &ov);

  /* check for opening of NIL:, in which case we just return
     a filehandle with a zero handle field */
  if (!res && IoErr() == 4242)
  {
    res = -1;
#ifdef __pos__
    ov.fh = kmalloc(sizeof(struct pOS_FileHandle));
    ov.fh->fh_DosDev = 0;
#endif
  }

#ifndef __pos__  
  if (!res)
    FreeDosObject (DOS_FILEHANDLE, fh);
#endif

  return res ? CTOBPTR(ov.fh) : 0;
}
