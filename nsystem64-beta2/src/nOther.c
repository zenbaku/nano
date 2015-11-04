#include "nSysimp.h"
#include "nSystem.h"
#include <string.h>
#include <stdio.h>

/*************************************************************
 * DescribeTask
 *************************************************************/

/* Sirve para hacer debugging.  Despliega nombre y estado de una tarea */

static char *statusname[]= STATUS_LIST;

void DescribeTask(nTask task)
{
  nFprintf(2, "%s (estado %s)\n",
              task->taskname!=NULL ? task->taskname : "sin nombre",
              task->status<=STATUS_END ? statusname[task->status] :
                                         "desconocido");
}

/*************************************************************
 * Timers
 *************************************************************/

#include <stdlib.h>   /* Para setitimer */
#include <sys/time.h> /* Para setitimer */

void SetAlarm(int timertype, int msecs, void (*sighandler)())
{
  int unixsigtype, unixtimertype;
  struct itimerval Value;

  VerifyCritical("SetAlarm");

  if (timertype==REALTIMER)
  {
    unixsigtype= SIGALRM;
    unixtimertype= ITIMER_REAL;
  }
  else if (timertype==VIRTUALTIMER)
  {
    unixsigtype= SIGVTALRM;
    unixtimertype= ITIMER_VIRTUAL;
  }
  else
    nFatalError("SetAlarm", "Tipo de timer desconocido\n");

  SetHandler(unixsigtype, sighandler);

  /* Timer */
  Value.it_interval.tv_sec= 0;
  Value.it_interval.tv_usec= 0;
  Value.it_value.tv_sec= msecs/1000;
  Value.it_value.tv_usec= (msecs%1000)*1000;

  setitimer( unixtimertype, &Value, NULL);
}

/*************************************************************
 * nMalloc y nFree
 *************************************************************/

typedef struct MagicTag
{
  int size;
  int magic;
}
  MagicTag;

#define MAGICTAG (0x1a2f473b)
#define MAGICGARBAGE (0x2a65c17f)

/* nMalloc agrega un encabezado y una cola de dos palabras c/u
 * con el taman~o y un numero magico.  Al hacer nFree se
 * verifica que no hayan sido alterados, de otro modo gritamos.
 *   Ademas, nFree introduce basura en la memoria devuelta para
 * acelerar la ocurrencia de un error, en caso de que el
 * programador utilice ilegalmente la memoria que ya devolvio.
 *   Con estas verificaciones se pretende detectar la mayoria
 * de los errores por manejo incorrecto de punteros (pero no todos).
 */

void *nMalloc(int size)
{
  MagicTag *ptaghead, *ptagtail;
  void *ptr;
  size= (size+7)&-8; /* alinea el taman~o a una doble palabra */
  START_CRITICAL();
    ptaghead= (MagicTag*)malloc(sizeof(MagicTag)+size+sizeof(MagicTag));
    if (ptaghead==NULL) nFatalError("nMalloc", "Se acabo la memoria\n");
    ptr= (void*)&ptaghead[1];
    ptagtail= (MagicTag*)(((char*)ptr)+size);
    ptaghead->size= ptagtail->size= size;
    ptaghead->magic= ptagtail->magic= MAGICTAG;
  END_CRITICAL();
  return ptr;
}

void nFree(void *ptr)
{
  MagicTag *ptaghead, *ptagtail;
  int *pgarbage;
  ptaghead= &((MagicTag*)ptr)[-1];
  ptagtail= (MagicTag*)(((char*)ptr)+ptaghead->size);
  if (ptaghead->size!=ptagtail->size ||
      ptaghead->magic!=MAGICTAG || ptagtail->magic!=MAGICTAG)
    nFatalError("nFree", "Inconsistencia en el manejo de memoria\n");
  ptagtail++;
  for (pgarbage= (int*)ptaghead; pgarbage<(int*)ptagtail; pgarbage++)
    *pgarbage= MAGICGARBAGE;

  START_CRITICAL();
    free(ptaghead);
  END_CRITICAL();
}

/*************************************************************
 * nPrintf, nFprintf
 *************************************************************/

#include <stdarg.h>

#ifndef BUFFSIZE
#define BUFFSIZE 800
#endif

int nPrintf(char *format, ...)
{
  int rc;
  va_list ap;

  va_start(ap, format);
  rc= Gprintf(1, format, ap);
  va_end(ap);

  return rc;
}

int nFprintf(int fd, char *format, ...)
{
  int rc;
  va_list ap;

  va_start(ap, format);
  rc= Gprintf(fd, format, ap);
  va_end(ap);

  return rc;
}

int Gprintf(int fd, char *format, va_list ap)
{
  char buff[BUFFSIZE];
  int len;

  vsprintf(buff, format, ap);

  len= strlen(buff);
  if (len>=BUFFSIZE)
    nFatalError("nFprintf", "Se excede el taman~o del buffer\n");

  { int written=0;

    while (written<len)
    { int rc= nWrite(fd, &buff[written], len-written);
      if (rc<0) return EOF;
      written += rc;
  } }

  return len;
}

/*************************************************************
 * Chequeo del desborde de la pila
 *************************************************************/

/* Con MarkStack se colocan numeros magicos en la base de la pila,
 * donde se supone que no deberia llegar.  En los cambios de contexto
 * (ResumeNextTaskReady) se verifica que esos numeros no hayan cambiado.
 * Si cambiaron hubo posiblemente un desborde.  En ese caso el programador
 * deberia agrandar el stack de esa tarea.
 *   Este chequeo no es infalible.  Podrian haber desbordes no detectados.
 * Ademas, la deteccion es posterior al desborde 8^(...
 */

#define MAGIC1 ((void *)0x18457235)
#define ADDRMAGIC1 0
#define MAGIC2 ((void *)0x18457235)
#define ADDRMAGIC2 32

void MarkStack(SP sp)
{
  sp[ADDRMAGIC1]= MAGIC1;
  sp[ADDRMAGIC2]= MAGIC2;
}

void CheckStack(SP sp)
{
    if (sp!=NULL) /* El nMain nunca hace overflow */
      if (sp[ADDRMAGIC1] != MAGIC1 || sp[ADDRMAGIC2] != MAGIC2 )
        nFatalError("ChechStack", "Desorde de pila\n");
}
