#include "nSysimp.h"
#include "nSystem.h"

  /* Pura fuerza bruta, no es necesario mirar este codigo */

/*************************************************************
 * Colas para Scheduling de CPU : Queue
 *************************************************************/

Queue MakeQueue() /* Puede ser llamada de cualquier parte */
{
  Queue queue= (Queue) nMalloc(sizeof(*queue));
  queue->type= TYPE_QUEUE;
  queue->first= NULL;
  queue->last= &queue->first;

  return queue;
}

void PutTask(Queue queue, nTask task)
{
  /* VerifyCritical("PutTask"); */
  if (task->queue!=NULL)
    nFatalError("PutTask", "La tarea ya estaba en alguna cola\n");
  task->queue= queue;

  *(queue->last)= task;
  task->next_task= NULL;
  queue->last= &task->next_task;
}

void PushTask(Queue queue, nTask task)
{
  /* VerifyCritical("PushTask"); */
  if (task->queue!=NULL)
    nFatalError("PushTask", "La tarea ya estaba en alguna cola\n");
  task->queue= queue;

  task->next_task= queue->first;
  queue->first= task;
  if (task->next_task==NULL) queue->last= &task->next_task;
}

nTask GetTask(Queue queue)
{
  nTask task;

  /* VerifyCritical("GetTask"); */

  task= queue->first;
  if (task==NULL) return NULL;

  queue->first= task->next_task;
  if (queue->first==NULL) queue->last= &queue->first;

  task->queue= NULL;

  return task;
}

int QueryTask(Queue queue, nTask query_task)
{
  nTask task= queue->first;
  while (task!=NULL && task!=query_task)
    task= task->next_task;
  return task!=NULL;
}

void DeleteTaskQueue(Queue queue, nTask task)
{
  nTask *ptask;

  /* VerifyCritical("DeleteTaskQueue"); */

  ptask= &queue->first;
  while (*ptask!=task && *ptask!=NULL)
    ptask= &(*ptask)->next_task;

  if (*ptask==NULL)
    nFatalError("DeleteTaskQueue",
                "No se encuentro la tarea especificada\n");

  *ptask= task->next_task;
  if (queue->last==&task->next_task) queue->last= ptask;
  task->queue= NULL;
}

int EmptyQueue(Queue queue)
{
  return queue->first==NULL;
}

int QueueLength(Queue queue)
{
  int length=0;
  nTask task= queue->first;

  while (task!=NULL)
  {
    task= task->next_task;
    length++;
  }

  return length;
}

void DestroyQueue(Queue queue) /* Puede ser llamada de cualquier parte */
{
  if (!EmptyQueue(queue))
    nFatalError("DelQueue","Se destruye una cola con tareas pendientes\n");

  nFree(queue);  /* Se supone que no hay procesos colgando */
}

/*************************************************************
 * Colas ordenadas por tiempo : Squeue
 *************************************************************/

Squeue MakeSqueue()
{
  Squeue squeue= (Squeue) nMalloc(sizeof(*squeue));
  squeue->type= TYPE_SQUEUE;
  squeue->first= NULL;

  return squeue;
}

void PutTaskSqueue(Squeue squeue, nTask task, int wake_time)
{
  nTask *ptask;

  /* VerifyCritical("ProgramTask"); */
  if (task->queue!=NULL)
    nFatalError("ProgramTask", "La tarea ya estaba en alguna cola\n");
  task->queue= squeue;

  ptask= &squeue->first;
  while (*ptask!=NULL && (*ptask)->wake_time-wake_time<0)
    ptask= &(*ptask)->next_task;

  task->wake_time= wake_time;
  task->next_task= *ptask;
  *ptask= task;
}

nTask GetTaskSqueue(Squeue squeue)
{
  nTask task;

  /* VerifyCritical("GetNextProgrammedTask"); */

  task= squeue->first;
  if (task==NULL) return NULL;

  squeue->first= task->next_task;

  task->queue= NULL;

  return task;
}

int GetNextTimeSqueue(Squeue squeue)
{
  return squeue->first==NULL ? 0 : squeue->first->wake_time ;
}

int EmptySqueue(Squeue squeue)
{
  return squeue->first==NULL;
}

void DeleteTaskSqueue(Squeue squeue, nTask task)
{
  nTask *ptask;

  /* VerifyCritical("DeleteTaskSqueue"); */

  ptask= &squeue->first;
  while (*ptask!=task && *ptask!=NULL &&
         (*ptask)->wake_time-task->wake_time<=0)
    ptask= &(*ptask)->next_task;

  if (*ptask!=task)
    nFatalError("DeleteTaskSqueue",
                "No se encontro la tarea especificada\n");

  *ptask= task->next_task;
  task->queue= NULL;
}

void DestroySqueue(Squeue squeue)
{
  if (!EmptySqueue(squeue))
    nFatalError("DelSqueue", "Se destruye una cola con tareas pendientes\n");
  nFree(squeue);  /* No hay procesos colgando */
}
