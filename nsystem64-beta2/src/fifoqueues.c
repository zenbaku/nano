/* #include "nSysimp.h" */
#include "nSystem.h"
#include "fifoqueues.h"

/*************************************************************
 * Implementacion del manejo de colas FIFO
 *
 * Pura fuerza bruta, no es necesario mirar este codigo
 *
 *************************************************************/

typedef struct FifoQueueElem
{
  struct FifoQueueElem* next;
  void* obj;
} FifoQueueElem;

FifoQueue MakeFifoQueue() /* Puede ser llamada de cualquier parte */
{
  FifoQueue queue= (FifoQueue) nMalloc(sizeof(*queue));
  queue->first= NULL;
  queue->last= &queue->first;
  queue->len= 0;

  return queue;
}

void PutObj(FifoQueue queue, void* obj)
{
  FifoQueueElem* elem= (FifoQueueElem*)nMalloc(sizeof *elem);

  elem->obj= obj;
  elem->next= NULL;

  if (*(queue->last)!=NULL)
    nFatalError("PutObj", "Inconsistencia, last no es el ultimo\n");
  *(queue->last)= elem;
  queue->last= &elem->next;

  queue->len++;
}

void PushObj(FifoQueue queue, void* obj)
{
  FifoQueueElem* elem= (FifoQueueElem*)nMalloc(sizeof *elem);
  elem->obj= obj;
  elem->next= queue->first;
  queue->first= elem;
  if (elem->next==NULL) queue->last= &elem->next;

  queue->len++;
}

void* GetObj(FifoQueue queue)
{
  FifoQueueElem* elem;
  void* obj;

  elem= queue->first;
  if (elem==NULL) return NULL;

  queue->first= elem->next;
  if (queue->first==NULL) queue->last= &queue->first;

  obj= elem->obj;
  nFree(elem);

  queue->len--;

  return obj;
}

int QueryObj(FifoQueue queue, void* obj)
{
  FifoQueueElem* elem= queue->first;
  while (elem!=NULL && elem->obj!=obj)
    elem= elem->next;
  return elem!=NULL;
}

void DeleteObj(FifoQueue queue, void* obj)
{
  FifoQueueElem** pelem= &queue->first;

  while (*pelem!=NULL && (*pelem)->obj!=obj)
    pelem= &(*pelem)->next;

  if (*pelem!=NULL)
  {
    FifoQueueElem* elem= *pelem;
    *pelem= elem->next;
    if (elem->next==NULL)
    {
      if (queue->last!=&elem->next)
        nFatalError("DeleteObj",
                    "Inconsistencia, next es nulo y no es el ultimo\n");
      queue->last= pelem;
    }
    else if (queue->last==&elem->next)
      nFatalError("DeleteObj", "Inconsistencia, elem no es el ultimo\n");
    nFree(elem);
    queue->len--;
  }
}

int EmptyFifoQueue(FifoQueue queue)
{
  return queue->first==NULL;
}

int LengthFifoQueue(FifoQueue queue)
{
  return queue->len;
}

void DestroyFifoQueue(FifoQueue queue)
{
  if (!EmptyFifoQueue(queue))
    nFatalError("DestroyFifoQueue",
                "Se destruye una cola con tareas pendientes\n");

  nFree(queue); /* Se supone que no hay procesos colgando */
}
