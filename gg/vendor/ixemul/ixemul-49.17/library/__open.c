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
#include "kprintf.h"

#include <utility/tagitem.h>
#include <dos/dostags.h>

#if 0

struct open_vec {
  int action;
  struct FileHandle *fh;
};

static int
__open_func (struct lockinfo *info, struct open_vec *ov, int *error)
{
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

  /* continue if we failed because of symlink - reference */
  return 1;
}

BPTR
__open (char *name, int action)
{
  unsigned long *fh;
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
      oldwin = this_proc->pr_WindowPtr;
      this_proc->pr_WindowPtr = (APTR)-1;
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

  /* needed to get berserk ReadArgs() to behave sane, GRRR */
  fh = AllocDosObjectTags (DOS_FILEHANDLE, ADO_FH_Mode, action, TAG_DONE);
  if (! fh)
    return 0;
  ov.fh = (struct FileHandle *) fh;
  ov.action = action;

  res = __plock (name, __open_func, &ov);

  /* check for opening of NIL:, in which case we just return
     a filehandle with a zero handle field */
  if (!res && IoErr() == 4242)
  {
    res = -1;
  }

  if (!res)
    FreeDosObject (DOS_FILEHANDLE, fh);

  return res ? CTOBPTR(ov.fh) : 0;
}
#else

char *
ix_to_ados(char *buf, const char *name)
{
  usetup;
  char *p = buf;
  const char *q = name;
  int parent_count = 0;

  KPRINTF(("name \"%s\"\n", name));

  if (u.u_is_root)
    {
      *p++ = '/';
    }

  while (*q)
    {
      if (*q == '/')
	{
	  if (p > buf && p[-1] == '/')
	    {
	      --p;
	    }
	  else
	    {
	      p = buf;
	      parent_count = 0;
	    }
	}
      else if (*q == '.' && (q[1] == '/' || q[1] == '\0'))
	{
	  ++q;
	  if (*q)
	    ++q;
	  continue;
	}
      else if (*q == '.' && q[1] == '.' && (q[2] == '/' || q[2] == '\0'))
	{
	  q += 2;

	  if (p == buf)
	    {
	      ++parent_count;
	      if (*q)
		++q;
	      continue;
	    }
	  else if (p == buf + 1 && *buf == '/')
	    {
	      p = buf;
	      parent_count = 0;
	    }
	  else
	    {
	      do
		{
		  --p;
		}
	      while (p > buf && p[-1] != '/');
	      if (p > buf + 1)
		--p;
	      else
		{
		  while (*q == '/')
		    {
		      ++q;
		    }
		}
	    }
	}
      else
	{
	  const char *start = q;

	  do
	    {
	      *p++ = *q++;
	    }
	  while (*q && *q != '/' && *q != ':');

	  if (*q == ':')
	    {
	      p = buf;
	      *p++ = '/';
	      while (start != q)
		*p++ = *start++;
	    }
	}

      if (*q == '/' || *q == ':')
        {
	  *p++ = '/';
	  ++q;
	}
    }

  if (p == buf + 1 && buf[0] == '/')
    {
      buf[0] = ':';
      *p = '\0';
    }
  else
    {
      if (p > buf && p[-1] == '/')
	--p;

      *p++ = '\0';

      if (parent_count)
        {
	  while (p != buf)
	    {
	      --p;
	      p[parent_count] = *p;
	    }
	  do
	    {
	      *p++ = '/';
	    }
	  while (--parent_count);
	}
      else if (*buf == '/')
        {
	  ++buf;
	  p = buf;
	  while (*p != '/' && *p)
	    ++p;
	  if (!*p)
	    p[1] = '\0';
	  *p = ':';
	}
    }

  KPRINTF(("result \"%s\"\n", buf));

  if (!strcasecmp(buf, "console:"))
  {
    buf[0] = '*';
    buf[1] = '\0';
  }
  else if (!strncmp(buf, "dev:", 4))
  {
    if (!strcmp(buf + 4, "tty"))
    {
      buf[0] = '*';
      buf[1] = '\0';
    }
    else if (!strcmp(buf + 4, "null"))
    {
      strcpy(buf, "NIL:");
    }
    else if (!strcmp(buf + 4, "random") || !strcmp(buf + 4, "urandom") || !strcmp(buf + 4, "srandom") || !strcmp(buf + 4, "prandom"))
    {
      strcpy(buf, "RANDOM:");
    }
  }

   return buf;
}

char *check_root(char *name)
{
//char buf[256];
//dprintf("check_root(%s)\n", name);
  if (*name == '/')
  {
    BPTR olddir = CurrentDir(0);
    BPTR dir = DupLock(olddir);
    int root = 0;
//NameFromLock(olddir, buf, sizeof(buf));
//dprintf("curdir <%s>\n", buf);

    CurrentDir(olddir);

    do
    {
      BPTR parent = ParentDir(dir);

      ++name;
//dprintf("dir %p parent %p name <%s>\n", dir, parent, name);
      if (parent == 0)
      {
	root = 1;
	break;
      }

      UnLock(dir);
      dir = parent;
    }
    while (*name == '/');

    UnLock(dir);

    while (*name == '/')
      ++name;

    if (!root)
    {
      name = NULL;
    }
    else if (*name)
    {
      char *p = name;
      do
      {
	++p;
      }
      while (*p && *p != '/');

      if (*p)
      {
	*p = ':';
      }
      else
      {
	p[0] = ':';
	p[1] = '\0';
      }
    }
  }
  else
  {
    name = NULL;
  }
//dprintf("return(%s)\n",name ?name: "NULL");
  return name;
}

BPTR
__open (char *name, int action)
{
  BPTR file;
  APTR win;
  char *buf = alloca(strlen(name) + 4);
  buf = ix_to_ados(buf, name);
  win = ((struct Process *)SysBase->ThisTask)->pr_WindowPtr;
  ((struct Process *)SysBase->ThisTask)->pr_WindowPtr = (APTR)-1;
  file = Open(buf, action);
  if (!file)
  {
    LONG err = IoErr();
    if (err == ERROR_OBJECT_NOT_FOUND)
    {
      buf = check_root(buf);
      if (buf && *buf)
      {
	file = Open(buf, action);
	if (file)
	  err = 0;
      }
      SetIoErr(err);
    }
  }
  ((struct Process *)SysBase->ThisTask)->pr_WindowPtr = win;
  return file;
}

#endif
