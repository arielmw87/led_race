/*
 * efectos.h — qué se dibuja en la tira en cada estado del juego.
 */
#ifndef EFECTOS_H
#define EFECTOS_H

#include <stdint.h>
#include "juego.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dibuja el cuadro que corresponde al estado actual y lo manda a la tira.
   Llamar en cada vuelta del loop. */
void efectos_dibujar(const juego_t *j, uint32_t ahora_ms);

#ifdef __cplusplus
}
#endif

#endif /* EFECTOS_H */
