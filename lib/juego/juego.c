/*
 * juego.c — la MÁQUINA DE ESTADOS del juego.
 *
 * ¿Qué es una máquina de estados?
 *   Es una forma de organizar un programa que "pasa por etapas". En cada
 *   momento el juego está en UN estado (invitación, armado, semáforo,
 *   carrera o festejo). Cada estado sabe:
 *     - qué hacer mientras dura,
 *     - qué está esperando que pase,
 *     - a qué otro estado pasar cuando eso pasa (una "transición").
 *
 * ¿Por qué usarla acá?
 *   La versión anterior hacía las animaciones con bucles for que se quedaban
 *   "trabados" hasta terminar: mientras tanto no se podía leer botones ni
 *   hacer otra cosa. Acá, juego_paso() se llama muchísimas veces por segundo,
 *   mira la hora y los botones, hace un poquito de trabajo y vuelve enseguida.
 *   Nunca espera. Así el mismo programa puede mover los autos, sonar, dibujar
 *   los LEDs y atender los botones "a la vez".
 *
 * Diagrama de estados:
 *
 *     ┌────────────┐  J1 y J2 apretados   ┌─────────┐
 *     │ INVITACION │─────────────────────▶│ ARMADO  │
 *     │            │◀─ alguno suelta ─────│ (2 s)   │
 *     └────────────┘   antes de 2 s       └────┬────┘
 *        ▲   ▲                                 │ pasaron 2 s
 *        │   │  los dos sueltan           ┌────▼─────┐
 *        │   └──(dentro de 200 ms)────────│ SEMAFORO │
 *        │                                └────┬─────┘
 *        │      uno suelta: gana el otro ──────┤ (al FESTEJO)
 *        │                                     │ se apagan las luces
 *        │                                ┌────▼─────┐
 *        │                                │ CARRERA  │
 *        │                                └────┬─────┘
 *        │                                     │ alguien completa 3 vueltas
 *        │   botón (después de 3 s)       ┌────▼─────┐
 *        └──────── o pasaron 20 s ────────│ FESTEJO  │
 *                                         └──────────┘
 *   Además, el botón SELECT en ARMADO, SEMAFORO o CARRERA vuelve a INVITACION.
 *
 * Sobre el tiempo: todas las comparaciones son del tipo
 *   (ahora - inicio) >= duracion
 * con enteros sin signo. Así funcionan bien aunque el reloj de milisegundos
 * dé la vuelta (pasa de 4294967295 a 0 cada ~49 días).
 */
#include "juego.h"
#include "ajustes.h"

#define AMBOS (BOTON_J1 | BOTON_J2)

/* ----------------------------------------------------------- utilidades -- */

static void cambiar_estado(juego_t *j, estado_t nuevo, uint32_t ahora)
{
    j->estado = nuevo;
    j->inicio_estado = ahora;
}

static uint32_t tiempo_en_estado(const juego_t *j, uint32_t ahora)
{
    return ahora - j->inicio_estado;
}

/* Volver a la invitación. esperar_soltar = 1 obliga a soltar TODOS los
   botones antes de poder armar otra vez: si los jugadores siguen apretando
   cuando termina una carrera, no queremos que se arme sola otra largada. */
static uint8_t ir_a_invitacion(juego_t *j, uint32_t ahora)
{
    cambiar_estado(j, EST_INVITACION, ahora);
    j->esperar_soltar = 1;
    return EV_A_INVITACION;
}

static uint8_t ir_a_festejo(juego_t *j, uint8_t ganador, uint32_t ahora)
{
    cambiar_estado(j, EST_FESTEJO, ahora);
    j->ganador = ganador;
    return EV_GANADOR;
}

/* -------------------------------------------------------------- estados -- */

/* INVITACION: la tira muestra una animación (eso lo hace efectos.c) y
   esperamos a que los dos jugadores aprieten su botón. */
static uint8_t estado_invitacion(juego_t *j, uint8_t entradas, uint32_t ahora)
{
    if (entradas == 0) {
        j->esperar_soltar = 0;
    }
    if (!j->esperar_soltar && (entradas & AMBOS) == AMBOS) {
        cambiar_estado(j, EST_ARMADO, ahora);
        return EV_ARMANDO;
    }
    return 0;
}

static uint8_t entrar_al_semaforo(juego_t *j, uint32_t ahora, uint16_t azar)
{
    cambiar_estado(j, EST_SEMAFORO, ahora);
    j->luces = 1; /* la primera luz se prende apenas se entra */
    j->quien_solto = 0;
    j->ganador = 0;
    /* El tiempo que quedan todas prendidas se elige AL AZAR para que nadie
       pueda "adivinar" la largada contando. */
    j->espera_apagado = SEMAFORO_ESPERA_MIN_MS +
        azar % (SEMAFORO_ESPERA_MAX_MS - SEMAFORO_ESPERA_MIN_MS + 1);
    return EV_LUZ_SEMAFORO;
}

/* ARMADO: los dos mantienen apretado. Si aguantan ARMADO_MS, arranca el
   semáforo. Si alguno suelta antes, no pasa nada: se vuelve a invitación. */
static uint8_t estado_armado(juego_t *j, uint8_t entradas, uint32_t ahora, uint16_t azar)
{
    if ((entradas & AMBOS) != AMBOS) {
        cambiar_estado(j, EST_INVITACION, ahora); /* sin penalidad */
        return 0;
    }
    if (tiempo_en_estado(j, ahora) >= ARMADO_MS) {
        return entrar_al_semaforo(j, ahora, azar);
    }
    return 0;
}

/* En el semaforo, soltar el botón antes de tiempo es "largada en falso".
   Como dos personas nunca sueltan en el mismo milisegundo exacto, después
   de la primera soltada damos GRACIA_SOLTAR_MS: si el otro también suelta
   en ese rato, se toma como que soltaron los dos (nadie gana).
   Devuelve 1 si ya se resolvió el estado (cambió), y deja eventos en *ev. */
static uint8_t revisar_soltadas(juego_t *j, uint8_t entradas, uint32_t ahora, uint8_t *ev)
{
    uint8_t suelto1 = !(entradas & BOTON_J1);
    uint8_t suelto2 = !(entradas & BOTON_J2);

    if (j->quien_solto == 0) {
        if (suelto1 && suelto2) {          /* soltaron en el mismo ms */
            *ev |= ir_a_invitacion(j, ahora);
            return 1;
        }
        if (suelto1 || suelto2) {
            j->quien_solto = suelto1 ? 1 : 2;
            j->primera_soltada = ahora;
        }
        return 0;
    }

    uint8_t otro_suelto = (j->quien_solto == 1) ? suelto2 : suelto1;
    if (otro_suelto) {                     /* el otro soltó dentro de la gracia */
        *ev |= ir_a_invitacion(j, ahora);
        return 1;
    }
    if (ahora - j->primera_soltada >= GRACIA_SOLTAR_MS) {
        uint8_t ganador = (j->quien_solto == 1) ? 2 : 1;
        *ev |= EV_DESCALIFICADO | ir_a_festejo(j, ganador, ahora);
        return 1;
    }
    return 0;
}

static uint8_t largar(juego_t *j, uint8_t entradas, uint32_t ahora)
{
    cambiar_estado(j, EST_CARRERA, ahora);
    /* Le pasamos a cada auto el estado actual de su botón: como lo tienen
       apretado desde el armado, eso NO cuenta como pulsación. El auto
       arranca recién cuando sueltan y vuelven a apretar. */
    auto_reiniciar(&j->autos[0], (entradas & BOTON_J1) ? 1 : 0);
    auto_reiniciar(&j->autos[1], (entradas & BOTON_J2) ? 1 : 0);
    j->ultimo_paso = ahora;
    j->lider = 1;
    return EV_LARGADA;
}

/* SEMAFORO: se prende una luz roja cada SEMAFORO_PASO_MS. Con todas
   prendidas se espera un tiempo al azar y se apagan: ¡largada! */
static uint8_t estado_semaforo(juego_t *j, uint8_t entradas, uint32_t ahora)
{
    uint8_t ev = 0;
    if (revisar_soltadas(j, entradas, ahora, &ev) || j->quien_solto != 0) {
        return ev; /* resuelto, o esperando que venza la gracia */
    }

    uint32_t t = tiempo_en_estado(j, ahora);
    uint32_t luces = 1 + t / SEMAFORO_PASO_MS;
    if (luces > SEMAFORO_LUCES) {
        luces = SEMAFORO_LUCES;
    }
    if (luces != j->luces) {
        j->luces = (uint8_t)luces;
        ev |= EV_LUZ_SEMAFORO;
    }

    uint32_t momento_largada = (uint32_t)(SEMAFORO_LUCES - 1) * SEMAFORO_PASO_MS + j->espera_apagado;
    if (t >= momento_largada) {
        ev |= largar(j, entradas, ahora);
    }
    return ev;
}

/* Líder: el que tiene más vueltas, o con las mismas vueltas, más posición.
   Si están exactamente empatados se queda el anterior; si no, el sonido del
   motor saltaría de un auto al otro. */
static void actualizar_lider(juego_t *j)
{
    const auto_t *a1 = &j->autos[0];
    const auto_t *a2 = &j->autos[1];
    if (a1->vueltas != a2->vueltas) {
        j->lider = (a1->vueltas > a2->vueltas) ? 1 : 2;
    } else if (a1->posicion != a2->posicion) {
        j->lider = (a1->posicion > a2->posicion) ? 1 : 2;
    }
}

/* CARRERA: la física se calcula de a 1 ms, tantos ms como hayan pasado
   desde la última vez (el loop principal puede tardar varios ms en dar una
   vuelta porque mandar los 240 LEDs lleva tiempo). */
static uint8_t estado_carrera(juego_t *j, uint8_t entradas, uint32_t ahora)
{
    uint8_t ev = 0;
    uint8_t b1 = (entradas & BOTON_J1) ? 1 : 0;
    uint8_t b2 = (entradas & BOTON_J2) ? 1 : 0;

    while (ahora - j->ultimo_paso > 0) {
        j->ultimo_paso++;
        if (auto_paso_1ms(&j->autos[0], b1)) {
            ev |= EV_VUELTA_J1;
        }
        if (auto_paso_1ms(&j->autos[1], b2)) {
            ev |= EV_VUELTA_J2;
        }
        /* Se revisa primero J1: si los dos llegan en el mismo ms, gana J1. */
        if (j->autos[0].vueltas >= VUELTAS_PARA_GANAR) {
            return ev | ir_a_festejo(j, 1, ahora);
        }
        if (j->autos[1].vueltas >= VUELTAS_PARA_GANAR) {
            return ev | ir_a_festejo(j, 2, ahora);
        }
    }
    actualizar_lider(j);
    return ev;
}

/* FESTEJO: luces y música del ganador. Durante los primeros FESTEJO_MIN_MS
   se ignoran los botones (los jugadores suelen seguir apretando al cruzar
   la meta y no queremos que se saltee el festejo sin querer). Después,
   cualquier pulsación nueva vuelve a la invitación; si nadie toca nada,
   se vuelve sola a los FESTEJO_MAX_MS. */
static uint8_t estado_festejo(juego_t *j, uint8_t pulsadas, uint32_t ahora)
{
    uint32_t t = tiempo_en_estado(j, ahora);
    if (t >= FESTEJO_MAX_MS) {
        return ir_a_invitacion(j, ahora);
    }
    if (t >= FESTEJO_MIN_MS && pulsadas) {
        return ir_a_invitacion(j, ahora);
    }
    return 0;
}

/* --------------------------------------------------------------- público -- */

void juego_iniciar(juego_t *j, uint32_t ahora_ms)
{
    cambiar_estado(j, EST_INVITACION, ahora_ms);
    j->luces = 0;
    j->espera_apagado = 0;
    j->primera_soltada = 0;
    j->quien_solto = 0;
    j->esperar_soltar = 0;
    j->ganador = 0;
    j->lider = 1;
    auto_reiniciar(&j->autos[0], 0);
    auto_reiniciar(&j->autos[1], 0);
    j->entradas_antes = 0;
    j->ultimo_paso = ahora_ms;
}

uint8_t juego_paso(juego_t *j, uint8_t entradas, uint32_t ahora_ms, uint16_t azar)
{
    /* "pulsadas" = botones que en este paso pasaron de suelto a apretado. */
    uint8_t pulsadas = entradas & (uint8_t)~j->entradas_antes;
    j->entradas_antes = entradas;

    /* SELECT aborta una partida en curso. */
    uint8_t en_partida = (j->estado == EST_ARMADO || j->estado == EST_SEMAFORO ||
                          j->estado == EST_CARRERA);
    if (en_partida && (entradas & BOTON_SELECT)) {
        return ir_a_invitacion(j, ahora_ms);
    }

    switch (j->estado) {
    case EST_INVITACION: return estado_invitacion(j, entradas, ahora_ms);
    case EST_ARMADO:     return estado_armado(j, entradas, ahora_ms, azar);
    case EST_SEMAFORO:   return estado_semaforo(j, entradas, ahora_ms);
    case EST_CARRERA:    return estado_carrera(j, entradas, ahora_ms);
    case EST_FESTEJO:    return estado_festejo(j, pulsadas, ahora_ms);
    }
    return 0;
}

const auto_t *juego_auto_lider(const juego_t *j)
{
    return &j->autos[j->lider == 2 ? 1 : 0];
}
