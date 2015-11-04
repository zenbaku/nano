#include "nSysimp.h"

/*************************************************************
 * Manejo de Semaforos
 *************************************************************/

typedef struct nSem
{
  int count;
  struct Queue *queue;
}
  *nSem;

#define NOVOID_NSEM

#include "nSystem.h"

nSem nMakeSem(int count)
{
  nSem sem;

  sem= (nSem) nMalloc(sizeof(*sem));
  sem->count= count;
  sem->queue= MakeQueue();

  return sem;
}

void nWaitSem(nSem sem)
{
  START_CRITICAL();

  if (sem->count>0)
    sem->count--;
  else
  {
    current_task->status= WAIT_SEM;
    PutTask(sem->queue, current_task);
    ResumeNextReadyTask();
  }

  END_CRITICAL();
}

void nSignalSem(nSem sem)
{
  START_CRITICAL();

    if (EmptyQueue(sem->queue))
      sem->count++;
    else
    {
       nTask wait_task= GetTask(sem->queue);
       wait_task->status= READY;
       /* wait_task ahora pasa al estado ready y queremos que
        * tome la CPU.  La siguiente secuencia es la unica forma
        * de transferir la CPU directamente a otra tarea.
        */
       PushTask(ready_queue, current_task); /* Sigue estando ready */
       PushTask(ready_queue, wait_task);
       ResumeNextReadyTask(); /* wait_task es la primera en la cola! */
       /* Frecuentemente, esta tarea retomara la CPU cuando wait_task
        * pierda la CPU.
        */
    }

  END_CRITICAL();
}

void nDestroySem(nSem sem)
{
  if (! EmptyQueue(sem->queue) )
    nFatalError("nDestroySem",
      "Se intenta destruir un semaforo con tareas pendientes\n");
  DestroyQueue(sem->queue);
  nFree(sem);
}

