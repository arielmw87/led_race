/*
 * autos.c — física de los autos.
 *
 * ¿Por qué "punto fijo" y no float?
 *   El ATmega328P no tiene unidad de punto flotante: cada operación con float
 *   se hace por software, es lenta y ocupa mucha memoria de programa. En vez
 *   de eso guardamos la posición multiplicada por 65536. Por ejemplo,
 *   posicion = 3 * 65536 + 32768 significa "LED 3 y medio". Todo son sumas y
 *   restas de enteros, que el micro hace en pocos ciclos.
 *
 * ¿Por qué se calcula de a 1 ms?
 *   Así la física no depende de qué tan rápido gire el loop principal: el
 *   juego llama a auto_paso_1ms() tantas veces como milisegundos pasaron.
 *   Además, como la velocidad máxima es menor a 65536 (un LED por ms), el
 *   auto nunca puede saltearse un LED: el avance se ve continuo.
 */
#include "autos.h"
#include "ajustes.h"

#define LARGO_PISTA ((uint32_t)NL * AUTO_UNIDADES_POR_LED)

void auto_reiniciar(auto_t *a, uint8_t boton_ahora)
{
    a->posicion = 0;
    a->velocidad = 0;
    a->vueltas = 0;
    a->boton_antes = boton_ahora;
}

/* Impulso: sólo en el FLANCO de subida (cuando pasa de suelto a apretado).
   Si contáramos "mientras está apretado", mantener el botón aceleraría y no
   habría habilidad en pulsar rápido. */
static void aplicar_impulso(auto_t *a, uint8_t boton)
{
    uint8_t flanco = (boton && !a->boton_antes);
    a->boton_antes = boton;
    if (!flanco) {
        return;
    }
    uint32_t nueva = (uint32_t)a->velocidad + IMPULSO_POR_PULSACION;
    a->velocidad = (nueva > VELOCIDAD_MAXIMA) ? VELOCIDAD_MAXIMA : (uint16_t)nueva;
}

/* Rozamiento: cada ms se pierde una fracción de la velocidad (1/DIVISOR).
   Es como el aire: cuanto más rápido vas, más te frena. Redondeamos hacia
   arriba para que, a velocidades chicas, igual pierda al menos 1 y el auto
   termine frenando del todo (si no, 437/1024 daría 0 y nunca pararía). */
static void aplicar_rozamiento(auto_t *a)
{
    uint16_t perdida = (uint16_t)((a->velocidad + ROZAMIENTO_DIVISOR - 1) / ROZAMIENTO_DIVISOR);
    a->velocidad -= perdida;
}

uint8_t auto_paso_1ms(auto_t *a, uint8_t boton)
{
    aplicar_impulso(a, boton);
    aplicar_rozamiento(a);

    a->posicion += a->velocidad;
    if (a->posicion >= LARGO_PISTA) {
        /* Llegó al final de la tira: vuelve al principio y suma una vuelta.
           Restamos (en vez de poner 0) para no perder la fracción de LED. */
        a->posicion -= LARGO_PISTA;
        a->vueltas++;
        return 1;
    }
    return 0;
}

uint16_t auto_led(const auto_t *a)
{
    return (uint16_t)(a->posicion / AUTO_UNIDADES_POR_LED);
}
