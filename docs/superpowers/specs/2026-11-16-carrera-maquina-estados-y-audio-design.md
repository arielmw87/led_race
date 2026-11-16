# Carrera de LEDs: máquina de estados, física de autos y motor de audio

Fecha: 2026-11-16 · Rama: `dev`

## Intención

El juego actual (escrito por un alumno) funciona, pero su dinámica es confusa: cada pulsación salta 10 LEDs, "llegar al final" suma un punto, las animaciones bloquean el programa, y el buzzer activo sólo hace pitidos. Queremos:

1. Un juego con una estructura clara de carrera: invitación a jugar → largada estilo F1 → 3 vueltas → festejo del ganador.
2. Que la velocidad del auto dependa de la **habilidad** (qué tan rápido pulsás y soltás) y que el avance se vea **continuo**, no a saltos.
3. Sonido de motor que sube de tono con la velocidad, usando el motor de audio del proyecto `C:\Proyectos\rfid-reader` (`lib/buzzer/`), con soporte para buzzer pasivo y activo.
4. **Que el código sirva como material didáctico** para el alumno: la máquina de estados y cada decisión de diseño explicadas en comentarios claros, en español. Este objetivo pesa tanto como la funcionalidad.

## Objetivos

- Máquina de estados no bloqueante para todo el flujo del juego.
- Física simple con inercia (velocidad + rozamiento) en punto fijo.
- Motor de audio portado del rfid-reader, compilable para buzzer pasivo o activo.
- Código comentado didácticamente en español.

## No-objetivos

- Modo de un solo jugador. Si alguien quiere probar con un auto, aprieta los dos botones y hace la largada completa.
- Cambios de hardware (excepto montar un buzzer pasivo en lugar del activo, en el mismo pin).
- Configuración en tiempo de ejecución (volumen, cantidad de vueltas, etc.): todo son constantes en código.

## Anti-objetivos (sería un fracaso aunque "funcione")

- Código ingenioso pero difícil de leer para un alumno de secundaria técnica.
- Cualquier `for`/espera que bloquee el loop principal durante animaciones o sonidos.
- Avance visiblemente a saltos de varios LEDs.
- Que el sonido del motor favorezca siempre a un jugador fijo.

## Restricciones

- ATmega328P (Arduino Nano) a 16 MHz, PlatformIO, `int main()` propio (sin `setup/loop`).
- Alimentación: fuente de 40 A para la tira (decidido con el usuario). Alcanza para la tira entera a brillo máximo, así que las animaciones no necesitan límite de corriente.
- RAM: hoy se usan 1047 de 2048 bytes (el arreglo `led[240]` ocupa 960). No duplicar el buffer de LEDs.
- **Timer0** ya está tomado: interrupción de 1 ms que multiplexa el display, cuenta tiempo (`hola`) y filtra el rebote de los botones. Se mantiene así. `millis()`/`delay()` no existen.
- **Timer1 y Timer2** están libres: los usa el motor de audio.
- **Cambio de cableado (decidido con el usuario):** buzzer en **PB2** (OC1B, salida PWM del Timer1); botón J1 en **PB3**, J2 en **PB4**, select en **PB5** (D13). D13 tiene el LED de la placa: el select necesita **pull-up externo de ~4,7 kΩ** a 5 V.
- Pines: botones PB3 (J1), PB4 (J2), PB5 (select), activos en bajo con pull-up; tira APA102/SK9822 en PC4 (datos) / PC5 (clock); display de 4 dígitos ánodo común, segmentos PB0–PB1 y PD2–PD7, dígitos PC0–PC3.
- Colores: jugador 1 = verde, jugador 2 = azul. El rojo no pertenece a ningún auto y se usa para el semáforo.

## Diseño

### Flujo del juego (decidido con el usuario)

```
            ┌─────────────────────────────────────────────┐
            ▼                                             │
     ┌─────────────┐  ambos botones     ┌──────────┐      │
     │ INVITACION  │──apretados────────▶│  ARMADO  │      │
     │ (animación) │◀──alguno suelta────│ (2 s)    │      │
     └─────────────┘   antes de 2 s     └────┬─────┘      │
            ▲                                │ 2 s        │
            │                                ▼            │
            │  ambos sueltan            ┌──────────┐      │
            ├──antes de tiempo──────────│ SEMAFORO │      │
            │                           │ (F1)     │      │
            │                           └────┬─────┘      │
            │       uno suelta antes ────────┤            │
            │       (gana el otro)           │ luces      │
            │                                │ apagadas   │
            │                                ▼            │
            │                           ┌──────────┐      │
            │                           │ CARRERA  │      │
            │                           └────┬─────┘      │
            │                                │ alguien    │
            │                                │ completa 3 │
            │                                ▼ vueltas    │
            │ botón (después de 3 s)    ┌──────────┐      │
            └───── o 20 s ──────────────│ FESTEJO  │◀─────┘
                                        └──────────┘  (descalificación)
```

- **INVITACION:** animación sencilla que invite a jugar, como un fader/arcoíris (puede partir de `phaser_leds()`). Espera a que ambos jugadores mantengan apretado su botón.
- **ARMADO:** ambos botones apretados durante 2 s seguidos. Si alguno suelta antes, se vuelve a INVITACION sin penalidad *(decisión del diseñador)*. Conviene un indicio visual de que se está armando.
- **SEMAFORO:** secuencia tipo largada de F1 en rojo, con sonido. Mi propuesta: 5 bloques rojos que se prenden de a uno por segundo, cada uno con un bip; después todos quedan prendidos un tiempo **aleatorio** (entre 0,2 y 3 s) y se apagan juntos. En ese momento arranca la carrera. *(Los detalles de la secuencia son decisión del diseñador.)*
  - Si **un** jugador suelta antes de que se apaguen las luces, queda **descalificado** y **gana el otro**: se pasa a FESTEJO del otro.
  - Si **ambos** sueltan antes, no gana nadie y se vuelve a INVITACION *(decisión del diseñador)*.
  - *(Decisión del diseñador)* Detectar el "soltar":
    - Se exige que el botón esté suelto unos 20 ms seguidos, igual que la pulsación. Hoy el filtro del Timer0 sólo filtra la pulsación, y un rebote de 1 ms descalificaría a alguien.
    - Después de la primera soltada hay una ventana de gracia de ~200 ms: si el otro también suelta dentro de esa ventana, cuenta como "ambos sueltan". Si no, se descalifica al primero.
- **CARRERA:** los autos aceleran con cada pulsación (ver física). Cada vez que un auto completa una vuelta suena un "pip" de vuelta y se actualiza el display. Al completar **3 vueltas** gana.
- **FESTEJO:** efecto de luces con el color del ganador más sonidos de victoria. Dura **mínimo 3 s**: los botones se ignoran para que nadie lo saltee sin querer al seguir apretando. Dura **máximo 20 s**. Pasados los 3 s, tocar cualquier botón lleva a INVITACION; si nadie toca nada, a los 20 s pasa solo.
- **Botón select** *(decisión del diseñador)*: en ARMADO, SEMAFORO o CARRERA aborta y vuelve a INVITACION. En los demás estados se comporta como cualquier botón.

### Display *(decisión del diseñador)*

Muestra las vueltas completadas: dígito izquierdo = jugador 1, dígito derecho = jugador 2, dígitos del medio apagados. En INVITACION muestra el resultado de la última carrera (al encender, 0 y 0). Al pasar a SEMAFORO se pone en 0–0.

### Física de los autos (decidido con el usuario, números a ajustar)

- La velocidad crece cuanto más rápido se **pulsa y suelta**: cada flanco de pulsación suma un impulso de velocidad. Hay rozamiento constante que la va bajando y un tope máximo.
- La posición avanza según la velocidad en cada paso de tiempo (derivado de `hola`), en **punto fijo** (por ejemplo 1/256 de LED), así el LED avanza de a uno, de forma continua.
- Valor inicial orientativo: una vuelta pulsando a buen ritmo dura unos 5–8 s. Los parámetros (impulso, rozamiento, velocidad máxima) son constantes con nombre, comentadas, para ajustarlas probando.
- Al pasar de `NL-1` la posición vuelve a 0 y se cuenta la vuelta.
- Mantener apretado el botón no acelera: sólo cuenta el flanco.

### Audio

**Motor portado del rfid-reader** (`C:\Proyectos\rfid-reader\lib\buzzer\buzzer.c`):
- **Timer1** genera el tono por PWM de hardware en PB2 (OC1B), como la variante 328P del original pero usando OC1B (`COM1B1`, `OCR1B`) en vez de OC1A. Así el tono sale limpio sin interrupciones por ciclo.
- **Timer2** hace de secuenciador cada 10 ms: notas, glides, fades y ruido.
- Suena en segundo plano: el juego llama "tocar sonido X" y sigue.
- Se puede recortar el catálogo a los sonidos que se usen y agregar los nuevos.
- **Se porta casi tal cual, como caja negra** (decidido con el usuario): se traducen los comentarios al español y se agrega una explicación general arriba de qué hace y cómo se usa, pero no se reescribe en estilo didáctico. El objetivo didáctico aplica al resto del código.

**Dos envs de compilación** en `platformio.ini`, con el mismo pin PB2 (decidido con el usuario):
- `nano_pasivo` con `BUZZER_PASIVO`: tonos completos y motor.
- `nano_activo` con `BUZZER_ACTIVO`: mismo catálogo y misma API, pero PB2 se prende mientras una nota tiene volumen y se apaga en los silencios, o sea la versión rítmica de cada sonido. No hay sonido de motor.
- El env por defecto es `nano_activo`, porque es el hardware montado hoy *(decisión del diseñador)*.

**Sonido de motor (sólo en pasivo):**
- Tono continuo cuya frecuencia sale de la velocidad del auto **que va adelante** (más vueltas, o igual cantidad y más posición). En empate se mantiene el último que iba adelante. Nunca un auto fijo.
- Parado suena un ralentí grave y a velocidad máxima un tono agudo (rango orientativo 300–1500 Hz; el motor tiene un mínimo de unos 245 Hz con prescaler 1).
- Para que suene a motor y no a pitido limpio lleva algo de vibración: pulsos rápidos de volumen o pequeñas variaciones de frecuencia.
- Suena sólo en CARRERA. Los sonidos puntuales (pip de vuelta) lo interrumpen y al terminar el motor vuelve.

**Sonidos puntuales:**
- Bip por cada luz del semáforo y un sonido distinto al apagarse (largada).
- Pip de vuelta.
- Sonido de descalificación (por ejemplo, el "trombón triste" del catálogo).
- Música/efectos de festejo durante el FESTEJO.
- INVITACION en silencio *(decisión del diseñador)*.

### Estructura de código y objetivo didáctico

- Hoy toda la lógica está en `lib/utiles/utiles.cpp`. Se separa en módulos chicos y con nombres claros, uno por responsabilidad: tira de LEDs, display, botones, sonido, autos/física y juego/máquina de estados. Nombres y organización exactos a criterio del constructor.
- **Comentarios en español, pensados para el alumno:**
  - Arriba del módulo del juego: qué es una máquina de estados, por qué se usa acá (el programa nunca se queda esperando y siempre puede leer botones, mover autos y sonar a la vez) y el diagrama de estados.
  - Cada estado: qué hace, qué espera y a dónde puede ir.
  - El porqué de cada decisión: punto fijo en vez de `float`, por qué las interrupciones son mínimas, por qué dos timers para el audio, por qué el festejo tiene mínimo de 3 s, por qué el motor sigue al que va adelante, etc.
  - Constantes con nombre y explicadas, sin números mágicos.
- Variables con nombres descriptivos. Hoy hay nombres como `hola`, `jiji`, `noze`, `juan`, que se reemplazan explicando qué son.
- Se mantiene el `main()` propio y el Timer0 de 1 ms del alumno, explicando cómo funcionan.

### Documentación

Actualizar `README.md` (cómo se juega, envs de buzzer, pines) y `CLAUDE.md` (arquitectura nueva).

## Pruebas y ajuste (decidido con el usuario)

- No se va a probar en la placa durante el desarrollo. El usuario carga el firmware desde VS Code en unos días.
- El resultado tiene que quedar **listo para probar**: compila en ambos envs, con valores iniciales razonables elegidos con criterio.
- Todas las constantes ajustables (física, tiempos del semáforo y del festejo, frecuencias y vibración del motor, brillos) quedan agrupadas y documentadas. Para cada una: qué hace, en qué unidad está, valor inicial y cómo se nota el cambio al subirla o bajarla. Por ejemplo: "si la vuelta dura mucho, subí IMPULSO_POR_PULSACION".
- El README incluye una sección corta de "Ajustes" que apunta a esas constantes.

## Lo que queda a criterio del constructor

- Valores exactos de física, frecuencias del motor y duraciones de sonidos (todos son ajustables).
- Diseño concreto de las animaciones de invitación, armado y festejo, y de la secuencia de semáforo dentro de lo descrito.
- Nombres de archivos, funciones y variables.
- Fuente de aleatoriedad para la largada (por ejemplo, un contador libre al momento de armar).
- Si la lógica pura (física, transiciones) se separa del hardware para poder testearla con un env `native`. Es deseable si el toolchain lo permite; si no, se verifica en la placa.
