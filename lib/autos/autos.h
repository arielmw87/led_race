/*
 * autos.h — física de un auto de la carrera.
 *
 * Cada auto tiene una posición y una velocidad. La velocidad sube con cada
 * PULSACIÓN (no por mantener apretado) y baja sola por rozamiento. La
 * posición avanza según la velocidad. Así, el que pulsa más rápido va más
 * rápido, y el LED avanza de a uno, de forma continua.
 *
 * Este módulo no toca el hardware: se puede probar en la PC.
 */
#ifndef AUTOS_H
#define AUTOS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* "Punto fijo": la posición se guarda multiplicada por 65536. Los 16 bits
   altos son el número de LED y los 16 bajos la "fracción" de LED recorrida.
   Ver autos.c para el porqué. */
#define AUTO_UNIDADES_POR_LED 65536UL

typedef struct {
    uint32_t posicion;    /* 1/65536 de LED, de 0 a NL*65536-1        */
    uint16_t velocidad;   /* 1/65536 de LED por milisegundo           */
    uint8_t  vueltas;     /* vueltas completas                        */
    uint8_t  boton_antes; /* cómo estaba el botón en el ms anterior   */
} auto_t;

/* Pone el auto en la largada, quieto. boton_ahora es el estado actual del
   botón: así, si el jugador ya lo tiene apretado, eso NO cuenta como
   pulsación (tiene que soltar y volver a apretar). */
void auto_reiniciar(auto_t *a, uint8_t boton_ahora);

/* Avanza la física 1 milisegundo. boton: 1 = apretado (ya sin rebote).
   Devuelve 1 si en este milisegundo completó una vuelta. */
uint8_t auto_paso_1ms(auto_t *a, uint8_t boton);

/* Número de LED donde está el auto (0 a NL-1). */
uint16_t auto_led(const auto_t *a);

#ifdef __cplusplus
}
#endif

#endif /* AUTOS_H */
