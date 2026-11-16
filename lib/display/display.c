/*
 * display.c — multiplexado del display de 7 segmentos.
 *
 * Los 4 dígitos comparten los mismos 8 cables de segmentos. Para mostrar
 * números distintos en cada uno se prende UN dígito por vez, muy rápido
 * (un dígito por milisegundo). El ojo no llega a ver el parpadeo y parece
 * que los cuatro están prendidos a la vez. Esto se llama "multiplexar".
 *
 * Como el display es de ánodo común, un segmento se prende poniendo su pin
 * en 0 (por eso se invierte la tabla con ~) y un dígito se habilita
 * poniendo su pin en 1.
 *
 * Segmentos: bits 0-1 en PB0-PB1 y bits 2-7 en PD2-PD7.
 * Dígitos:   PC0 (izquierda) a PC3 (derecha).
 */
#include "display.h"

#include <avr/io.h>

#define DIGITO_APAGADO 10
#define CANT_DIGITOS   4

/* Qué segmentos prender para cada número (bit 1 = segmento prendido). */
static const uint8_t SEGMENTOS[] = {
    0b00111111, /* 0 */
    0b00000110, /* 1 */
    0b01011011, /* 2 */
    0b01001111, /* 3 */
    0b01100110, /* 4 */
    0b01101101, /* 5 */
    0b01111101, /* 6 */
    0b00000111, /* 7 */
    0b01111111, /* 8 */
    0b01101111, /* 9 */
    0b00000000  /* DIGITO_APAGADO */
};

/* Lo que se muestra en cada dígito. La interrupción lo lee, por eso volatile. */
static volatile uint8_t digitos[CANT_DIGITOS] = {
    0, DIGITO_APAGADO, DIGITO_APAGADO, 0
};

void display_iniciar(void)
{
    DDRB |= (1 << PB0) | (1 << PB1);
    DDRD |= 0b11111100;
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);
}

void display_vueltas(uint8_t j1, uint8_t j2)
{
    digitos[0] = (j1 > 9) ? 9 : j1;
    digitos[3] = (j2 > 9) ? 9 : j2;
}

/* Pone en los pines los segmentos del valor (0-9 o DIGITO_APAGADO). */
static void poner_segmentos(uint8_t valor)
{
    uint8_t pines = (uint8_t)~SEGMENTOS[valor]; /* ánodo común: 0 = prendido */
    PORTB = (PORTB & 0b11111100) | (pines & 0b00000011);
    PORTD = (PORTD & 0b00000011) | (pines & 0b11111100);
}

void display_multiplexar(void)
{
    static uint8_t actual = 0;

    /* Orden importante: apagar el dígito anterior, cambiar los segmentos y
       recién ahí prender el nuevo. Si no, se ve un "fantasma" del número
       anterior en el dígito nuevo. */
    PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
    poner_segmentos(digitos[actual]);
    PORTC |= (1 << (PC0 + actual));

    actual = (actual + 1) % CANT_DIGITOS;
}
