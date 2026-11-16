/*
 * display.h — display de 7 segmentos de 4 dígitos (ánodo común).
 *
 * Muestra las vueltas de cada jugador: dígito izquierdo = jugador 1,
 * dígito derecho = jugador 2, los dos del medio apagados.
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configura los pines de segmentos y dígitos como salidas. */
void display_iniciar(void);

/* Elige qué mostrar. Valores mayores a 9 se muestran como 9. */
void display_vueltas(uint8_t j1, uint8_t j2);

/* Muestra el dígito siguiente. Se llama desde la interrupción de 1 ms. */
void display_multiplexar(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
