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
| D13 (PB5) | Botón select (a GND) **+ pull-up externo de ~470 Ω a 5 V** |
| A4 (PC4) | Tira LED — datos |
| A5 (PC5) | Tira LED — clock |
| A0–A3 (PC0–PC3) | Habilitación de los 4 dígitos del display |
| D8–D9 (PB0–PB1), D2–D7 (PD2–PD7) | Segmentos del display (activos en bajo) |

- La tira es APA102 / SK9822 (datos + clock, brillo de 5 bits por LED), alimentada con una fuente de 40 A.
- El display es de 4 dígitos de ánodo común.
- El D13 tiene el LED de la placa conectado (con 1 kΩ a GND). Por eso el select necesita un pull-up externo **fuerte**: con el interno, o incluso con 4,7 kΩ, el pin queda por debajo de 3 V y se lee como apretado, y el juego aborta todas las largadas. Con 470 Ω queda en ~4 V. Conviene medir el pin con el tester (suelto tiene que dar más de 3,5 V).

### Alimentación de la tira

- **Masa común:** el GND de la fuente de 40 A tiene que estar unido al GND del Nano. Sin eso, las señales de datos y clock no tienen referencia y la tira muestra colores al azar o no responde.
- **Inyección de 5 V:** en una tira de 240 LEDs, la tensión cae a lo largo de los cables finos de la tira y los últimos LEDs se ven más tenues o amarillentos con colores claros. Conviene llevar 5 V y GND desde la fuente a **los dos extremos** de la tira (y, si se nota caída, también al medio), con cable grueso.
- **El Nano no alimenta la tira:** el USB o el pin 5V del Nano dan unos cientos de mA; la tira a pleno puede pedir más de 10 A. Durante las pruebas por USB, la tira tiene que estar conectada a su fuente igual.
- **Orden de conexión recomendado:** primero masa, después alimentación, al final datos y clock. Si la tira queda alimentada y el Nano no, los pines de datos pueden alimentar la tira "por atrás" y dañarse.

## Buzzer activo o pasivo

El buzzer va siempre en D10 (PB2). Lo que cambia es con qué env se compila:

| Env | Buzzer | Qué suena |
|-----|--------|-----------|
| `nano_activo` (por defecto) | activo (trae su oscilador) | el ritmo de cada sonido; sin motor |
| `nano_pasivo` | pasivo (con transistor) | notas completas y sonido de motor que sube con la velocidad |

Con el buzzer pasivo, el tono sale por PWM del Timer1 (OC1B), así que suena limpio sin cargar al micro.

### Circuito del buzzer

Un pin del Nano entrega como máximo ~20 mA. Para no forzarlo, el buzzer se maneja con un transistor:

```
                +5V
                 │
            ┌────┴────┐
            │         │
         Buzzer     Diodo 1N4148
            │       (cátodo a +5V)
            │         │
            └────┬────┘
                 │
                 C
 D10 ──[ 1 kΩ ]──B   NPN (BC547, 2N2222 o similar)
                 E
                 │
                GND
```

- **Pasivo:** el circuito de arriba es obligatorio. El diodo protege al transistor de los picos de tensión que genera la bobina del buzzer al cortar la corriente.
- **Activo:** se puede usar el mismo circuito (recomendado). Si el buzzer consume menos de 20 mA (ver hoja de datos), puede ir directo entre D10 y GND, respetando la polaridad (+ a D10).
- Si un buzzer pasivo suena muy bajo, se puede bajar la resistencia de base a 470 Ω.

En VS Code se elige el env desde la barra de PlatformIO (abajo), o por línea de comandos:

```sh
pio run -e nano_activo -t upload
pio run -e nano_pasivo -t upload
```

Si hay más de un puerto serie conectado, agregar `--upload-port COMx`.

## Primera prueba en la placa

Antes de cargar: compilar el env que corresponde al buzzer montado y verificar que diga `SUCCESS`.

Probar en este orden; cada paso depende de que el anterior funcione:

1. **Antes de enchufar:** con el tester, medir que no haya corto entre 5 V y GND de la fuente, y que la masa de la fuente esté unida a la del Nano.
2. **Select en reposo:** con todo alimentado y el botón select suelto, medir D13: tiene que dar más de 3,5 V. Si da menos, revisar el pull-up de 470 Ω.
3. **Al encender:** suena nada (la invitación es en silencio), el display muestra **0** a la izquierda y **0** a la derecha, y la tira muestra un **arcoíris** que se mueve.
4. **Botones uno por uno:** apretar sólo J1, sólo J2 y sólo select: no tiene que pasar nada (para armar hacen falta los dos).
5. **Armado:** mantener J1 y J2 juntos: suena un tic y la tira se llena de rojo desde las puntas. Soltar uno antes de que se junten: vuelve el arcoíris.
6. **Semáforo:** volver a armar y aguantar 2 s: se prenden 5 bloques rojos, uno por segundo, cada uno con un bip, y después se apagan juntos con un bip agudo.
7. **Carrera:** pulsar J1 y J2: los autos (verde y azul) avanzan con estela. Cada vuelta suena un "pip-pip" y suma en el display. Con buzzer pasivo, se escucha un motor que sube de tono al acelerar.
8. **Festejo:** al completar 3 vueltas, destellos del color del ganador y música. Antes de 3 s los botones no hacen nada; después, cualquiera vuelve a la invitación.
9. **Descalificación:** armar y soltar un solo botón durante el semáforo: suena el trombón triste y festeja el otro.
10. **Select:** en medio de una carrera, apretar select: vuelve a la invitación.

### Si algo falla

| Síntoma | Qué revisar primero |
|---------|---------------------|
| La tira no prende o muestra colores al azar | masa común entre fuente y Nano; datos en A4 y clock en A5 (no invertidos); orden de la tira (la flecha de datos apunta hacia afuera del Nano) |
| Los últimos LEDs se ven tenues o amarillos | inyectar 5 V también en el otro extremo de la tira |
| Armado o semáforo se cortan solos y vuelve el arcoíris | el select se lee como apretado: medir D13 (paso 2) |
| Un botón no responde | cableado a GND; que sea el pin correcto (D11, D12, D13) |
| Se descalifica a alguien sin que suelte | botón con mal contacto; probar subir `ANTIRREBOTE_MS` |
| El display muestra segmentos cruzados o números raros | orden de los cables de segmentos (D8, D9, D2–D7 = segmentos a–g y punto) |
| No suena nada | env correcto para el buzzer montado; transistor y diodo; polaridad del buzzer activo |
| Suena "raro" o muy bajo con buzzer activo | se compiló `nano_pasivo` por error: cargar `nano_activo` |
| Se resetea al prender la tira entera | la tira está tomando corriente del Nano: alimentarla desde la fuente |

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
