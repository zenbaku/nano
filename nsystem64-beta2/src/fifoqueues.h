
/*************************************************************
 * Manejo de colas FIFO
 *
 * Atencion: Estas colas no pueden ser manipuladas concurrentemente.
 *
 *************************************************************/

typedef struct FifoQueue
{
  struct FifoQueueElem* first;
  struct FifoQueueElem** last;
  int len;
}
  *FifoQueue; /* Atencion, una cola es un puntero */

FifoQueue MakeFifoQueue();              /* El constructor */
void PutObj(FifoQueue q, void* o);      /* Agrega un objeto al final */
void* GetObj(FifoQueue q);              /* Retorna y extrae el primer objeto */
int  EmptyFifoQueue(FifoQueue q);       /* Verdadero si la cola esta vacia */
void DestroyFifoQueue(FifoQueue q);     /* Elimina la cola */

/* Procedimientos adicionales */

int LengthFifoQueue(FifoQueue q);    /* Entrega el largo de la cola */
int QueryObj(FifoQueue q, void* o);     /* Verdadero si o esta en la cola */
void DeleteObj(FifoQueue q, void* o);   /* Elimina o si esta en la cola */
void PushObj(FifoQueue q, void* o);     /* Agrega un objeto al principio */
