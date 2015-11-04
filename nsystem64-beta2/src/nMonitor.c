#include "nSysimp.h"
#include "fifoqueues.h"

typedef struct
{
  nTask owner;
  Queue mqueue;
  FifoQueue wqueue;
}
  *nMonitor;

typedef struct
{
  nMonitor mon;
  FifoQueue wqueue;
}
  *nCondition;

#define NOVOID_NMONITOR

#include <nSystem.h>
#include <stdio.h>

static void ReadyFirstTask(Queue queue);

nMonitor nMakeMonitor()
{
  nMonitor mon= (nMonitor)nMalloc(sizeof(*mon));
  mon->owner= NULL;
  mon->mqueue= MakeQueue();
  mon->wqueue= MakeFifoQueue();
  return mon;
}

void nDestroyMonitor(nMonitor mon)
{
  DestroyQueue(mon->mqueue);
  DestroyFifoQueue(mon->wqueue);
  nFree(mon);
}

void nEnter(nMonitor mon)
{
  START_CRITICAL();

  if (mon->owner!=NULL)
  {
    if (mon->owner==current_task)
      nFatalError("nEnter", "Trying to own the same monitor twice\n");
    current_task->status= WAIT_MON;
    PutTask(mon->mqueue, current_task);
    ResumeNextReadyTask();
  }

  mon->owner= current_task;

  END_CRITICAL();
}

void nExit(nMonitor mon)
{
  START_CRITICAL();

  if (mon->owner!=current_task)
    nFatalError("nExit", "This thread does not own this monitor\n");
  mon->owner= NULL;

  PushTask(ready_queue, current_task);
  ReadyFirstTask(mon->mqueue);
  ResumeNextReadyTask();
  
  END_CRITICAL();
}

void nWait(nMonitor mon)
{
  START_CRITICAL();

  if (mon->owner!=current_task)
    nFatalError("nWait", "This thread does not own this monitor\n");
  mon->owner= NULL;
  current_task->status= WAIT_COND;
  PutObj(mon->wqueue, current_task);
  ReadyFirstTask(mon->mqueue);
  ResumeNextReadyTask();

  mon->owner= current_task;

  END_CRITICAL();
}

void nNotifyAll(nMonitor mon)
{
  START_CRITICAL();

  if (mon->owner!=current_task)
    nFatalError("nNotifyAll", "This thread does not own this monitor\n");

  while (!EmptyFifoQueue(mon->wqueue))
  {
    nTask task= (nTask)GetObj(mon->wqueue);
    task->status= WAIT_MON;
    PushTask(mon->mqueue, task);
  }

  END_CRITICAL();
}

nCondition nMakeCondition(nMonitor mon)
{
  nCondition cond= (nCondition)nMalloc(sizeof(*cond));
  cond->mon= mon;
  cond->wqueue= MakeFifoQueue();
  return cond;
}

void nDestroyCondition(nCondition cond)
{
  DestroyFifoQueue(cond->wqueue);
  nFree(cond);
}

void nWaitCondition(nCondition cond)
{
  START_CRITICAL();

  if (cond->mon->owner!=current_task)
    nFatalError("nNotifyAll", "This thread does not own this monitor\n");

  cond->mon->owner= NULL;
  current_task->status= WAIT_COND;
  PutObj(cond->wqueue, current_task);
  ReadyFirstTask(cond->mon->mqueue);
  ResumeNextReadyTask();

  cond->mon->owner= current_task;

  END_CRITICAL();
}

void nSignalCondition(nCondition cond)
{
  nTask task;
  START_CRITICAL();

  if (cond->mon->owner!=current_task)
    nFatalError("nSignalCondition", "This thread does not own this monitor\n");

  task= (nTask)GetObj(cond->wqueue);
  if (task!=NULL)
  {
    task->status= WAIT_MON;
    PushTask(cond->mon->mqueue, task);
  }

  END_CRITICAL();
}

static void ReadyFirstTask(Queue queue)
{
  nTask task= GetTask(queue);
  if (task!=NULL)
  {
    task->status= READY;
    PushTask(ready_queue, task);
} }
