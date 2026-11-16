/*
 * temporizador.c — Timer0 en modo CTC con interrupción cada 1 ms.
 *
 * Cuenta: F_CPU = 16 MHz, prescaler 64  →  el timer cuenta a 250 kHz.
 * Con OCR0A = 249 el contador va de 0 a 249 (250 cuentas) = 1 ms.
 *
 * ¿Por qué la interrupción hace tan poco?
 *   Mientras una interrupción corre, el resto del programa (y las otras
 *   interrupciones, como la del sonido) espera. Por eso acá sólo se hace lo
 *   imprescindible y rápido; la lógica del juego va en el loop principal.
 *
 * Botones (activos en bajo, con pull-up): J1 = PB3, J2 = PB4, SELECT = PB5.
 * Ojo: PB5 es el D13 del Nano, que tiene el LED de la placa conectado. Para
 * que lea bien, el botón SELECT necesita un pull-up externo FUERTE, de
 * ~470 Ω a 5 V. ¿Por qué tan chico? El LED y su resistencia de 1 kΩ van a
 * GND y forman un divisor con el pull-up: con 4,7 kΩ el pin queda en
 * ~2,3 V, y el micro necesita al menos 3 V para leer un 1. Con 470 Ω queda
 * en ~4 V. (Si no, el juego cree que select está siempre apretado y aborta
 * todas las largadas.)
 */
#include "temporizador.h"
#include "antirrebote.h"
#include "display.h"
#include "juego.h" /* BOTON_J1, BOTON_J2, BOTON_SELECT */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#define PINES_BOTONES ((1 << PB3) | (1 << PB4) | (1 << PB5))

static volatile uint32_t milisegundos = 0;
static volatile uint8_t botones = 0;
static antirrebote_t filtro_j1, filtro_j2, filtro_select;

void temporizador_iniciar(void)
{
    DDRB  &= ~PINES_BOTONES; /* entradas      */
    PORTB |= PINES_BOTONES;  /* pull-up activo */

    antirrebote_iniciar(&filtro_j1);
    antirrebote_iniciar(&filtro_j2);
    antirrebote_iniciar(&filtro_select);

    TCCR0A = (1 << WGM01);              /* modo CTC          */
    TCCR0B = (1 << CS01) | (1 << CS00); /* prescaler 64      */
    OCR0A  = 249;                       /* 250 cuentas: 1 ms */
    TIMSK0 = (1 << OCIE0A);             /* habilitar la interrupción */
}

uint32_t temporizador_ms(void)
{
    uint32_t copia;
    /* Un uint32_t son 4 bytes y el micro los lee de a uno: si la
       interrupción lo cambia en el medio, leeríamos un valor mezclado.
       ATOMIC_BLOCK desactiva las interrupciones durante la copia. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        copia = milisegundos;
    }
    return copia;
}

uint8_t temporizador_botones(void)
{
    return botones; /* 1 byte: se lee de una sola vez, no hace falta ATOMIC */
}

ISR(TIMER0_COMPA_vect)
{
    display_multiplexar();
    milisegundos++;

    uint8_t pines = PINB;
    /* Activo en bajo: el pin vale 0 cuando el botón está apretado. */
    uint8_t b = 0;
    if (antirrebote_paso(&filtro_j1, !(pines & (1 << PB3))))     b |= BOTON_J1;
    if (antirrebote_paso(&filtro_j2, !(pines & (1 << PB4))))     b |= BOTON_J2;
    if (antirrebote_paso(&filtro_select, !(pines & (1 << PB5)))) b |= BOTON_SELECT;
    botones = b;
}
