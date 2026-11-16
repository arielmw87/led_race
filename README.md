# Carrera de LEDs

Juego de carreras para dos jugadores sobre una tira de 240 LEDs, hecho con un Arduino Nano (ATmega328P) programado a nivel de registros.

Cada jugador pulsa su botón para acelerar su auto (verde = jugador 1, azul = jugador 2). Cuanto más rápido pulsás y soltás, más rápido va. Si dejás de pulsar, el auto frena de a poco. Gana el primero en completar 3 vueltas.

## Cómo se juega

1. **Invitación:** la tira muestra un arcoíris esperando jugadores.
2. **Armado:** los dos jugadores mantienen apretado su botón. La tira se llena de rojo desde las puntas; si aguantan 2 segundos, arranca la largada. Si alguno suelta antes, se vuelve a la invitación sin penalidad.
3. **Semáforo (estilo F1):** se prenden 5 luces rojas, una por segundo, y después se apagan todas juntas en un momento al azar. Hay que seguir apretando hasta que se apaguen:
   - si **uno** suelta antes, queda descalificado y gana el otro;
   - si **los dos** sueltan antes (con menos de 0,2 s de diferencia), no gana nadie y se vuelve a la invitación.
4. **Carrera:** al apagarse las luces, a pulsar. Mantener apretado no sirve: cuenta cada pulsación. Cada vuelta suena un "pip" y el display muestra las vueltas (izquierda = jugador 1, derecha = jugador 2).
5. **Festejo:** el que completa 3 vueltas gana: luces de su color y música. Dura entre 3 y 20 segundos; después de los 3 primeros, cualquier botón vuelve a la invitación.

El botón **select** aborta el armado, el semáforo o la carrera y vuelve a la invitación.

Para probar con un solo auto, hay que apretar los dos botones y hacer la largada completa.

## Hardware

| Pin del Nano | Conexión |
|--------------|----------|
| D10 (PB2) | Buzzer (activo o pasivo, ver abajo) |
| D11 (PB3) | Botón jugador 1 (a GND, pull-up interno) |
| D12 (PB4) | Botón jugador 2 (a GND, pull-up interno) |
| D13 (PB5) | Botón select (a GND) **+ pull-up externo de ~4,7 kΩ a 5 V** |
| A4 (PC4) | Tira LED — datos |
| A5 (PC5) | Tira LED — clock |
| A0–A3 (PC0–PC3) | Habilitación de los 4 dígitos del display |
| D8–D9 (PB0–PB1), D2–D7 (PD2–PD7) | Segmentos del display (activos en bajo) |

- La tira es APA102 / SK9822 (datos + clock, brillo de 5 bits por LED), alimentada con una fuente de 40 A.
- El display es de 4 dígitos de ánodo común.
- El D13 tiene el LED de la placa conectado. Por eso el select necesita el pull-up externo: con sólo el interno, el pin puede leerse como apretado sin que nadie lo toque.

## Buzzer activo o pasivo

El buzzer va siempre en D10 (PB2). Lo que cambia es con qué env se compila:

| Env | Buzzer | Qué suena |
|-----|--------|-----------|
| `nano_activo` (por defecto) | activo (trae su oscilador) | el ritmo de cada sonido; sin motor |
| `nano_pasivo` | pasivo (con transistor) | notas completas y sonido de motor que sube con la velocidad |

Con el buzzer pasivo, el tono sale por PWM del Timer1 (OC1B), así que suena limpio sin cargar al micro.

En VS Code se elige el env desde la barra de PlatformIO (abajo), o por línea de comandos:

```sh
pio run -e nano_activo -t upload
pio run -e nano_pasivo -t upload
```

Si hay más de un puerto serie conectado, agregar `--upload-port COMx`.

## Ajustes

Todas las constantes para ajustar el juego están en `lib/ajustes/ajustes.h`. Cada una explica qué hace, en qué unidad está y qué pasa al subirla o bajarla. Los valores iniciales salen de cuentas, no de pruebas en la placa.

| Si notás que… | Tocá | Hacia |
|---------------|------|-------|
| las vueltas duran mucho | `IMPULSO_POR_PULSACION` | subir |
| las vueltas duran poco | `IMPULSO_POR_PULSACION` | bajar |
| el auto frena muy rápido al dejar de pulsar | `ROZAMIENTO_DIVISOR` | subir |
| el auto sigue andando mucho sin pulsar | `ROZAMIENTO_DIVISOR` | bajar |
| los que pulsan muy rápido no se despegan | `VELOCIDAD_MAXIMA` | subir |
| la carrera es muy corta o muy larga | `VUELTAS_PARA_GANAR` | subir/bajar |
| un rebote de botón cuenta como pulsación o soltada | `ANTIRREBOTE_MS` | subir |
| se descalifica a alguien sin que haya soltado | `ANTIRREBOTE_MS` | subir |
| la largada es muy predecible | `SEMAFORO_ESPERA_MIN_MS` / `SEMAFORO_ESPERA_MAX_MS` | ampliar el rango |
| el motor suena muy grave o muy agudo | `MOTOR_HZ_MIN` / `MOTOR_HZ_MAX` | subir/bajar |
| el motor suena a pitido limpio | `MOTOR_VIBRATO_HZ` | subir |
| el motor tapa a los demás sonidos | `MOTOR_VOLUMEN` | bajar |
| la tira encandila | `BRILLO_TIRA` | bajar |
| el festejo es muy corto o muy largo | `FESTEJO_MIN_MS` / `FESTEJO_MAX_MS` | subir/bajar |

## Tests

La lógica del juego (antirrebote, física de los autos y máquina de estados) no toca el hardware, así que se prueba en la PC:

```sh
pio test -e native
```

Hace falta un `gcc` instalado en la PC.

## Estructura del código

- `src/main.c`: inicializa todo y corre el loop principal (botones → juego → sonidos → luces → display).
- `lib/juego/`: **máquina de estados** del juego. Empezar a leer por acá: el comentario del principio explica qué es una máquina de estados y por qué se usa.
- `lib/autos/`: física de cada auto (impulso por pulsación, rozamiento, vueltas) en punto fijo.
- `lib/antirrebote/`: filtro de rebote de los botones.
- `lib/ajustes/`: todas las constantes ajustables.
- `lib/temporizador/`: Timer0 con interrupción de 1 ms: reloj, lectura de botones y multiplexado del display.
- `lib/tira/`: protocolo de la tira APA102.
- `lib/display/`: display de 7 segmentos multiplexado.
- `lib/efectos/`: animaciones de la tira para cada estado.
- `lib/sonido/`: motor de sonido (portado del proyecto rfid-reader, se usa como caja negra).

Como el Timer0 está reconfigurado, `millis()` y `delay()` de Arduino **no funcionan** en este proyecto. El tiempo se lee con `temporizador_ms()`.
