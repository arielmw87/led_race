# Carrera de LEDs

Juego de carreras para dos jugadores sobre una tira de 240 LEDs, hecho con un Arduino Nano (ATmega328P) programado a nivel de registros.

Cada jugador aprieta su botón para hacer avanzar su luz por la tira (verde = jugador 1, azul = jugador 2). El primero en llegar al final suma un punto, y el que llega a 3 puntos gana la partida. El puntaje se muestra en un display de 7 segmentos de 4 dígitos.

## Cómo se juega

1. Al encender suena el buzzer y la tira arranca apagada.
2. Cada pulsación de tu botón avanza tu LED 10 posiciones.
3. Al llegar al final de la tira:
   - Se suma un punto y se muestra una animación con LEDs aleatorios.
   - El display muestra el puntaje: el dígito de la izquierda es el jugador 1 y el de la derecha el jugador 2.
   - Las posiciones vuelven a cero y arranca una nueva ronda.
4. Con 3 puntos gana el jugador: su luz recorre toda la tira y se reinicia la partida.
5. El botón de **select**:
   - En medio de una ronda, la reinicia (posiciones a cero, se mantiene el puntaje).
   - Con ambos jugadores en la posición inicial, reinicia también el puntaje.

## Hardware

| Pin del Nano | Conexión |
|--------------|----------|
| D10 (PB2) | Botón jugador 1 (a GND, pull-up interno) |
| D11 (PB3) | Botón jugador 2 (a GND, pull-up interno) |
| D12 (PB4) | Botón select / reinicio (a GND, pull-up interno) |
| D13 (PB5) | Buzzer |
| A4 (PC4) | Tira LED — datos |
| A5 (PC5) | Tira LED — clock |
| A0–A3 (PC0–PC3) | Habilitación de los 4 dígitos del display |
| D8–D9 (PB0–PB1), D2–D7 (PD2–PD7) | Segmentos del display (activos en bajo) |

La tira es de tipo APA102 / SK9822 (protocolo de datos + clock, brillo de 5 bits por LED). El display es de 4 dígitos de ánodo común.

## Compilar y cargar

Requiere [PlatformIO](https://platformio.org/) (CLI o la extensión de VS Code).

```sh
pio run               # compilar
pio run -t upload     # compilar y cargar al Nano
```

Si hay más de un puerto serie conectado, agregar `--upload-port COMx`.

## Estructura del código

- `src/main.cpp` — configuración de puertos y del Timer 0, y bucle principal. Usa su propio `main()` en lugar de `setup()`/`loop()`.
- `lib/utiles/` — toda la lógica del juego: manejo de la tira de LEDs, movimiento de los jugadores, puntajes y animaciones, buzzer, display y la interrupción del timer.

El Timer 0 está reconfigurado para generar una interrupción cada 1 ms, que se encarga de:

- multiplexar el display,
- llevar la cuenta de tiempo (`hola`),
- filtrar el rebote de los botones (20 ms).

Por eso `millis()` y `delay()` de Arduino **no funcionan** en este proyecto.
