#include <nSystem.h>
#include "h2o.h"

#define N_OXYGEN 100
#define N_HYDROGEN 100

// Monitor y condiciones para el problema.
nMonitor m;
nCondition waitingH;
nCondition waitingO;

// Variables globales para las tareas:
//  - aH: cantidad de tareas de hidrógeno activas (formando una molécula)
//  - aO: cantidad de tareas de oxígeno activas (formando una molécula)
//  - wH: cantidad de tareas de hidrógeno esperando asociarse con otras tareas
//  - wO: cantidad de tareas de oxígeno esperando asociarse con otras tareas
int aH = 0, aO = 0, wH = 0, wO = 0;

// Variable global utilizada para almacenar la molécula de agua en formación
// Se asume, por construcción, que nunca se formarán dos moléculas al mismo tiempo.
// 
// Esta variable se utiliza para "pasar" molécula desde la tarea que la formó
// a las tareas que participan de la formación.
H2O water;

// Se utilizan dos buffers (como se implementan en los apuntes).
// Uno para producir y consumir átomos de oxígeno.
// Otro otra para átomos de hidrógeno.
// Nota: lamentablemente el código quedó algo duplicado.
struct HydrogenAtoms {
    Hydrogen buf[N_HYDROGEN];
    int next_empty;
    int next_full;
    int count;
    nSem empty;
    nSem full;
} hydrogen_buf;

struct OxygenAtoms {
    Oxygen buf[N_OXYGEN];
    int next_empty;
    int next_full;
    int count;
    nSem empty;
    nSem full;
} oxygen_buf;

// Poner un átomo de oxígeno en el buffer de átomos de oxígeno
void putOxygen(Oxygen o)
{
    // nPrintf("putOxygen esperando ticket empty...\n");
    nWaitSem(oxygen_buf.empty);
    // nPrintf("putOxygen trabajando...\n");
    oxygen_buf.buf[oxygen_buf.next_empty] = o;
    oxygen_buf.next_empty = (oxygen_buf.next_empty + 1) % N_OXYGEN;
    nSignalSem(oxygen_buf.full);
}

// Obtener un átomo de oxígeno del buffer de átomos de oxígeno
Oxygen getOxygen()
{
    Oxygen ret;
    // nPrintf("getOxygen esperando ticket full...\n");
    nWaitSem(oxygen_buf.full);
    // nPrintf("getOxygen trabajando...\n");
    ret = oxygen_buf.buf[oxygen_buf.next_full];
    oxygen_buf.next_full = (oxygen_buf.next_full + 1) % N_OXYGEN;
    nSignalSem(oxygen_buf.empty);
    return ret;
}

// Poner un átomo de hidrógeno en el buffer de átomos de hidrógeno
void putHydrogen(Hydrogen h)
{
    // nPrintf("putHydrogen esperando ticket empty...\n");
    nWaitSem(hydrogen_buf.empty);
    // nPrintf("putHydrogen trabajando...\n");
    hydrogen_buf.buf[hydrogen_buf.next_empty] = h;
    hydrogen_buf.next_empty = (hydrogen_buf.next_empty + 1) % N_HYDROGEN;
    nSignalSem(hydrogen_buf.full);
}

// Obtener un átomo de hidrógeno del buffer de átomos de hidrógeno
Hydrogen getHydrogen()
{
    Hydrogen ret;
    // nPrintf("getHydrogen esperando ticket full...\n");
    nWaitSem(hydrogen_buf.full);
    // nPrintf("getHydrogen trabajando...\n");
    ret = hydrogen_buf.buf[hydrogen_buf.next_full];
    hydrogen_buf.next_full = (hydrogen_buf.next_full + 1) % N_HYDROGEN;
    nSignalSem(hydrogen_buf.empty);
    return ret;
}

// Función inicializadora de las variables globales.
void initH2O(void) {
    // Se inicializan "pools" de átomos
    // Para hidrógeno
    hydrogen_buf.next_empty = 0;
    hydrogen_buf.next_full = 0;
    hydrogen_buf.count = 0;
    hydrogen_buf.empty = nMakeSem(N_HYDROGEN);
    hydrogen_buf.full = nMakeSem(0);
    // Para oxígeno
    oxygen_buf.next_empty = 0;
    oxygen_buf.next_full = 0;
    oxygen_buf.count = 0;
    oxygen_buf.empty = nMakeSem(N_OXYGEN);
    oxygen_buf.full = nMakeSem(0);

    // Inicialización de monitor y condiciones
    // del problema
    m = nMakeMonitor();
    waitingH = nMakeCondition(m);
    waitingO = nMakeCondition(m);
}

H2O combineHydro(Oxygen h)
{
    H2O ret;

    nEnter(m);
    // ¿Es esta tarea la que formó la molécula, o sólo participa de ella?
    int productor = 0;

    // Se añade esta tarea como "esperando"
    wH += 1;

    // Meter un hidrógeno al buffer
    putHydrogen(h);

    // Esta tarea sólo puede retornar cuando ya no hayan más tareas activas
    while (aH == 0) {
        // Si hay suficientes esperando, utilizarlas para formar la molécula
        if (wH >= 2 && wO >= 1) {
            // Actualizar contadores
            wH -= 2; aH += 2;
            wO -= 1; aO += 1;

            // Marcar esta tarea como la productora de la molécula
            productor = 1;

            // Obtener los átomos de los buffers respectivos
            Hydrogen h1 = getHydrogen();
            Hydrogen h2 = getHydrogen();
            Oxygen o = getOxygen();

            // Formar la molécula y guardar la referencia en la variable global
            water = makeH2O(h1, h2, o);
            ret = water;

            // Avisar a las otras tareas asociadas que la molécula está lista
            nSignalCondition(waitingH);
            nSignalCondition(waitingO);
        } else {
            // Seguir esperando
            nWaitCondition(waitingH);
        }
    }
    // Se descuenta esta tarea como "activa"
    aH -= 1;

    // Si esta tarea no fue la productora, significa que aportó su átomo
    // a la formación
    if (productor == 0) {
        // Obtener la molécula desde la variable global
        ret = water;
    }
    nExit(m);

    return ret;
}


H2O combineOxy(Oxygen o)
{
    H2O ret;

    nEnter(m);
    // ¿Es esta tarea la que formó la molécula, o sólo participa de ella?
    int productor = 0;

    // Se añade esta tarea como "esperando"
    wO += 1;

    // Meter un oxígeno al buffer    
    putOxygen(o);

    // Esta tarea sólo puede retornar cuando ya no hayan más tareas activas
    while (aO == 0) {
        // Si hay suficientes esperando, utilizarlas para formar la molécula
        if (wH >= 2 && wO >= 1) {
            // Actualizar contadores
            wH -= 2; aH += 2;
            wO -= 1; aO += 1;
            
            // Marcar esta tarea como la productora de la molécula
            productor = 1;

            // Obtener los átomos de los buffers respectivos
            Hydrogen h1 = getHydrogen();
            Hydrogen h2 = getHydrogen();
            Oxygen o = getOxygen();

            // Formar la molécula y guardar la referencia en la variable global
            water = makeH2O(h1, h2, o);
            ret = water;
            
            // Avisar a las otras tareas asociadas que la molécula está lista            
            nSignalCondition(waitingH);
            nSignalCondition(waitingH);
        } else {
            // Seguir esperando
            nWaitCondition(waitingO);
        }
    }
    // Se descuenta esta tarea como "activa"
    aO -= 1;

    // Si esta tarea no fue la productora, significa que aportó su átomo
    // a la formación
    if (productor == 0) {
        // Obtener la molécula desde la variable global
        ret = water;
    }

    nExit(m);

    return ret;
}