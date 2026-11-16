/*
 * ajustes.h — TODAS las constantes que se pueden tocar para ajustar el juego.
 *
 * La idea es que, al probar en la placa, no haga falta leer el resto del
 * código: si algo "se siente" mal, se busca acá la constante, se cambia, se
 * vuelve a compilar y se prueba de nuevo.
 *
 * Cada constante dice:
 *   - qué controla,
 *   - en qué unidad está,
 *   - qué pasa si la subís o la bajás.
 *
 * Nota: los valores iniciales están elegidos con cuentas razonables pero
 * nunca se probaron en el hardware real. Son un punto de partida.
 */
#ifndef AJUSTES_H
#define AJUSTES_H

/* ------------------------------------------------------------------ Tira -- */

/* Cantidad de LEDs de la tira. */
#define NL 240

/* Brillo global de la tira (0 a 31, es el brillo de 5 bits del APA102).
   Subirlo: todo más brillante. Bajarlo: más tenue (y consume menos). */
#define BRILLO_TIRA 31

/* --------------------------------------------------------------- Botones -- */

/* Milisegundos SEGUIDOS que un botón tiene que estar en un nivel nuevo para
   que lo creamos (filtra el "rebote" mecánico de los contactos).
   Subirlo: más robusto frente a botones gastados, pero responde más lento.
   Bajarlo: responde más rápido, pero un rebote puede contar como pulsación
   (o, en el semáforo, como soltada y descalificar a alguien). */
#define ANTIRREBOTE_MS 20

/* -------------------------------------------------------------- Largada -- */

/* Tiempo (ms) que los dos jugadores tienen que mantener apretado su botón
   para "armar" la largada. */
#define ARMADO_MS 2000

/* Cantidad de luces del semáforo y tiempo (ms) entre una luz y la siguiente. */
#define SEMAFORO_LUCES   5
#define SEMAFORO_PASO_MS 1000

/* Con todas las luces prendidas, se espera un tiempo AL AZAR entre estos dos
   valores (ms) antes de apagarlas. Que sea al azar es lo que impide "adivinar"
   la largada. Achicar el rango la hace más predecible. */
#define SEMAFORO_ESPERA_MIN_MS 200
#define SEMAFORO_ESPERA_MAX_MS 3000

/* Si durante el semáforo un jugador suelta y el otro suelta dentro de esta
   ventana (ms), se considera que soltaron los dos (nadie gana). Si el otro
   tarda más, el primero queda descalificado. */
#define GRACIA_SOLTAR_MS 200

/* -------------------------------------------------------------- Carrera -- */

/* Vueltas que hay que completar para ganar. */
#define VUELTAS_PARA_GANAR 3

/*
 * Física de los autos.
 *
 * Unidades (punto fijo, ver autos.c):
 *   posición:  1/65536 de LED      (65536 = un LED entero)
 *   velocidad: 1/65536 de LED por milisegundo
 *
 * Cada pulsación suma IMPULSO_POR_PULSACION a la velocidad. Cada milisegundo
 * el rozamiento le saca velocidad / ROZAMIENTO_DIVISOR (o sea, la velocidad
 * decae de forma exponencial, con una "constante de tiempo" de unos
 * ROZAMIENTO_DIVISOR milisegundos).
 *
 * Cuenta de referencia, pulsando 6 veces por segundo:
 *   velocidad de equilibrio ≈ IMPULSO × 6 × ROZAMIENTO_DIVISOR / 1000
 *                           ≈ 437 × 6 × 1.024 ≈ 2685  → ≈ 41 LED/s
 *   una vuelta de 240 LEDs ≈ 6 s  (+ lo que tarda en acelerar)
 *
 * Si las vueltas duran MUCHO: subí IMPULSO_POR_PULSACION.
 * Si duran POCO:              bajá IMPULSO_POR_PULSACION.
 * Si el auto frena demasiado rápido al dejar de pulsar: subí ROZAMIENTO_DIVISOR.
 * Si "patina" demasiado (sigue andando mucho sin pulsar): bajá ROZAMIENTO_DIVISOR.
 * VELOCIDAD_MAXIMA es el techo: 3932 ≈ 60 LED/s (vuelta de 4 s). Si los que
 * pulsan muy rápido no se diferencian lo suficiente, subilo (máx. 65535).
 */
#define IMPULSO_POR_PULSACION 437
#define ROZAMIENTO_DIVISOR    1024
#define VELOCIDAD_MAXIMA      3932

/* --------------------------------------------------------------- Festejo -- */

/* Duración del festejo (ms). Antes del mínimo se ignoran los botones (para
   que nadie lo saltee sin querer por seguir apretando); al llegar al máximo
   se vuelve solo a la invitación. */
#define FESTEJO_MIN_MS 3000
#define FESTEJO_MAX_MS 20000

/* ------------------------------------------------- Sonido de motor -------- */
/* (Sólo con buzzer pasivo, env nano_pasivo.) */

/* Frecuencia (Hz) del motor parado (ralentí) y a velocidad máxima.
   Subir MOTOR_HZ_MIN: el ralentí suena más agudo. Subir MOTOR_HZ_MAX: el
   "a fondo" suena más agudo. No bajar MOTOR_HZ_MIN de 245 (límite del timer). */
#define MOTOR_HZ_MIN 300
#define MOTOR_HZ_MAX 1500

/* Cuánto (Hz) se mueve el tono hacia arriba y abajo cada 10 ms para que suene
   "áspero" como un motor. 0 = pitido limpio. Más alto = más ronco/vibrante. */
#define MOTOR_VIBRATO_HZ 40

/* Volumen del motor (0 a 255). Bajarlo si tapa a los demás sonidos. */
#define MOTOR_VOLUMEN 200

#endif /* AJUSTES_H */
