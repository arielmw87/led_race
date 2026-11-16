#include "antirrebote.h"
#include "ajustes.h"

void antirrebote_iniciar(antirrebote_t *a)
{
    a->estable = 0;
    a->cuenta = 0;
}

uint8_t antirrebote_paso(antirrebote_t *a, uint8_t crudo)
{
    if (crudo == a->estable) {
        /* Coincide con lo que ya creíamos: cualquier "intento de cambio"
           anterior era un rebote, así que empezamos a contar de nuevo. */
        a->cuenta = 0;
        return a->estable;
    }

    /* El pin muestra algo distinto: contamos cuántos ms seguidos lleva así. */
    a->cuenta++;
    if (a->cuenta >= ANTIRREBOTE_MS) {
        /* Se mantuvo el tiempo suficiente: el cambio es real. */
        a->estable = crudo;
        a->cuenta = 0;
    }
    return a->estable;
}
