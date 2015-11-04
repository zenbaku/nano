#include "nSysimp.h"
#include "nSystem.h"

/*************************************************************
 * Secciones Criticas:
 *************************************************************/

/*
 * En las secciones criticas no se permiten cambios de contexto implicitos.
 * Las secciones criticas se pueden anidar.  Para mayor seguridad
 * se trata de verificar el buen anidamiento de las secciones criticas.
 */

static int sig_level= 0; /* Se usa para el anidamiento de secc. criticas */
static sigset_t old_Sigset;

void START_CRITICAL() /* Se deshabilitan las interrupciones */
{
  sigset_t curr_Sigset;

  /* Cuidado: dos tareas pueden preguntar simultaneamente por sig_level==0 */
  if (sig_level==0)
  {
    /* se deshabilitan las int. */
    sigset_t new_Sigset;

    nAssert(sigfillset(&new_Sigset)==0, "sigfillset");
    nAssert(sigprocmask(SIG_BLOCK, &new_Sigset, &curr_Sigset)==0, "sigprocmask");
  }

  /* Ahora si que puedo incrementar sig_level */
  if (sig_level++==0) old_Sigset= curr_Sigset;
  /* No hace nada cuando las interrupciones ya habian sido deshabilitadas */
}

void END_CRITICAL() /* Se rehabilitan las interrupciones */
{
  if (sig_level==0)
    nFatalError("END_CRITICAL", "Mal uso de secciones criticas\n");

  if (--sig_level==0) {
    nAssert(sigprocmask(SIG_SETMASK, &old_Sigset, NULL)==0, "sigprocmask"); /* se habilitan las int. */
  }

  /* Si sig_level>=1 quiere decir que todavia estamos en una
   * seccion critica => no se pueden deshabilitar las interrupciones.
   */
}

/*************************************************************
 * Chequeo de funcionamiento de secciones criticas.
 * Existen para detectar si se invoca una interrupcion dentro
 * de una seccion critica.  No deberia ocurrir, pero es mejor
 * desconfiar de uno mismo y de Solaris 8^).
 *************************************************************/

void StartHandler()
{
  if (sig_level++!=0 && cpu_status==RUNNING)
    nFatalError("StartHandler", "Mal uso de secciones criticas\n");
}

void EndHandler()
{
  if (--sig_level<0)
    nFatalError("EndHandler", "Mal uso de secciones criticas\n");
}

void VerifyCritical(char *str)
{
  if (sig_level<0)
    nFatalError(str,
      "Esta operacion debe ser invocada dentro de una seccion critica");
}

/*************************************************************
 * Cambios de contexto.
 *************************************************************/

/*
 * Cambio de stacks: en nStack.s
 * Son sumamente dependientes de la maquina.
 */

void _ChangeToStack(SP *, SP *);
void _CallInNewStack( SP *from_psp, SP to_sp, void (*proc)(), void *ptr);

#ifdef NO_UNDERSCORE
#define _ChangeToStack ChangeToStack
#define _CallInNewStack CallInNewStack
#endif

void ChangeContext(nTask this_task, nTask next_task)
{
  int curr_sig_level= sig_level;

  _ChangeToStack(&this_task->sp, &next_task->sp);

  /* Se restaura el nivel de anidamiento de secciones criticas, puesto
   * que pueden diferir en ambos contextos (tareas).
   */
  sig_level=curr_sig_level;
}

void CallInNewContext(nTask this_task, nTask new_task,
                      void (*proc)(), void *ptr)
{
  int curr_sig_level= sig_level;

  _CallInNewStack(&this_task->sp, new_task->sp, proc, ptr);

  sig_level=curr_sig_level;
}

/*************************************************************
 * Bloqueo en espera de una interrupcion
 *************************************************************/

void WaitSignal()
{
  sigset_t Sigset;

  nAssert(sigfillset(&Sigset)==0, "sigfillset");
  nAssert(sigdelset(&Sigset, SIGVTALRM)==0, "sigdelset");
  nAssert(sigdelset(&Sigset, SIGALRM)==0, "sigdelset");
  nAssert(sigdelset(&Sigset, SIGINT)==0, "sigdelset");
  nAssert(sigdelset(&Sigset, SIGIO)==0, "sigdelset");

  sigsuspend(&Sigset);
}

/*************************************************************
 * Instalacion de un handler de interrupciones
 *************************************************************/

#ifndef SA_RESTART /* En algunas versiones esto es lo estandar */
#define SA_RESTART 0
#endif

#ifndef SA_RESETHAND /* En algunas versiones esto es lo estandar */
#define SA_RESETHAND 0
#endif

void SetHandler(int sigtype, void (*sighandler)())
{
  struct sigaction Action;

  Action.sa_handler= sighandler;
  sigfillset(&Action.sa_mask);
  /* Action.sa_flags= SA_RESTART | SA_RESETHAND; */
  Action.sa_flags= SA_RESTART;

  sigaction(sigtype, &Action, NULL);
}
