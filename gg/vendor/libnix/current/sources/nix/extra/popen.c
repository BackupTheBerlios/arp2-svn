#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "strsup.h"
#include "stabs.h"

typedef struct {
  struct MinNode  node;
  FILE           *fptr;
  struct Message  msg;
  APTR            dos;
  char           *cmd;
  struct TagItem *tags;
  int             rc;
} pomsg;

static struct MinList __popenlist = { /* list of popen blocks */
  (struct MinNode *)&__popenlist.mlh_Tail,
  NULL,
  (struct MinNode *)&__popenlist.mlh_Head
};

static int childprocess(void)
{ APTR DOSBase, SysBase = *(APTR *)4;
  struct Process *pr;
  pomsg *msg;
  int i;

  /* find our process structure */
  pr = (struct Process *)FindTask(NULL);

  /* Get the command to execute */
  msg = (pomsg *)(WaitPort(&pr->pr_MsgPort),GetMsg(&pr->pr_MsgPort));

  /* Now run the command. stdin and stdout are already set up */
  if ((DOSBase=msg->dos)) {
    i = System(msg->cmd,msg->tags);
  }
  else i = RETURN_FAIL;

  /* UNIX compatibility... */
  msg->rc = i << 8;

  /* pass the exit code back to the parent */
  ReplyMsg(&msg->msg);

  /* leave */
  return 0;
}

int pclose(FILE *fptr)
{ pomsg *msg;
  int rc;

  /* find pomsg block */
  for(msg=(pomsg *)__popenlist.mlh_Head->mln_Succ; msg; msg=(pomsg *)msg->node.mln_Succ)
    if (msg->fptr==fptr) break;

  /* found? */
  if (rc=-1,msg) {

    /* take it out */
    Remove((struct Node *)&msg->node);

    /* now wait for the exit status */
    WaitPort(msg->msg.mn_ReplyPort);

    /* free port */
    DeleteMsgPort(msg->msg.mn_ReplyPort);

    /* close the file */
    fclose(msg->fptr);

    /* get return code */
    rc = msg->rc;

    /* free memory */
    FreeVec(msg);
  }

  return rc;
}

#ifdef CreateNewProcTags
#undef CreateNewProcTags
static APTR createnewproctags(APTR DOSBase,ULONG tag,...)
{ return CreateNewProc((struct TagItem *)&tag); }
#define CreateNewProcTags(tag...) createnewproctags(DOSBase,tag)
#endif

FILE *popen(const char *cmd, const char *mode)
{ static const ULONG tags[] = { SYS_UserShell,TRUE,TAG_DONE };
  char redir[20],*pname;
  struct Process *pr;
  pomsg *poptr;
  ULONG size;
  FILE *fptr;

  /* verify mode */
  if (((mode[0]=='r') || (mode[0] == 'w')) && !mode[1]) {
    pname = mktemp(strcpy(redir,"pipe:pXXX.XXX"));
    if ((fptr=fopen(pname,mode))) {
      size = strlen(cmd) + sizeof(*poptr) + 20 + 15;
      if ((poptr=AllocVec(size,MEMF_PUBLIC))) {
        poptr->fptr                = fptr;
        poptr->msg.mn_Node.ln_Type = NT_MESSAGE;
        poptr->msg.mn_Node.ln_Pri  = 0;
        poptr->msg.mn_ReplyPort    = CreateMsgPort();
        if (poptr->msg.mn_ReplyPort) {
          poptr->dos = DOSBase;
          poptr->cmd = (char *)&poptr[1];
          /* Create the command. Since the "System" function runs through
             the default shell, we need to tell it not to fail so that we
             ALWAYS get back the exit status. This wouldn't be necessary
             if the CLI created by the System function inherited the parent's
             FAILAT level.
             The pipe file is passed as redirection to shell, which the 
             SystemTags() will parse. For some reason the default "more"
             could not get it's input properly if redirection was not used.
          */
          sprintf(poptr->cmd,"failat 9999\n%s %c%s\n",cmd,mode[0]=='w'?'>':'<',pname);

          /* our system tags */
          poptr->tags = (APTR)tags;

          /* get process structure */
          pr = (struct Process *)poptr->msg.mn_ReplyPort->mp_SigTask;

          /* get stacksize */
          size = (char *)pr->pr_Task.tc_SPUpper - (char *)pr->pr_Task.tc_SPLower;
          if (pr->pr_CLI)
            size = *(ULONG *)pr->pr_ReturnAddr;

          /* Now we can start the new process.
             This process then runs the comman passed in the startup message. */
          pr = CreateNewProcTags(
            NP_Entry,       (Tag)childprocess,
            NP_Name,        (Tag)"popen process",
            NP_Input,       Input(),
            NP_CloseInput,  FALSE,
            NP_Output,      Output(),
            NP_CloseOutput, FALSE,
            NP_StackSize,   size,
            NP_Cli,         TRUE,
            TAG_DONE
            );

          /* success? */
          if (pr) {

            /* add node to our list */
            AddHead((struct List *)&__popenlist,(struct Node *)&poptr->node);

            /* now pass the child the startup message */
            PutMsg(&pr->pr_MsgPort,&poptr->msg);

            /* return FILE ptr */
            return poptr->fptr;
          }
          else
            errno = EPROCLIM;

          DeleteMsgPort(poptr->msg.mn_ReplyPort);
        }
        else
          errno = ENOMEM;

        FreeVec(poptr);
      }
      else
        errno = ENOMEM;

      fclose(fptr);
    }
    else
      errno = EMFILE;
  }
  else
    errno = EINVAL;

  return NULL;
}

void __exitpopen(void)
{ struct MinNode *node;
  while((node=__popenlist.mlh_Head)->mln_Succ!=NULL)
    pclose(((pomsg *)node)->fptr);
}
ADD2EXIT(__exitpopen,-15);
