/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 *  $Id: ix_sigwinch.c,v 1.2 2000/06/20 22:17:23 emm Exp $
 *
 *  $Log: ix_sigwinch.c,v $
 *  Revision 1.2  2000/06/20 22:17:23  emm
 *  First attempt at a native MorphOS ixemul
 *
 *  Revision 1.1.1.1  2000/05/07 19:38:08  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:48:27  nobody
 *  Initial import
 *
 *  Revision 1.2  1994/06/19  15:13:17  rluebbert
 *  *** empty log message ***
 *
 */
#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <string.h>

#ifdef __pos__

#include <pInline/pIntui2.h>
#define gb_IntuiBase IntuitionBase      /* reuse IntuitionBase for pOS */
#include <pDevice/IEvent.h>
#include <pDevice/Input.h>

struct pOS_Window *get_window(BPTR lock, void *fh)
{
  struct pOS_DosIOReq IOReq;

  if (lock)
    pOS_InitDosIOReq(((struct pOS_FileLock *)lock)->fl_DosDev, &IOReq);
  else
    pOS_InitDosIOReq(((struct pOS_FileHandle *)fh)->fh_DosDev, &IOReq);
  IOReq.dr_Command = DOSCMD_GetWindow;
  IOReq.dr_U.dr_GetWindow.drgw_Lock = (void *)lock;
  IOReq.dr_U.dr_GetWindow.drgw_FH = fh;
  pOS_DoIO((struct pOS_IORequest *)&IOReq);
  if (IOReq.dr_Error2 == 0)
    return IOReq.dr_U.dr_GetWindow.drgw_ResWin;
  return NULL;
}

static struct pOS_InputEvent *
sigwinch_input_handler(const struct pOS_Interrupt *intr, struct pOS_InputEvent *old_chain)
{
  struct user *user = getuser(intr->is_UserData[0]);
  struct pOS_Window *w = (struct pOS_Window *)user->u_window;
  struct pOS_InputEvent *ie;

  for (ie = old_chain; ie; ie = ie->ie_NextEvent)
      if (ie->ie_Class == IECLASS_SIZEWINDOW)
	  if (w == (struct pOS_Window *) ie->ie_EventAddress)
	    _psignal((struct Task *)intr->is_UserData[0], SIGWINCH);

  /* always return the old chain, since we don't consume or generate events */
  return old_chain;
}

void __ix_install_sigwinch (void)
{
  struct Window *w;
  struct Process *me = (struct Process *) SysBase->ThisTask;
  struct user *u_ptr = getuser(me);

  if (!pOS_IsFileInteractive(pOS_GetStdOutput()))
    return;

  w = (struct Window *)get_window(0, pOS_GetStdOutput());
  if (!w) 
    return;

  if (!(u.u_idev_req = (struct IOStdReq *)
	 ix_create_extio(u.u_sync_mp, sizeof (struct pOS_IOStdReq))));

  if (pOS_OpenDevice ("pinput.device", 0, 
		  (struct pOS_IORequest *) u.u_idev_req, 0, 0))
    {
      ix_delete_extio((struct IORequest *)u.u_idev_req);
      u.u_idev_req = 0;
      return;
    }
 
  u.u_window = w;
  u.u_idev_int.is_Code = (void *) sigwinch_input_handler;
  u.u_idev_int.is_UserData[0] = (long)me;
  u.u_idev_int.is_Node.ln_Pri = 10;     /* must be before console.device */
  u.u_idev_int.is_Node.ln_Name = "ixemul SIGWINCH handler";
  u.u_idev_req->io_Data = (APTR) &u.u_idev_int;
  u.u_idev_req->io_Command = INDCMD_AddHandler;
  pOS_DoIO ((struct pOS_IORequest *) u.u_idev_req);
}


void __ix_remove_sigwinch (void)
{
  usetup;

  if (u.u_idev_req)
    {
      u.u_idev_req->io_Data = (APTR) &u.u_idev_int;
      u.u_idev_req->io_Command = INDCMD_RemHandler;
      pOS_DoIO ((struct pOS_IORequest *) u.u_idev_req);
      pOS_CloseDevice ((struct pOS_IORequest *) u.u_idev_req);
      ix_delete_extio((struct IORequest *)u.u_idev_req);
      u.u_idev_req = 0;
    }
}

#else

#include <intuition/intuition.h>
#include <devices/input.h>
#include <devices/inputevent.h>

static struct InputEvent *sigwinch_input_handler (void)
{
#ifndef NATIVE_MORPHOS
  register struct InputEvent *_old_chain asm ("a0");
  register struct Task *_me asm ("a1");
#else
  struct InputEvent *_old_chain = (void*)REG_A0;
  struct Task *_me = (void*)REG_A1;
#endif
  register struct InputEvent *old_chain = _old_chain;
  register struct Task *me = _me;
  struct user *user = getuser(me);
  struct Window *w = user->u_window;
  struct InputEvent *ie;
  
  for (ie = old_chain; ie; ie = ie->ie_NextEvent)
      if (ie->ie_Class == IECLASS_SIZEWINDOW)
	  if (w == (struct Window *) ie->ie_EventAddress)
	    _psignal (me, SIGWINCH);

  /* always return the old chain, since we don't consume or generate events */
  return old_chain;
}

#ifdef NATIVE_MORPHOS
static struct EmulLibEntry _gate_sigwinch_input_handler = {
    TRAP_LIB,0,(void(*)())sigwinch_input_handler
};
#endif

void __ix_install_sigwinch (void)
{
  struct InfoData *info;
  struct Window *w;
  struct MsgPort *handler;
  struct Process *me = (struct Process *) SysBase->ThisTask;
  struct user *u_ptr = getuser(me);
  struct StandardPacket *sp;

  info = alloca (sizeof (struct InfoData) + 2);
  info = LONG_ALIGN (info);
  bzero (info, sizeof (struct InfoData));
  
  sp = alloca (sizeof (struct StandardPacket) + 2);
  sp = LONG_ALIGN (sp);

  handler = (struct MsgPort *) me->pr_ConsoleTask;
  if (! handler)
      return;

  __init_std_packet (sp);
  sp->sp_Pkt.dp_Port = u.u_sync_mp;
  sp->sp_Pkt.dp_Type = ACTION_DISK_INFO;
  sp->sp_Pkt.dp_Arg1 = CTOBPTR (info);

  PutPacket (handler, sp);
  __wait_sync_packet (sp);
  if (sp->sp_Pkt.dp_Res1 != -1)
      return;

  w = (struct Window *) info->id_VolumeNode;
  if (! w) 
      return;

  if (!(u.u_idev_req = (struct IOStdReq *)
	 ix_create_extio(u.u_sync_mp, sizeof (struct IOStdReq))))
    return;

  if (OpenDevice ("input.device", 0, 
		  (struct IORequest *)u.u_idev_req, 0))
    {
      ix_delete_extio((struct IORequest *)u.u_idev_req);
      u.u_idev_req = 0;
      return;
    }
 
  u.u_window = w;
#ifndef NATIVE_MORPHOS
  u.u_idev_int.is_Code = (void *) sigwinch_input_handler;
#else
  u.u_idev_int.is_Code = (void *) &_gate_sigwinch_input_handler;
#endif
  u.u_idev_int.is_Data = (void *) me;
  u.u_idev_int.is_Node.ln_Pri = 10;     /* must be before console.device */
  u.u_idev_int.is_Node.ln_Name = "ixemul SIGWINCH handler";
  u.u_idev_req->io_Data = (APTR) &u.u_idev_int;
  u.u_idev_req->io_Command = IND_ADDHANDLER;
  DoIO ((struct IORequest *) u.u_idev_req);
}


void __ix_remove_sigwinch (void)
{
  usetup;

  if (u.u_idev_req)
    {
      u.u_idev_req->io_Data = (APTR) &u.u_idev_int;
      u.u_idev_req->io_Command = IND_REMHANDLER;
      DoIO ((struct IORequest *) u.u_idev_req);
      CloseDevice ((struct IORequest *) u.u_idev_req);
      ix_delete_extio((struct IORequest *)u.u_idev_req);
      u.u_idev_req = 0;
    }
}

#endif
