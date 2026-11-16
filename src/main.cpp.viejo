#include <Arduino.h>
#include <utiles.h>

int main()
{
  // Configurar Timer 0 en modo CTC –- interrupci ón cada 1 ms
  // F_CPU = 16 MHz , preescalador = 64 , OCR0A = 249
  TCCR0A = (1 << WGM01);              // modo CTC
  TCCR0B = (1 << CS01) | (1 << CS00); // preescalador 64
  OCR0A = 249;                        // comparación: interrupt cada 1 ms
  DDRC |= (1 << PC4) | (1 << PC5) | (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);
  DDRD |= (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6);
  DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB5);
  DDRB &= ~((1 << PB4) | (1 << PB3) | (1 << PB2));
  DDRD |= (1 << PD7);
  PORTD &= ~(PD7);
  PORTC &= ~((1 << PC4) | (1 << PC5));
  PORTB |= (1 << PB2) | (1 << PB3) | (1 << PB4);
  // Habilitar interrupción por comparación A
  TIMSK0 = (1 << OCIE0A);
  sei();

  off_leds();
sonido_empieza();
  while (1)
  {
    mi_codigo();
  }
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
