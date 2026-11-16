/*
 * tira.h — manejo de la tira de LEDs APA102 / SK9822.
 *
 * Se trabaja en dos pasos:
 *   1. Se "pinta" en memoria (el arreglo de LEDs) con tira_pintar(), etc.
 *   2. Se manda todo junto a la tira con tira_refrescar().
 * Así la tira nunca muestra un dibujo a medio hacer.
 */
#ifndef TIRA_H
#define TIRA_H

#include <stdint.h>
#include "ajustes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configura PC4 (datos) y PC5 (clock) como salidas. */
void tira_iniciar(void);

/* Pinta el LED "pos" con el color (r, g, b) y brillo (0-31).
   Si pos está fuera de la tira (>= NL) no hace nada. */
void tira_pintar(uint16_t pos, uint8_t r, uint8_t g, uint8_t b, uint8_t brillo);

/* Apaga todos los LEDs (en memoria). */
void tira_apagar(void);

/* Divide el color de todos los LEDs por 2^corrimiento (los "atenúa").
   Llamándola en cada cuadro se logran estelas que se desvanecen. */
void tira_atenuar(uint8_t corrimiento);

/* Manda el contenido de la memoria a la tira. Tarda unos milisegundos. */
void tira_refrescar(void);

#ifdef __cplusplus
}
#endif

#endif /* TIRA_H */
