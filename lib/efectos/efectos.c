/*
 * efectos.c — las animaciones de la tira.
 *
 * La regla de oro: NINGÚN efecto usa bucles de espera ni delay. Cada vez que
 * se llama, el efecto calcula cómo tiene que verse la tira EN ESTE INSTANTE
 * a partir de la hora (ahora_ms) y del estado del juego, la dibuja una vez
 * y vuelve. La "animación" aparece porque se llama muchas veces por segundo
 * y la hora va cambiando.
 */
#include "efectos.h"
#include "ajustes.h"
#include "tira.h"

/* Colores de cada jugador y del semáforo (r, g, b). */
#define VERDE  0, 255, 0
#define AZUL   0, 0, 255
#define ROJO   255, 0, 0

#define LARGO_ESTELA   5    /* LEDs de "cola" detrás de cada auto          */
#define MS_POR_COLOR   8    /* velocidad del arcoíris: más chico = más rápido */
#define DESTELLOS      3    /* destellos nuevos por cuadro en el festejo   */
#define SEPARACION_LUCES 4  /* LEDs apagados entre las luces del semáforo  */

/* "Rueda de colores": convierte un número 0-255 en un color del arcoíris
   (rojo → verde → azul → rojo). */
static void rueda(uint8_t posicion, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (posicion < 85) {
        *r = 255 - posicion * 3; *g = posicion * 3;       *b = 0;
    } else if (posicion < 170) {
        posicion -= 85;
        *r = 0;                  *g = 255 - posicion * 3; *b = posicion * 3;
    } else {
        posicion -= 170;
        *r = posicion * 3;       *g = 0;                  *b = 255 - posicion * 3;
    }
}

/* INVITACION: arcoíris que recorre toda la tira. */
static void dibujar_invitacion(uint32_t ahora)
{
    uint8_t corrimiento = (uint8_t)(ahora / MS_POR_COLOR);
    for (uint16_t i = 0; i < NL; i++) {
        uint8_t r, g, b;
        rueda((uint8_t)((uint32_t)i * 256 / NL) + corrimiento, &r, &g, &b);
        tira_pintar(i, r, g, b, BRILLO_TIRA);
    }
}

/* ARMADO: la tira se llena de rojo desde las dos puntas hacia el centro.
   Cuando se juntan en el medio, pasaron los 2 s. */
static void dibujar_armado(const juego_t *j, uint32_t ahora)
{
    uint32_t t = ahora - j->inicio_estado;
    if (t > ARMADO_MS) {
        t = ARMADO_MS;
    }
    uint16_t lleno = (uint16_t)(t * (NL / 2) / ARMADO_MS);
    tira_apagar();
    for (uint16_t i = 0; i < lleno; i++) {
        tira_pintar(i, ROJO, BRILLO_TIRA);
        tira_pintar(NL - 1 - i, ROJO, BRILLO_TIRA);
    }
}

/* SEMAFORO: la tira se divide en SEMAFORO_LUCES bloques; se prenden en rojo
   tantos bloques como luces haya. */
static void dibujar_semaforo(const juego_t *j)
{
    uint16_t largo_bloque = NL / SEMAFORO_LUCES;
    tira_apagar();
    for (uint8_t luz = 0; luz < j->luces; luz++) {
        uint16_t inicio = luz * largo_bloque;
        for (uint16_t i = 0; i < largo_bloque - SEPARACION_LUCES; i++) {
            tira_pintar(inicio + i, ROJO, BRILLO_TIRA);
        }
    }
}

/* Un auto con su estela: el LED del auto a pleno y atrás LEDs cada vez
   más tenues (cada uno con la mitad de color que el anterior). */
static void dibujar_auto(const auto_t *a, uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t pos = auto_led(a);
    for (uint8_t k = 0; k < LARGO_ESTELA; k++) {
        uint16_t led = (pos >= k) ? pos - k : pos + NL - k; /* da la vuelta */
        tira_pintar(led, r >> k, g >> k, b >> k, BRILLO_TIRA);
    }
}

/* CARRERA: los dos autos, cada uno con su color. */
static void dibujar_carrera(const juego_t *j)
{
    tira_apagar();
    dibujar_auto(&j->autos[1], AZUL);
    dibujar_auto(&j->autos[0], VERDE);
}

/* Generador de números "al azar" muy simple (LFSR), para los destellos. */
static uint16_t azar(void)
{
    static uint16_t lfsr = 0xBEEF;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
    return lfsr;
}

/* FESTEJO: destellos al azar del color del ganador que se van apagando,
   con un brillo que "late". */
static void dibujar_festejo(const juego_t *j, uint32_t ahora)
{
    uint8_t r = 0, g = 0, b = 0;
    if (j->ganador == 1) {
        g = 255;
    } else {
        b = 255;
    }
    /* Pulso: el brillo sube y baja en forma de triángulo, una vez por segundo. */
    uint16_t fase = ahora % 1000;
    uint8_t brillo = (uint8_t)(8 + ((fase < 500 ? fase : 1000 - fase) * (BRILLO_TIRA - 8)) / 500);

    tira_atenuar(1);
    for (uint8_t i = 0; i < DESTELLOS; i++) {
        tira_pintar(azar() % NL, r, g, b, brillo);
    }
}

void efectos_dibujar(const juego_t *j, uint32_t ahora_ms)
{
    switch (j->estado) {
    case EST_INVITACION: dibujar_invitacion(ahora_ms);   break;
    case EST_ARMADO:     dibujar_armado(j, ahora_ms);    break;
    case EST_SEMAFORO:   dibujar_semaforo(j);            break;
    case EST_CARRERA:    dibujar_carrera(j);             break;
    case EST_FESTEJO:    dibujar_festejo(j, ahora_ms);   break;
    }
    tira_refrescar();
}
