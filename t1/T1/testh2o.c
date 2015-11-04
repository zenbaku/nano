#include <nSystem.h>
#include "h2o.h"

/* Ver el nMain al final */

typedef struct h2o {
  Hydrogen h1, h2;
  Oxygen o;
  int count;
  nMonitor m;
} *H2O;

/****************************************************
 * Procedimientos de prueba y verificacion
 ****************************************************/

static nMonitor mutex;
static int serial= 1;

static Oxygen produceOxy() {
  int ret;
  nEnter(mutex);
  ret= serial++;
  nExit(mutex);
  return ret;
}

static Hydrogen produceHydro() { return produceOxy(); }

H2O makeH2O(Hydrogen h1, Hydrogen h2, Oxygen o) {
  H2O h2o= (H2O)nMalloc(sizeof(*h2o));
  h2o->o= o;
  h2o->h1= h1;
  h2o->h2= h2;
  h2o->count= 0;
  h2o->m= nMakeMonitor();
  return h2o;
}

static int destroyIfLast(H2O h2o) {
  nEnter(h2o->m);
  h2o->count++;
  if (h2o->count>3 || h2o->count<0) /* En caso de bug podria ser basura */
    nFatalError("destroyIfLast",
      "Inconsistencia: El valor de count (%d) es basura\n", h2o->count);
  if (h2o->count<3) {
    nExit(h2o->m);
    return 0;
  }
  else {
    nExit(h2o->m);
    nDestroyMonitor(h2o->m);
    nFree(h2o);
    return 1;
  }
}

static int use1(H2O h2o, Oxygen o) {
  if (h2o->o!=o)
    nFatalError("use1", "El oxigeno %d en el h2o no es el suministrado %d\n",
      h2o->o, o);
  return destroyIfLast(h2o);
}

static int use2(H2O h2o, Hydrogen h) {
  if (h2o->h1!=h && h2o->h2!=h)
    nFatalError("use2",
      "El hidrogeno %d y %d en el h2o no es el suministrado %d\n",
      h2o->h1, h2o->h2, h);
  return destroyIfLast(h2o);
}

static int oxygen(void) {
  Oxygen o= produceOxy();
  H2O h2o= combineOxy(o);
  return use1(h2o, o);
}

static int hydrogen(void) {
  Hydrogen h= produceHydro();
  H2O h2o= combineHydro(h);
  return use2(h2o, h);
}

/****************************************************
 * Programa principal
 ****************************************************/

#define N_TASKS 10000
#define N_ITER 40

int nMain(int argc, char **argv) {
  mutex= nMakeMonitor();
  initH2O();
  /* Primer test de prueba: semantica */
  {
    nTask H1, H2, H3, H4, O1, O2;
    H1= nEmitTask(hydrogen);
    nSleep(10);
    O1= nEmitTask(oxygen);
    nSleep(10);
    O2= nEmitTask(oxygen);
    nSleep(10);
    H2= nEmitTask(hydrogen);
    if (nWaitTask(H1)+nWaitTask(O1)+nWaitTask(H2)!=1)
      nFatalError("nMain", "Primer retorno es incosistente\n");
    nSleep(10);
    H3= nEmitTask(hydrogen);
    nSleep(10);
    H4= nEmitTask(hydrogen);
    if (nWaitTask(O2)+nWaitTask(H3)+nWaitTask(H4)!=1)
      nFatalError("nMain", "2do. retorno es incosistente\n");
    nPrintf("Su tarea satisface el primer test de prueba\n");
  }

  /* Segundo test de prueba: robustez */
  {
    nTask ot[N_TASKS], h1t[N_TASKS], h2t[N_TASKS];
    int i, k, sum= 0;
    nPrintf("Iniciando test de robustez.\n");
    nPrintf("En dichato tomo ~30 segundos y ocupo 240 MB.\n");
    nPrintf("Iteracion (de un total de %d):\n", N_ITER);
    nSetTimeSlice(1);
    for (i= 0; i<N_TASKS; i++) {
      ot[i] = nEmitTask(oxygen);
      h1t[i]= nEmitTask(hydrogen);
      h2t[i]= nEmitTask(hydrogen);
    }
    for (k=1; k<N_ITER; k++) {
      for (i= 0; i<N_TASKS; i++) {
        if (ot[i]!=NULL)
          sum+= nWaitTask(ot[i]);
        ot[i]= nEmitTask(oxygen);
        if (h1t[i]!=NULL)
          sum+= nWaitTask(h1t[i]);
        h1t[i]= nEmitTask(hydrogen);
        if (h2t[i]!=NULL)
          sum+= nWaitTask(h2t[i]);
        h2t[i]= nEmitTask(hydrogen);
      }
      nPrintf("%d\n", k);
    }
    for (i= 0; i<N_TASKS; i++) {
      sum+= nWaitTask(ot[i]); ot[i]= NULL;
      sum+= nWaitTask(h1t[i]); h1t[i]= NULL;
      sum+= nWaitTask(h2t[i]); h2t[i]= NULL;
    }
    nPrintf("%d\n", k);
    if (sum!= N_TASKS*N_ITER)
      nFatalError("nMain", "Inconsistencia: 3er. retorno es inconsistente\n");
    nPrintf("Su tarea satisface el 2do. test de prueba\n");
  }
  return 0;
}

