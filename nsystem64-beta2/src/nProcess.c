#include "nSysimp.h"
#include "nSystem.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

static nTask MakeTask(int stack_size);

static void VtimerHandler();
      /* El handler de interrupciones del timer virtual */

/*************************************************************
 * El prologo y el epilogo
 *************************************************************/

Queue ready_queue;                /* Las tareas ready */
nTask current_task;               /* La tarea running */

int cpu_status=RUNNING;           /* Estado del procesador */

static nTask main_task;           /* La tarea que corre nMain */

static int context_changes=0;     /* nro de cambios de contexto implicitos */
static double rq_sum_length= 0.0;
static int rq_n=0;

void ProcessInit()
{
  ready_queue= MakeQueue();
  main_task= current_task= MakeTask(0);
    /* el nMain usa el stack del proceso Unix */
  nSetTaskName("nMain");
}

void ProcessEnd()
{
  nFprintf(2, "\nNro. de cambios de contextos implicitos: %d\n",
               context_changes);
  if (rq_n!=0)
    nFprintf(2, "Largo promedio de la cola ``ready'': %f\n",
                rq_sum_length/rq_n);

  if ( ! EmptyQueue(ready_queue) )
    nFprintf(2, "\nTareas que quedaron ``ready'':\n");

  while ( ! EmptyQueue(ready_queue) )
    DescribeTask(GetTask(ready_queue));
}

nTask nCurrentTask()
{
  return current_task;
}

int nGetContextSwitches()
{
  return context_changes;
}

int nGetQueueLength()
{
  int len;
  START_CRITICAL();
    len= QueueLength(ready_queue);
  END_CRITICAL();
  return len;
}

/*************************************************************
 * Definicion de parametros para las tareas
 *************************************************************/

static int current_stack_size= 8192;  /* Taman~o de un stack */

/*
 * Define el taman~o del stack de una tarea.
 */

int nSetStackSize(int size)
{
  int old_stack_size= current_stack_size;
  current_stack_size= size; /* empieza a correr para las nuevas tareas */
                            /* que se creen */

  return old_stack_size;
}

/*
 * Define el taman~o de una tajada de CPU.
 *
 * Si current_slice==0 el scheduling es non-preemptive.
 * Esta es la situacion inicial para hacer el debugging mas facil.
 */

int current_slice=0;

void nSetTimeSlice(int slice)
{
  START_CRITICAL();
    current_slice= slice;
    SetAlarm(VIRTUALTIMER, current_slice , VtimerHandler);
    /* Si current_slice==0, el timer deja de interrumpir */
  END_CRITICAL();
}

/*************************************************************
 * El scheduler
 *************************************************************/

/* Estados del procesador:
 *
 * RUNNING =>
 *   Esta corriendo alguna tarea (ya sea codigo del programador o
 *   codigo de nSystem).
 *
 * WAIT_INTERRUPT =>
 *   Se llamo a ResumeNextReadyTask pero no habia ninguna tarea
 *   READY.  Se entra en un ciclo de espera de que lleguen tareas
 *   a la cola de tareas ready.  Estas tareas llegaran cuando,
 *   producto de una interrupcion, se invoque el handler de E/S
 *   o el del timer real (por timeout de un nReceive).
 */

/* Retoma la proxima tarea ready, sin considerar la tarea actual */

void ResumeNextReadyTask()
{
  nTask next_task;
  nTask this_task;

  while (EmptyQueue(ready_queue))
  {
    /* No hay tareas "ready".  Todas las tareas estan en alguna cola
     * esperando algun evento.  Los unicos eventos externos que
     * pueden despertar una tarea son: el reloj real (tiempo
     * de recepcion de un mensaje expira en alguna tarea);
     * una operacion de E/S realizable en algun descriptor de E/S, y;
     * el usuario presiona control-C (que mata el proceso).
     */
    if (current_task->status==READY) /* Debugging */
      nFatalError("ResumeNextReadyTask",
                  "Por que' la tarea que corria estaba ``ready''?\n");

    cpu_status= WAIT_INTERRUPT;

    WaitSignal(); /* Se espera una de la interrupciones mencionadas */

    cpu_status= RUNNING;
  }

  /* Ahora si' hay tareas ready */
  next_task= GetTask(ready_queue);
  this_task= current_task;

  /* Debugging: Se chequea la integridad de los stacks */
  CheckStack(this_task->stack);
  CheckStack(next_task->stack);

  /* EL CAMBIO DE CONTEXTO: */
  ChangeContext(this_task, next_task);

  current_task= this_task;
  /* Ahora ``this_task'' vuelve a ser la ``current_task'' */

  /*
   * Con la llamada a ChangeContext se transfiere la CPU (virtual)
   * a ``next_task''.  La pregunta es cuando retorna ChangeContext?
   *
   * Explicacion:  A estas alturas ``this_task'' esta descansando
   * en alguna cola esperando algun evento, como que le toque
   * una nueva tajada de tiempo.  Tarde o temprano ``this_task''
   * llegara a la cabeza de ``ready_queue'' y OTRA TAREA llamara
   * ResumeNextReadyTask para ceder la CPU.
   *
   * La invocacion de ChangeContext EN ESA OTRA TAREA
   * se traduce en el retorno de ChangeContext EN ``this_task''.
   *
   * Al igual que en recursividad en un cambio de contexto hay
   * dos activaciones de ResumeNextReadyTask.  Pero a diferencia
   * de la recursividad (en que se puede decir que una activacion
   * invoco a la otra) en un cambio de contexto ambas activaciones
   * de ResumeNextReadyTask estan AL MISMO NIVEL.  Este concepto
   * se llama ``reentrancia''.
   */
}

/*
 * Entrada y Salida de Handlers ``preemptive'', es decir que la
 * interrupcion puede quitarle la CPU a la tarea actual.
 * El handler tiene la forma:
 *
 * void <Int>Handler()
 * {  PreemptTask();
 *    ... procesamiento de la interrupcion ...
 *    ... pasa tareas al estado ready ...
 *    ResumePreemptive();
 * }
 *
 * PreemptTask: Si hay que quitarle la CPU a la tarea actual
 *              la agrega en la cola ready en primer lugar.
 *
 * Condiciones para quitarle la CPU a la tarea actual
 *
 * En modo ``non preemtive'' (current_slice==0) no podemos quitarle
 * la CPU a la tarea actual => no se hace nada.
 *
 * En modo ``preemtive'' hay dos posibilidades:
 *   No habia ninguna tarea READY (cpu_status==WAIT_INTERRUPT)
 *      ==> no se hace nada.
 *   La CPU corria una tarea READY (cpu_status==RUNNING)
 *      ==> La tarea actual se coloca en primer lugar en la cola
 *          de tareas ready.
 */

void PreemptTask()
{
  StartHandler(); /* Debugging */

  if (cpu_status==RUNNING && current_slice!=0)
  {
    context_changes++;
    PushTask(ready_queue, current_task);
} }

/*
 * Salida de un Handler ``preemptive'':
 *
 * Normalmente retomamos la proxima tarea ready, salvo que estemos
 * en modo ``non preemptive'' (tenemos que volver a la tarea interrumpida)
 * o que la CPU este en modo WAIT_INTERRUPT (estamos dentro de
 * ResumeNextReadyTask en un ciclo de espera de tareas ready por lo que
 * retornamos para que sea e'ste el que retome la proxima tarea). 
 */

void ResumePreemptive()
{
  if (cpu_status==RUNNING && current_slice!=0)
    ResumeNextReadyTask();

  EndHandler(); /* Debugging */
}

/*
 * El handler para las interrupciones del timer virtual.
 * Se invoca con las interrupciones deshabilitadas, las
 * que se rehabilitan cuando retorna.
 */

static void VtimerHandler()
{
  StartHandler(); /* Debugging */

  /* Se programa el taman~o de la tajada de tiempo (virtual).
   * Si current_slice==0, el timer deja de interrumpir.
   */
  SetAlarm(VIRTUALTIMER, current_slice, VtimerHandler);

  rq_sum_length += QueueLength(ready_queue);
  rq_n++;

  if (cpu_status==RUNNING)
  {
    context_changes++; /* solo para saber cuantos cambios implicitos hubo */

    PutTask(ready_queue, current_task); /* Al final de la ``ready_queue'' */
    ResumeNextReadyTask(); /* Este procedimiento retorna cuando a esta */
                             /* tarea le toque una nueva tajada de tiempo  */
  }

  EndHandler(); /* Debugging */
}

/*************************************************************
 * nEmitTask
 *************************************************************/

/* nEmitTask llama el procedimiento TaskInit en un nuevo stack (de
 * taman~o current_stack_size).  TaskInit recibe como argumento una
 * estructura de tipo InfoEmit con el procedimiento pasado a nEmit
 * y sus argumentos (a lo mas 6).
 */

typedef struct InfoEmit
{
  va_list ap;
  int (*proc)();
}
  InfoEmit;

static void TaskInit(InfoEmit *info);

nTask nEmitTask( int (*proc)(), ... )
{
  /* (un procedimiento puede declarar mas argumentos que la cantidad
   * de argumentos con que es llamado)
   */
  nTask new_task, this_task;
  InfoEmit info;
  va_list ap;

  /* Los argumentos y el procedimiento se pasaran a TaskInit en info */
  va_start(ap, proc);

  START_CRITICAL();

    this_task= current_task;

    /* Se crea el descriptor de la nueva tarea */
    new_task= MakeTask(current_stack_size);

    /* Debugging: chequea el desborde del stack */
    MarkStack(new_task->stack);

    /* Modificación para AMD64. Por Francisco Cifuentes */
    va_copy(info.ap, ap);
    info.proc= proc;

    /* La tarea actual la colocamos primera en la "ready_queue" */
    PushTask(ready_queue, this_task);

    /***** EL CAMBIO DE CONTEXTO ********/

    current_task= new_task; /* No sabemos hacerlo en ``TaskInit'' */

    CallInNewContext(this_task, new_task, TaskInit, (void *)&info );

    current_task= this_task;

    va_end(ap);

  END_CRITICAL();

  return new_task;
}

static void TaskInit( InfoEmit *pinfo )
{
  int rc;
  int (*proc)()= pinfo->proc;
  long  a0= va_arg(pinfo->ap, long);
  long  a1= va_arg(pinfo->ap, long);
  long  a2= va_arg(pinfo->ap, long);
  long  a3= va_arg(pinfo->ap, long);
  long  a4= va_arg(pinfo->ap, long);
  long  a5= va_arg(pinfo->ap, long);
  long  a6= va_arg(pinfo->ap, long);
  long  a7= va_arg(pinfo->ap, long);
  long  a8= va_arg(pinfo->ap, long);
  long  a9= va_arg(pinfo->ap, long);
  long a10= va_arg(pinfo->ap, long);
  long a11= va_arg(pinfo->ap, long);
  long a12= va_arg(pinfo->ap, long);
  long a13= va_arg(pinfo->ap, long);
  /* soporta hasta 14 argumentos enteros (o 7 punteros de 64 bits) */

  END_CRITICAL();
  /* Suena raro?  2 END_CRITICAL contra 1 START_CRITICAL!
   * En nExitTask esta el START_CRITICAL que falta ...
   */

  /* Llama el procedimiento raiz de la tarea */
  rc= (*proc)(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
  nExitTask(rc);
}

static nTask MakeTask(int stack_size)
{
  nTask new_task= (nTask) nMalloc(sizeof(*new_task));
  new_task->status= READY;
  new_task->taskname=NULL;
  new_task->wait_task= NULL; /* Ninguna tarea ha hecho nAbsorb */
  new_task->send_queue= MakeQueue();
  new_task->stack= stack_size==0 ? NULL : (SP) nMalloc(stack_size);
  new_task->sp= &new_task->stack[stack_size/sizeof(void *)];
  /* AMD64 requiere que la pila este alineada a 16 bytes */
  new_task->sp= (SP)( (long)new_task->sp & ~0xfL );
  new_task->queue= NULL;

  return new_task;
}

#define MAXNAMESIZE 80

void nSetTaskName(char *format, ...)
{
  char taskname[MAXNAMESIZE+1];
  int len;
  va_list ap;

  START_CRITICAL();

  va_start(ap, format);
  vsprintf(taskname, format, ap);
  va_end(ap);

  len= strlen(taskname);
  if (len>=MAXNAMESIZE)
    nFatalError("nSetTaskName", "Se excede el taman~o del buffer\n");

  if (current_task->taskname!=NULL) nFree(current_task->taskname);
  current_task->taskname=strcpy((char*)nMalloc(len+1), taskname);

  END_CRITICAL();
}

char* nGetTaskName()
{
  return current_task->taskname;
}

/*************************************************************
 * nExitTask y nWaitTask
 *************************************************************/

void nExitTask(int rc)
{
  START_CRITICAL();

    if (current_task->stack==NULL)
      nFatalError("nExitTask", "El nMain no debe morir\n");

    current_task->rc= rc;        /* el codigo de retorno */

    /* La tarea que estaba en espera de este nExitTask se coloca en la
     * cola de tareas ready.
     */
    if (current_task->wait_task!=NULL)
    {
      current_task->wait_task->status= READY;
      PushTask(ready_queue, current_task->wait_task);
    }

    current_task->status=ZOMBIE; /* Consultado por nWaitTask */
    ResumeNextReadyTask();
    /* (La proxima tarea se encarga de hacer END_CRITICAL) */

    /* Todavia no se puede liberar el stack, ya que ResumeNextReadyTask
     * lo esta usando.  Cuando ResumeNextReadyTask le pasa el control a otra
     * tarea, entonces se podria liberar el stack desde esa tarea.
     * Pero quien sabe que tarea es la que se esta retomando ??
     */
}

int nWaitTask(nTask task)
{
  int rc;

  START_CRITICAL();

    if (task->wait_task!=NULL)
      nFatalError("nWaitTask",
                  "Sos tareas no pueden esperar la misma tarea\n");

    task->wait_task= current_task;

    /* Si task no se ha suicidado todavia, hay que esperar */
    if (task->status!=ZOMBIE)
    {
      current_task->status= WAIT_TASK;
      ResumeNextReadyTask(); /* Vuelve cuando task invoco nExitTask */
    }

    if (task->taskname!=NULL) nFree(task->taskname);
    if (! EmptyQueue(task->send_queue))
       nFatalError("nWaitTask",
                   "Hay %d tarea(s) en la cola de la tarea moribunda\n",
                   QueueLength(task->send_queue) );
    DestroyQueue(task->send_queue);
    nFree(task->stack); /* Libera los recursos de la tarea */
    rc= task->rc;
    nFree(task);

  END_CRITICAL();

  return rc;
}
