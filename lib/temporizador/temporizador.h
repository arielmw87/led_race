/*
 * temporizador.h — el "corazón" de 1 ms del programa (Timer0).
 *
 * Cada 1 ms una interrupción:
 *   - multiplexa el display,
 *   - suma 1 al reloj de milisegundos,
 *   - lee los botones y les filtra el rebote.
 *
 * Como el Timer0 se usa para esto, las funciones millis() y delay() de
 * Arduino NO funcionan en este proyecto: usar temporizador_ms().
 */
#ifndef TEMPORIZADOR_H
#define TEMPORIZADOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configura el Timer0 y los pines de los botones. Después hace falta sei(). */
void temporizador_iniciar(void);

/* Milisegundos desde que arrancó el programa. */
uint32_t temporizador_ms(void);

/* Botones apretados (ya sin rebote), como máscara BOTON_J1/J2/SELECT. */
uint8_t temporizador_botones(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPORIZADOR_H */
