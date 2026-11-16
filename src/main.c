/*
 * main.c — Carrera de LEDs.
 *
 * Usamos nuestro propio main() en vez de setup()/loop() de Arduino para
 * manejar el micro directamente con sus registros.
 *
 * El loop principal es muy simple y NUNCA se queda esperando:
 *   1. Mira la hora y los botones.
 *   2. Le pregunta a la máquina de estados qué pasó (juego_paso).
 *   3. Según lo que pasó, dispara sonidos.
 *   4. Dibuja la tira y actualiza el display.
 *
 * Los periféricos que trabajan "solos" con interrupciones:
 *   Timer0 (1 ms): reloj, botones y display      → temporizador.c
 *   Timer1 + Timer2: tono y secuenciador de audio → sonido.c
 */
#include <avr/interrupt.h>
#include <avr/io.h>

#include "ajustes.h"
#include "display.h"
#include "efectos.h"
#include "juego.h"
#include "sonido.h"
#include "temporizador.h"
#include "tira.h"

/* Elige UN sonido según los eventos. Si pasan varias cosas a la vez suena
   la más importante (el buzzer sólo puede hacer un sonido por vez). */
static void sonar_eventos(uint8_t ev)
{
    if (ev & EV_DESCALIFICADO) {
        sonido_tocar(SND_DESCALIFICADO);
    } else if (ev & EV_GANADOR) {
        sonido_tocar(SND_FESTEJO);
    } else if (ev & EV_LARGADA) {
        sonido_tocar(SND_LARGADA);
    } else if (ev & EV_LUZ_SEMAFORO) {
        sonido_tocar(SND_LUZ);
    } else if (ev & (EV_VUELTA_J1 | EV_VUELTA_J2)) {
        sonido_tocar(SND_VUELTA);
    } else if (ev & EV_ARMANDO) {
        sonido_tocar(SND_ARMANDO);
    }
}

/* Motor: durante la carrera, el tono sigue la velocidad del auto que va
   adelante (regla de tres entre 0 y VELOCIDAD_MAXIMA). */
static void actualizar_motor(const juego_t *j)
{
    if (j->estado != EST_CARRERA) {
        sonido_motor(0);
        return;
    }
    uint32_t vel = juego_auto_lider(j)->velocidad;
    uint16_t hz = (uint16_t)(MOTOR_HZ_MIN +
                             vel * (MOTOR_HZ_MAX - MOTOR_HZ_MIN) / VELOCIDAD_MAXIMA);
    sonido_motor(hz);
}

int main(void)
{
    static juego_t juego; /* static: no ocupa lugar en la pila */

    tira_iniciar();
    display_iniciar();
    temporizador_iniciar();
    sonido_iniciar();
    sei(); /* a partir de acá corren las interrupciones */

    tira_apagar();
    tira_refrescar();
    juego_iniciar(&juego, temporizador_ms());

    for (;;) {
        uint32_t ahora = temporizador_ms();
        uint8_t botones = temporizador_botones();
        /* "Azar" para la largada: el contador del Timer0 mezclado con la
           hora. Depende de cuándo exactamente apretaron los jugadores. */
        uint16_t azar = (uint16_t)(ahora * 31u) ^ TCNT0;

        uint8_t eventos = juego_paso(&juego, botones, ahora, azar);
        sonar_eventos(eventos);
        actualizar_motor(&juego);

        efectos_dibujar(&juego, ahora);
        display_vueltas(juego.autos[0].vueltas, juego.autos[1].vueltas);
    }
}
