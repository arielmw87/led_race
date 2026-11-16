/*
 * antirrebote.h — filtro de rebote para un botón.
 *
 * Cuando se aprieta o se suelta un pulsador, el contacto metálico "rebota":
 * durante unos pocos milisegundos la señal salta entre 0 y 1 varias veces.
 * Si leyéramos el pin directamente, una sola pulsación podría contarse como
 * varias. Este filtro sólo acepta un cambio cuando el pin se mantuvo en el
 * nivel nuevo ANTIRREBOTE_MS milisegundos seguidos.
 *
 * Importante: filtra en los DOS sentidos (al apretar y al soltar). En el
 * semáforo, soltar el botón descalifica: sin filtrar la soltada, un rebote
 * de 1 ms mientras el jugador mantiene apretado lo descalificaría.
 *
 * Este módulo no toca el hardware: recibe el valor "crudo" del pin y
 * devuelve el valor filtrado. Por eso se puede probar en la PC.
 */
#ifndef ANTIRREBOTE_H
#define ANTIRREBOTE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t estable; /* último valor aceptado: 1 = apretado, 0 = suelto   */
    uint8_t cuenta;  /* ms seguidos que el crudo lleva distinto a estable */
} antirrebote_t;

/* Deja el filtro en "suelto". */
void antirrebote_iniciar(antirrebote_t *a);

/* Llamar exactamente cada 1 ms. crudo: 1 = apretado, 0 = suelto.
   Devuelve el estado filtrado (estable). */
uint8_t antirrebote_paso(antirrebote_t *a, uint8_t crudo);

#ifdef __cplusplus
}
#endif

#endif /* ANTIRREBOTE_H */
