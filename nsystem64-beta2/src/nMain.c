#include "nSysimp.h"
#include "nSystem.h"
#include <stdlib.h>
#include <string.h>

#ifdef TRAPINTS
static void TrapInts();
#endif

/*************************************************************
 * main, nExitSystem
 *************************************************************/

int main(int argc, char **argv)
{
  int rc;

  START_CRITICAL();
    ProcessInit();
    TimeInit();
    IOInit();

#ifdef TRAPINTS
    TrapInts();
#endif

  END_CRITICAL();

  { /* analisis de los argumentos */
    int in;
    int out=1;
    int count=1;
    for (in=1; in<argc; in++)
      if (strcmp(argv[in],"-slice")==0 && ++in<argc)
      {
        char *pc;
        for (pc= argv[in]; *pc!='\0'; pc++)
          if (! ('0'<= *pc && *pc<='9'))
            nFatalError("main", "Invalid option -slice %s", argv[in]);
        nSetTimeSlice(atoi(argv[in]));
      }
      else if (strcmp(argv[in],"-noblocking")==0)
        nSetNonBlockingStdio();
      else
      {
        argv[out++]= argv[in];
        count++;
      }
      argc= count;
  }

  rc= nMain(argc, argv);

  nExitSystem(rc);
  return rc; /* nunca llega aca */
}

void nExitSystem(int rc)
{
  START_CRITICAL();

    nFprintf(2,"Estadisticas finales:\n");
    ProcessEnd();
    MsgEnd();
    TimeEnd();
    IOEnd();

    exit(rc);
}

/*************************************************************
 * nFatalError
 *************************************************************/

#include <stdarg.h>

static int lock= 0;

void nFatalError(char *procname, char *format, ...)
{
  va_list ap;

  START_CRITICAL();

    /* Tratamos de evitar llamadas recursivas */
    if (lock==1) return; /* Pase lo que pase 8^) */
    lock=1;

    va_start(ap, format);
    nFprintf(2,"Error Fatal en la rutina %s y la tarea \n", procname);
    DescribeTask(current_task);
    Gprintf(2, format, ap);
    va_end(ap);

    nExitSystem(1); /* shutdown */

  /* END_CRITICAL(); no es necesario, ya termino */
}

/*************************************************************
 * Manejo de interrupciones extran~as
 *************************************************************/

void SighupHandler()
{
  nFatalError("SIGHUP", "Interrupcion no controlada\n");
}

void SigintHandler()
{
  nFatalError("SIGINT", "Interrupcion no controlada\n");
}

void SigquitHandler()
{
  nFatalError("SIGQUIT", "Interrupcion no controlada\n");
}

void SigillHandler()
{
  nFatalError("SIGILL", "Interrupcion no controlada\n");
}

void SigabrtHandler()
{
  nFatalError("SIGABRT", "Interrupcion no controlada\n");
}

void SigfpeHandler()
{
  nFatalError("SIGFPE", "Interrupcion no controlada\n");
}

void SigbusHandler()
{
  nFatalError("SIGBUS", "Interrupcion no controlada\n");
}

void SigsegvHandler()
{
  nFatalError("SIGSEGV", "Interrupcion no controlada\n");
}

void SigsysHandler()
{
  nFatalError("SIGSYS", "Interrupcion no controlada\n");
}

void SigpipeHandler()
{
  nFatalError("SIGPIPE", "Interrupcion no controlada\n");
}

#ifdef TRAPINTS
void TrapInts()
{
  SetHandler(SIGHUP, SighupHandler);
  /*SetHandler(SIGINT, SigintHandler);*/
  /*SetHandler(SIGQUIT, SigquitHandler);*/
  SetHandler(SIGILL, SigillHandler);
  SetHandler(SIGABRT, SigabrtHandler);
  SetHandler(SIGFPE, SigfpeHandler);
  SetHandler(SIGBUS, SigbusHandler);
  SetHandler(SIGSEGV, SigsegvHandler);
#ifdef SIGSYS
  SetHandler(SIGSYS, SigsysHandler);
#endif
  SetHandler(SIGPIPE, SigpipeHandler);
}
#endif
