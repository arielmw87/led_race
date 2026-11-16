/*
 * tira.c — protocolo APA102 hecho "a mano" (bit-banging) en PC4/PC5.
 *
 * El APA102 usa dos cables: DATOS y CLOCK. Para mandar un bit se pone el
 * valor en DATOS y se da un pulso en CLOCK; la tira lee el dato en el flanco
 * de subida del clock. Como el clock lo generamos nosotros, no importa qué
 * tan rápido o lento vayamos (a diferencia de las tiras WS2812, que exigen
 * tiempos exactos).
 *
 * Una actualización completa es:
 *   - trama de inicio: 4 bytes en 0,
 *   - por cada LED 4 bytes: 0b111 + brillo (5 bits), azul, verde, rojo,
 *   - trama de fin: 4 bytes (acá en 0, como en el código original).
 */
#include "tira.h"

#include <avr/io.h>

#define PIN_DATOS PC4
#define PIN_CLOCK PC5

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brillo;
} led_t;

/* La "memoria" de la tira: 240 LEDs × 4 bytes = 960 bytes, casi la mitad
   de la RAM del micro. Por eso hay uno solo. */
static led_t leds[NL];

void tira_iniciar(void)
{
    DDRC  |= (1 << PIN_DATOS) | (1 << PIN_CLOCK);
    PORTC &= ~((1 << PIN_DATOS) | (1 << PIN_CLOCK));
}

void tira_pintar(uint16_t pos, uint8_t r, uint8_t g, uint8_t b, uint8_t brillo)
{
    if (pos >= NL) {
        return; /* fuera de la tira: protegemos la memoria */
    }
    leds[pos].r = r;
    leds[pos].g = g;
    leds[pos].b = b;
    leds[pos].brillo = brillo;
}

void tira_apagar(void)
{
    for (uint16_t i = 0; i < NL; i++) {
        tira_pintar(i, 0, 0, 0, 0);
    }
}

void tira_atenuar(uint8_t corrimiento)
{
    for (uint16_t i = 0; i < NL; i++) {
        leds[i].r >>= corrimiento;
        leds[i].g >>= corrimiento;
        leds[i].b >>= corrimiento;
    }
}

/* Manda un byte, del bit más significativo al menos significativo. */
static void mandar_byte(uint8_t valor)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (valor & 0x80) {
            PORTC |= (1 << PIN_DATOS);
        } else {
            PORTC &= ~(1 << PIN_DATOS);
        }
        PORTC |= (1 << PIN_CLOCK);   /* flanco de subida: la tira lee el bit */
        PORTC &= ~(1 << PIN_CLOCK);
        valor <<= 1;
    }
}

static void mandar_trama_vacia(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        mandar_byte(0);
    }
}

void tira_refrescar(void)
{
    mandar_trama_vacia(); /* inicio */
    for (uint16_t i = 0; i < NL; i++) {
        mandar_byte(0xE0 | (leds[i].brillo & 0x1F));
        mandar_byte(leds[i].b);
        mandar_byte(leds[i].g);
        mandar_byte(leds[i].r);
    }
    mandar_trama_vacia(); /* fin */
}
