#include <signal.h>

/*************************************************************
 * nProcess.c
 *************************************************************/

typedef void **SP;  /* Punteros a pilas */

typedef struct Task /* Descriptor de una tarea */
{
  int status;       /* Estado de la tarea (READY, ZOMBIE ...) */
  char *taskname;   /* Util para hacer debugging */

  SP sp;            /* El stack pointer cuando esta suspendida */
  SP stack;         /* El stack */

  struct Task *next_task;   /* Se usa cuando esta en una cola */
  void *queue;              /* Debugging */

  /* Para el nExitTask y nWaitTask */
  int  rc;                  /* codigo de retorno de la tarea  */
  struct Task *wait_task;   /* La tarea que espera un nExitTask */

  /* Para nSend, nReceive y nReply */
  struct Queue *send_queue; /* cola de emisores en espera de esta tarea */
  union { void *msg; int rc; } send; /* sirve para intercambio de info */
  int wake_time;            /* Tiempo maximo de espera de un nReceive */
}
  *nTask;

#define NOVOID_NTASK

/* Estados de una tarea */

#define READY      0  /* Elegible por el scheduler (incluye RUNNING) */
#define ZOMBIE     1  /* llamo nExitTask y espera nWaitTask (nExitTask) */
#define WAIT_TASK  2  /* espera el final de otra tarea (nWaitTask) */
#define WAIT_REPLY 3  /* hizo nSend y espera nReply (nSend) */
#define WAIT_SEND  4  /* hizo nReceive y espera nSend (nReceive) */
#define WAIT_SEND_TIMEOUT 5
                      /* hizo nReceive y espera nSend o timeout (nReceive) */
#define WAIT_READ  6  /* esta bloqueada en un read (nRead) */
#define WAIT_WRITE 7  /* esta bloqueada en un write (nWrite) */
#define WAIT_SEM   8  /* esta bloqueada en un semaforo (nWaitSem) */
#define WAIT_MON   9  /* esta bloqueada en un monitor (nEnterMonitor)*/
#define WAIT_COND 10  /* esta bloqueada en una condicion (nWaitCondition) */
#define WAIT_COND_TIMEOUT 11 /*esta bloqueado en un monitor con timeout */
#define WAIT_SLEEP 12 /* esta dormida en nSleep */

#define STATUS_END WAIT_SLEEP

/* Agregar nuevos estados como STATUS_END+1, STATUS_END+2, ... */

#define STATUS_LIST {"READY", "ZOMBIE", "WAIT_TASK", "WAIT_REPLY", \
                     "WAIT_SEND", "WAIT_SEND_TIMEOUT", "WAIT_READ", \
                     "WAIT_WRITE", "WAIT_SEM", "WAIT_MON", "WAIT_COND" }

/*
 * Prologo y Epilogo:
 */

void ProcessInit();
void ProcessEnd();

/*
 * Para el Scheduler:
 */

extern struct Queue *ready_queue;   /* Cola de tareas en espera de la CPU */
extern nTask current_task;  /* La tarea que tiene la CPU */
extern int current_slice;   /* Taman~o de una tajada de CPU */

extern int cpu_status;      /* Estados del procesador */

#define RUNNING 0           /* Corre alguna tarea */
#define WAIT_INTERRUPT 1    /* Ciclo de espera dentro de ResumeNextReadyTask */

/* extern int context_changes;  Solo para las estadisticas finales */

/* Suspende la tarea actual y retoma la primera de la ``ready_queue'' */
void ResumeNextReadyTask();

/* Para la entrada y salida de handlers */
void PreemptTask();
void ResumePreemptive();

/*************************************************************
 * nTime.c
 *************************************************************/

void TimeInit();
void TimeEnd();
void ProgramTask(int timeout);
void CancelTask(nTask task);

/*************************************************************
 * nMsg.c
 *************************************************************/

void MsgEnd();

/*************************************************************
 * nIO-sysv.c
 *************************************************************/

void IOInit();
void IOEnd();

/*************************************************************
 * nDep-sysv.c
 *************************************************************/

/* Secciones criticas:
 * Para evitar errores, las secciones criticas se pueden anidar.
 * Esto significa que si se coloca una secciones critica dentro
 * de otra, las interrupciones se habilitaran solo al salir
 * de la secciones critica mas externa.
 */

void START_CRITICAL(); /* deshabilitacion de interrupciones */
void END_CRITICAL();   /* habilitacion de interrupciones */

  /* Cuidado: Puede haber un cambio de contexto en la seccion critica, por
   * lo que las interrupciones se habilitan en la tarea que se retoma.
   */

/* Debugging : se usan en los handlers para chequear
 * que no haya interrupciones dentro de secciones criticas
 */
void StartHandler();
void EndHandler();

/* Debugging: Verifica que se este dentro de una seccion critica */
void VerifyCritical(char *str);

/*
 * Cambios de contexto explicitos
 */

void ChangeContext(nTask this_task, nTask next_task);
void CallInNewContext(nTask this_task, nTask new_task,
                      void (*proc)(), void *ptr);

/*
 * Bloqueo en espera de una interrupcion
 */

void WaitSignal();

/*
 * Definicion de handlers de interrupciones
 */

void SetHandler(int sigtype, void (*sighandler)());


/*************************************************************
 * Manejo de colas
 *************************************************************/

#include <nQueue.h>

/*************************************************************
 * nOther.c
 *************************************************************/

/*
 * Debugging: Despliega nombre y estado de una tarea.
 */
void DescribeTask(nTask task);

/*
 * Programacion de Timers:
 *
 * Programa el timer de tipo "type" para que cause una interrupcion
 * dentro de "msecs" microsegundos. "handler" es el procedimiento que
 * se llama durante la interrupcion. Se llama con las interrupciones
 * deshabilitadas. Su retorno habilita las interrupciones.
 */
void SetAlarm(int type, int msecs, void (*timerhandler)());

#define REALTIMER 1    /* Cuenta el tiempo real */
#define VIRTUALTIMER 2 /* Cuenta el tiempo de uso de CPU */

/* El timer real se usa para los timeouts en los receive y el
 * time virtual para las tajadas de tiempo en el scheduler.
 */

/*
 * Chequeo del desborde del stack:
 */

void MarkStack(SP sp);
void CheckStack(SP sp);

/*
 * Despliegue a la ``printf'' (parametros variables):
 */

int Gprintf();

/*************************************************************
 * Las primitivas de nSystem
 *************************************************************/
