/*
 * sonido.h — motor de sonido del juego.
 *
 * Este módulo se portó del proyecto rfid-reader y se deja como "caja negra":
 * no hace falta entender cómo funciona por dentro para usarlo. Lo importante:
 *
 *   sonido_iniciar();            // una vez, al arrancar
 *   sonido_tocar(SND_VUELTA);    // dispara un sonido y vuelve ENSEGUIDA
 *   sonido_motor(800);           // tono continuo de motor (0 = apagado)
 *
 * Los sonidos suenan "en segundo plano" usando dos timers del micro, así
 * que el programa principal nunca se queda esperando a que terminen.
 *
 * Buzzer (en PB2, se elige al compilar con el env de PlatformIO):
 *   BUZZER_PASIVO: el micro genera el tono (notas, glissandos, motor).
 *   BUZZER_ACTIVO: el buzzer trae su oscilador y sólo se prende y apaga;
 *                  suena el "ritmo" de cada sonido y no hay motor.
 *
 * Para agregar un sonido: sumar un SND_ antes de SND_CANTIDAD, escribir su
 * tabla de notas en sonido.c y agregarla al arreglo SONIDOS.
 */
#ifndef SONIDO_H
#define SONIDO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SND_NINGUNO,       /* silencio                                  */
    SND_LUZ,           /* bip de cada luz del semáforo              */
    SND_LARGADA,       /* bip agudo largo al apagarse las luces     */
    SND_VUELTA,        /* "pip-pip" al completar una vuelta         */
    SND_DESCALIFICADO, /* trombón triste                            */
    SND_FESTEJO,       /* fanfarria de victoria                     */
    SND_ARMANDO,       /* tic suave al empezar a armar la largada   */
    SND_CANTIDAD
};

/* Configura el pin del buzzer y los timers 1 y 2. */
void sonido_iniciar(void);

/* Empieza un sonido del catálogo (corta el que estaba sonando) y vuelve
   enseguida. SND_NINGUNO o ids inválidos se ignoran. */
void sonido_tocar(uint8_t id);

/* Sonido de motor: tono continuo de "hz" Hz con vibración. 0 lo apaga.
   Los sonidos del catálogo lo interrumpen y al terminar el motor vuelve.
   Con BUZZER_ACTIVO no hace nada. */
void sonido_motor(uint16_t hz);

/* 1 mientras suena un sonido del catálogo. */
uint8_t sonido_sonando(void);

#ifdef __cplusplus
}
#endif

#endif /* SONIDO_H */
