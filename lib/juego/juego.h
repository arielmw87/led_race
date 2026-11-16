/*
 * juego.h — la máquina de estados del juego (ver juego.c para la explicación).
 *
 * Este módulo no toca el hardware: recibe qué botones están apretados y la
 * hora actual, y devuelve qué pasó (eventos). Por eso se puede probar en la PC.
 */
#ifndef JUEGO_H
#define JUEGO_H

#include <stdint.h>
#include "autos.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EST_INVITACION, /* animación esperando jugadores                       */
    EST_ARMADO,     /* los dos mantienen apretado; se cuenta hasta 2 s     */
    EST_SEMAFORO,   /* luces rojas tipo F1; soltar antes = descalificado   */
    EST_CARRERA,    /* los autos corren                                    */
    EST_FESTEJO     /* luces y música del ganador                          */
} estado_t;

/* Entradas: una máscara de bits con los botones apretados (ya sin rebote). */
enum { BOTON_J1 = 1, BOTON_J2 = 2, BOTON_SELECT = 4 };

/* Eventos: lo que pasó en un paso. El programa principal los usa para
   disparar sonidos. Pueden venir varios juntos (máscara de bits). */
enum {
    EV_ARMANDO       = 1,   /* los dos empezaron a mantener apretado */
    EV_LUZ_SEMAFORO  = 2,   /* se prendió una luz del semáforo       */
    EV_LARGADA       = 4,   /* se apagaron las luces: largaron       */
    EV_VUELTA_J1     = 8,   /* el jugador 1 completó una vuelta      */
    EV_VUELTA_J2     = 16,  /* el jugador 2 completó una vuelta      */
    EV_DESCALIFICADO = 32,  /* alguien soltó antes de la largada     */
    EV_GANADOR       = 64,  /* hay ganador: empieza el festejo       */
    EV_A_INVITACION  = 128  /* se volvió a la invitación             */
};

typedef struct {
    estado_t estado;
    uint32_t inicio_estado;   /* ms en que se entró al estado actual              */
    uint8_t  luces;           /* luces del semáforo prendidas (0..SEMAFORO_LUCES) */
    uint16_t espera_apagado;  /* ms al azar con todas las luces prendidas         */
    uint32_t primera_soltada; /* ms de la primera soltada en el semáforo          */
    uint8_t  quien_solto;     /* 0 = nadie, 1 o 2                                 */
    uint8_t  esperar_soltar;  /* 1 = no se arma hasta que se suelte todo          */
    uint8_t  ganador;         /* 0 = nadie, 1 o 2                                 */
    uint8_t  lider;           /* 1 o 2: el que va adelante                        */
    auto_t   autos[2];        /* autos[0] = jugador 1, autos[1] = jugador 2       */
    uint8_t  entradas_antes;  /* botones en el paso anterior (para flancos)       */
    uint32_t ultimo_paso;     /* ms hasta donde se calculó la física              */
} juego_t;

/* Deja el juego en INVITACION. */
void juego_iniciar(juego_t *j, uint32_t ahora_ms);

/* Llamar en cada vuelta del loop principal.
   entradas: máscara BOTON_*. ahora_ms: reloj en milisegundos.
   azar: cualquier número que varíe (se usa al entrar al semáforo).
   Devuelve la máscara de eventos EV_* que ocurrieron. */
uint8_t juego_paso(juego_t *j, uint8_t entradas, uint32_t ahora_ms, uint16_t azar);

/* El auto que va adelante (para el sonido del motor). */
const auto_t *juego_auto_lider(const juego_t *j);

#ifdef __cplusplus
}
#endif

#endif /* JUEGO_H */
