#include "nSysimp.h"
#include "nSystem.h"

/*************************************************************
 * Epilogo
 *************************************************************/

static int pending_sends=0;
static int pending_receives=0;

void MsgEnd()
{
  if ( pending_sends!=0 || pending_receives!=0)
  {
    nFprintf(2, "\nNro. de tareas bloqueadas en un nSend: %d\n",
                         pending_sends);
    nFprintf(2, "Nro. de tareas bloqueadas en un nReceive: %d\n",
                         pending_receives);
} }

/*************************************************************
 * nSend, nReceive y nReply
 *************************************************************/

int nSend(nTask task, void *msg)
{
  int rc;

  START_CRITICAL();
  pending_sends++;
  { nTask this_task= current_task;

    if (task->status==WAIT_SEND || task->status==WAIT_SEND_TIMEOUT)
    {
      if (task->status==WAIT_SEND_TIMEOUT)
        CancelTask(task);
      task->status= READY;
      PushTask(ready_queue, task); /* En primer lugar en la cola */
    }
    else if (task->status==ZOMBIE)
      nFatalError("nSend", "El receptor es un ``zombie''\n");

    /* En nReply se coloca ``this_task'' en la cola de tareas ready */
    PutTask(task->send_queue, this_task);
    this_task->send.msg= msg;
    this_task->status= WAIT_REPLY;
    ResumeNextReadyTask();

    rc= this_task->send.rc;
  }
  pending_sends--;
  END_CRITICAL();

  return rc;
}

void *nReceive(nTask *ptask, int timeout)
{
  void *msg;
  nTask send_task;

  START_CRITICAL();
  pending_receives++;
  { nTask this_task= current_task;

    if (EmptyQueue(this_task->send_queue) && timeout!=0)
    {
      if (timeout>0)
      {
        this_task->status= WAIT_SEND_TIMEOUT;
        ProgramTask(timeout);
        /* La tarea se despertara automaticamente despues de timeout */
      }
      else this_task->status= WAIT_SEND; /* La tarea espera indefinidamente */

      ResumeNextReadyTask(); /* Se suspende indefinidamente hasta un nSend */
    }

    send_task= GetTask(this_task->send_queue);
    if (ptask!=NULL) *ptask= send_task;
    msg= send_task==NULL ? NULL : send_task->send.msg;
  }
  pending_receives--;
  END_CRITICAL();

  return msg;
}    

void nReply(nTask task, int rc)
{
  START_CRITICAL();

    if (task->status!=WAIT_REPLY)
      nFatalError("nReply","Esta tarea no espera un ``nReply''\n");

    PushTask(ready_queue, current_task);

    task->send.rc= rc;
    task->status= READY;
    PushTask(ready_queue, task);

    ResumeNextReadyTask();

  END_CRITICAL();
}
