#include "nSysimp.h"
#include "nSystem.h"
#include <stdarg.h>

/* Para fcntl: */
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

/* Para ttyname e isatty: */
#include <stdlib.h>
#include <errno.h>

#ifdef SOLARIS
#include <stropts.h>
#endif

/*
 * Procedimientos locales al modulo:
 */

static void SetNonBlocking(int fd); /* Coloca un fd en modo no bloqueante */
static void SigioHandler(); /* Handler de interrupciones de E/S */
static void AddWaitingTask(int fd, nTask task);

/*************************************************************
 * El prologo y el epilogo
 *************************************************************/

static nTask *pending_tasks; /* Tareas con E/S pendiente */
static int maxsize_pending;  /* El taman~o de ambos vectores */

void IOInit()
{
  maxsize_pending=20;
  pending_tasks= (nTask *)
          malloc(maxsize_pending * sizeof(nTask));

  { int i;
    for (i=0; i<maxsize_pending; i++)
      pending_tasks[i]= NULL;
  }

  SetHandler(SIGIO, SigioHandler);   /* Define el handler de E/S */
}

void nSetNonBlockingStdio()
{
#ifdef REOPENSTDIO
  char *ttyname();

  if (isatty(0))
  {
    char *ttyin= ttyname(0);
    close(0);
    if (open(ttyin, O_RDONLY, 0)!=0)
      nFatalError("nReopenStdio","Por que' la entrada estandar no es 0?\n");
  }

  if (isatty(1))
  {
    char *ttyin= ttyname(1);
    close(1);
    if (open(ttyin, O_WRONLY, 0)!=1)
      nFatalError("nReopenStdio","Por que' la salida estandar no es 1?\n");
  }
#endif

  SetNonBlocking(0);
  SetNonBlocking(1);
}

void IOEnd()
{
  int i;
  int flags_in= fcntl(0, F_GETFL);
  int flags_out= fcntl(1, F_GETFL);

  /* Esto evita que las ventanas desaparezcan */
  fcntl(0, F_SETFL, flags_in&~O_NONBLOCK);
  fcntl(1, F_SETFL, flags_out&~O_NONBLOCK);

  for (i=0; i<maxsize_pending; i++)
    if (pending_tasks[i]!= NULL)
    {
      nFprintf(2,"\nTareas con E/S pendiente:\n");
      break;
    }

  while (i<maxsize_pending)
  {
    if (pending_tasks[i]!= NULL)
      DescribeTask(pending_tasks[i]);
    i++;
  }
}

/*************************************************************
 * nOpen, nClose, nRead, nWrite
 *************************************************************/

int nOpen(char *path, int flags, ...)
{
  int fd;
  va_list ap;
  int mode;

  START_CRITICAL();
    va_start(ap, flags);
    mode= va_arg(ap, int);
    va_end(ap);
    
    fd= open(path, flags, mode);
    if (fd>=0) SetNonBlocking(fd);
  END_CRITICAL();

  return fd;
}

int nClose(int fd)
{
  int rc;

  START_CRITICAL();
    rc= close(fd);
  END_CRITICAL();

  return rc;
}

int nRead(int fd, char *buf, int nbyte)
{  
  int rc;

  START_CRITICAL();

    rc= read(fd, buf, nbyte); /* Intentamos leer */
    while (rc<0 && errno==EAGAIN)
    {                         /* No hay nada disponible */
      AddWaitingTask(fd, current_task);
      current_task->status= WAIT_READ;
      ResumeNextReadyTask(); /* Pasamos a la proxima que este ready */
      rc= read(fd, buf, nbyte); /* Ahora si que deberia funcionar */
    }

  END_CRITICAL();

  return rc;
}

int nWrite(int fd, char *buf, int nbyte)
{  
  int rc;

  START_CRITICAL();

    rc= write(fd, buf, nbyte); /* Intentamos escribir */
    while (rc<0 && errno==EAGAIN)
    {                         /* El buffer esta lleno */
      AddWaitingTask(fd, current_task);
      current_task->status= WAIT_WRITE;
      ResumeNextReadyTask(); /* Pasamos a la proxima que este ready */
      rc= write(fd, buf, nbyte); /* Ahora deberia poder escribir un poco */
    }

  END_CRITICAL();

  return rc;
}

/*************************************************************
 * SetNonBlocking
 *************************************************************/

/* Coloca un fd en modo no bloqueante asincrono (envia una
 * interrupcion SIGIO cuando hay algo para leer o escribir).
 */

static void SetNonBlocking(int fd)
{
#ifdef SOLARIS
  if (isastream(fd))
  {
    int flags= fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags|O_NONBLOCK);
    ioctl(fd, I_SETSIG, S_RDNORM|S_WRNORM);
  }
#else
  int flags= fcntl(fd, F_GETFL);
  int unix_pid= getpid();
  fcntl(fd, F_SETFL, flags|O_NONBLOCK|FASYNC);
  fcntl(fd, F_SETOWN, unix_pid);
#endif
}

/*************************************************************
 * AddWaitingTask
 *************************************************************/

/* Este procedimiento agrega una tarea con E/S pendiente al vector
 * pending_fds que necesita la primitiva ``poll''.
 * AddWaitingTask busca un elemento desocupado (pending_fds[i].fd==-1)
 * y lo coloca ahi.  La tarea que origino el requerimiento queda
 * en pending_tasks[i].  La busqueda es circular.
 */

static void AddWaitingTask(int fd, nTask task)
{
  pending_tasks[fd]= task;
}

/*************************************************************
 * SigioHandler
 *************************************************************/

static struct timeval zero_timeval={0,0};

static void SigioHandler()
{
  int fd;
  fd_set readfds, writefds;

  PreemptTask();

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  /* Vemos que descriptores tienen asociada E/S pendiente */
  for (fd=0; fd<maxsize_pending; fd++) {
    nTask task= pending_tasks[fd];

    if (task!=NULL) { /* Hay una tarea pendiente para este descriptor */
      if (task->status==WAIT_READ)
        FD_SET(fd, &readfds);
      else if (task->status==WAIT_WRITE)
        FD_SET(fd, &writefds);
      else nFatalError("SigioHandler","Bug!\n");
    }
  }

  select(maxsize_pending, &readfds, &writefds, NULL, &zero_timeval);

  /* Vemos para que descriptores se ha resuelto la E/S pendiente */

  for (fd=0; fd<maxsize_pending; fd++) {
    nTask task= pending_tasks[fd];
    if (task!=NULL &&
       (FD_ISSET(fd, &readfds) || FD_ISSET(fd, &writefds))) {
       task->status= READY;
       PushTask(ready_queue, task);
       pending_tasks[fd]=NULL; /* Se resolvio ese descriptor */
       }
   }

  SetHandler(SIGIO, SigioHandler);

  ResumePreemptive();
}
