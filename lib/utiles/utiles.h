#include <Arduino.h>

#define NL 240
struct LED
{
  uint8_t R;
  uint8_t G;
  uint8_t B;
  uint8_t brightness;
};


void color_rojo(uint16_t pos);
void color_verde(uint16_t pos);
void color_azul(uint16_t pos);
void apagar_led(uint16_t pos);

void buzzer (void);
void mandar_byte(uint8_t val);
void mandar_start(void);
void mandar_stop(void);
void mandar_led(struct LED gyl);
void refresh_leds(void);
void off_leds(void);
void phaser_leds(struct LED led[]);
void enmascarado(uint8_t valor);
void multiplexado(uint16_t show_disp);
void jugador_1(struct LED led[]);
void jugador_2(struct LED led[]);
void efecto_ganar_1(struct LED led[]);
void efecto_ganar_2(struct LED led[]);
void boton_select(struct LED led[]);
void mi_codigo ();

void sonido_empieza(void);
void sonido_final(void);