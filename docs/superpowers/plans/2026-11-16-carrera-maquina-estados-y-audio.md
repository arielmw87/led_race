# Carrera de LEDs: máquina de estados, física y audio. Plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Objetivo:** Reescribir la lógica del juego como una máquina de estados no bloqueante con física de autos, largada F1, 3 vueltas y festejo, más el motor de audio del rfid-reader (buzzer pasivo o activo según el env).

**Arquitectura:** La lógica pura (antirrebote, física de autos, máquina de estados) va en módulos C sin acceso a hardware, testeados en la PC con un env `native` (Unity). Los módulos de hardware (tira, display, timer de 1 ms, sonido) son delgados. `main.c` arma el bucle: lee botones → `juego_paso()` → eventos a sonidos → dibuja efectos → display.

**Tech stack:** PlatformIO, ATmega328P (Nano, `atmelavr`, framework arduino sólo para el toolchain/upload), C99, Unity para los tests nativos (gcc de MinGW ya instalado).

**Spec:** `docs/superpowers/specs/2026-11-16-carrera-maquina-estados-y-audio-design.md`. Leer entero antes de empezar.

## Restricciones globales

- Todo el código nuevo va en **C** (`.c`/`.h`), con guardas `extern "C"` en los headers. `src/main.cpp` pasa a `src/main.c`.
- Comentarios y nombres **en español**, con estilo didáctico. Excepción: el motor de audio portado, que queda como caja negra con los comentarios traducidos y un encabezado explicativo.
- Sin números mágicos: toda constante ajustable va en `lib/ajustes/ajustes.h`, documentada con qué hace, unidad, valor inicial y "si subís/bajás, pasa…".
- Ningún bucle o espera bloqueante en la lógica del juego, los efectos o los sonidos.
- Timer0 = tick de 1 ms (CTC, presc. 64, OCR0A=249), igual que hoy. Timer1 = tono, Timer2 = secuenciador de audio.
- Pines: J1=PB3, J2=PB4, select=PB5 (activos en bajo, pull-up interno; select con pull-up externo 4,7 kΩ por el LED de D13), buzzer=PB2 (OC1B), tira datos=PC4 y clock=PC5, dígitos PC0–PC3, segmentos PB0–PB1 y PD2–PD7 (ánodo común, activos en bajo). NL=240.
- Colores: J1 verde, J2 azul, semáforo rojo.
- RAM: el buffer `led[240]` (960 B) es el único buffer de LEDs. Después de cada tarea de firmware, la RAM total reportada por `pio run` tiene que quedar por debajo del 75 %.
- Envs: `nano_activo` (`-D BUZZER_ACTIVO`, por defecto) y `nano_pasivo` (`-D BUZZER_PASIVO`), ambos `board = nanoatmega328new`; más `native` para tests.

## Foco de revisión

1. **Botón todavía apretado al apagarse el semáforo**: mantener apretado no es una pulsación. El auto arranca recién con la primera pulsación nueva (soltar y volver a apretar). Lo prueba la Tarea 3: `test_largada_boton_mantenido_no_acelera`.
2. **Rebote al mantener apretado en el semáforo**: un rebote de 1 a 19 ms no puede descalificar a nadie. Lo prueba la Tarea 1: `test_rebote_corto_al_soltar_se_ignora`.
3. **Jugadores que siguen apretando al terminar el festejo**: el juego no se vuelve a armar solo. Para que pueda armarse, primero tienen que soltar todos los botones. Lo prueba la Tarea 3: `test_invitacion_exige_soltar_antes_de_armar`.
4. **Empate exacto de posición**: el motor no salta de un auto a otro; se queda con el último que iba adelante. Lo prueba la Tarea 3: `test_lider_empate_mantiene_anterior`.
5. **Desborde de `ahora_ms` (49 días)**: todas las comparaciones de tiempo son `(ahora - inicio) >= duración` sobre `uint32_t`. Lo prueba la Tarea 3: `test_tiempos_sobreviven_desborde`.

---

### Task 1: Env de tests, ajustes y antirrebote

**Archivos:**
- Modificar: `platformio.ini` (reemplazar el env único por `nano_activo`, `nano_pasivo`, `native`; `[platformio] default_envs = nano_activo`)
- Crear: `lib/ajustes/ajustes.h`
- Crear: `lib/antirrebote/antirrebote.h`, `lib/antirrebote/antirrebote.c`
- Test: `test/test_antirrebote/test_antirrebote.c`

**Interfaces:**
- Produce `ajustes.h` con estos valores iniciales (unidades entre paréntesis):
  - `ANTIRREBOTE_MS 20`
  - `ARMADO_MS 2000`
  - `SEMAFORO_LUCES 5`, `SEMAFORO_PASO_MS 1000`, `SEMAFORO_ESPERA_MIN_MS 200`, `SEMAFORO_ESPERA_MAX_MS 3000`
  - `GRACIA_SOLTAR_MS 200`
  - `VUELTAS_PARA_GANAR 3`
  - `FESTEJO_MIN_MS 3000`, `FESTEJO_MAX_MS 20000`
  - Física (posición en 1/65536 de LED, velocidad en 1/65536 de LED por ms): `IMPULSO_POR_PULSACION 437`, `ROZAMIENTO_DIVISOR 1024` (cada ms `vel -= vel / ROZAMIENTO_DIVISOR`, constante de tiempo ≈ 1 s), `VELOCIDAD_MAXIMA 3932` (≈ 60 LED/s). En el comentario va la cuenta: a 6 pulsaciones/s da ≈ 40 LED/s, o sea una vuelta de 6 s.
  - Motor: `MOTOR_HZ_MIN 300`, `MOTOR_HZ_MAX 1500`, `MOTOR_VIBRATO_HZ 40` (variación ± de la frecuencia), `MOTOR_VOLUMEN 200`
  - `BRILLO_TIRA 31`
  - `NL 240` (sale de `utiles.h`)
- Produce `antirrebote.h`:
  ```c
  typedef struct { uint8_t estable; uint8_t cuenta; } antirrebote_t;
  void antirrebote_iniciar(antirrebote_t *a);            /* estable = 0 (suelto) */
  /* Llamar cada 1 ms. crudo: 1 = apretado. Devuelve el estado estable. */
  uint8_t antirrebote_paso(antirrebote_t *a, uint8_t crudo);
  ```
  El estado estable cambia sólo después de `ANTIRREBOTE_MS` lecturas seguidas en el nivel nuevo. Vale para los dos flancos, a diferencia del código actual.

- [ ] **Paso 1: Configurar `platformio.ini`.** El env `native` lleva `platform = native`, `test_framework = unity`, `test_build_src = no` y `lib_ignore = tira, display, temporizador, sonido, efectos`. Los dos envs nano llevan `build_flags` con la definición del buzzer y `test_ignore = *`.
- [ ] **Paso 2: Escribir los tests que fallan** en `test_antirrebote.c`:
  - `test_arranca_suelto`: después de iniciar, `paso(…,0)` devuelve 0.
  - `test_pulsacion_estable_tras_20ms`: 19 pasos con 1 → 0; el paso 20 → 1.
  - `test_rebote_corto_al_apretar_se_ignora`: 10×1, 1×0, 15×1 → siempre 0.
  - `test_rebote_corto_al_soltar_se_ignora`: estable en 1; 1..19×0 y después 1 → sigue en 1.
  - `test_soltada_estable_tras_20ms`: estable en 1; 20×0 → 0.
- [ ] **Paso 3:** `pio test -e native -f test_antirrebote`. Esperado: FALLA (no compila, falta `antirrebote.c`).
- [ ] **Paso 4: Implementar** `antirrebote.c` y `ajustes.h` con los comentarios didácticos (qué es el rebote y por qué se filtra en los dos flancos).
- [ ] **Paso 5:** `pio test -e native -f test_antirrebote`. Esperado: 5 tests PASAN.
- [ ] **Paso 6: Commit** `feat: env de tests nativos, ajustes y antirrebote de dos flancos`

### Task 2: Física de los autos

**Archivos:**
- Crear: `lib/autos/autos.h`, `lib/autos/autos.c`
- Test: `test/test_autos/test_autos.c`

**Interfaces:**
- Consume `ajustes.h` (física, `NL`).
- Produce:
  ```c
  typedef struct {
      uint32_t posicion;    /* 1/65536 de LED, 0 .. NL*65536-1 */
      uint16_t velocidad;   /* 1/65536 de LED por ms */
      uint8_t  vueltas;
      uint8_t  boton_antes; /* para detectar el flanco */
  } auto_t;
  void     auto_reiniciar(auto_t *a, uint8_t boton_ahora); /* pos=vel=vueltas=0, boton_antes=boton_ahora */
  /* Avanza 1 ms. boton: estado estable. Devuelve 1 si completó una vuelta en este ms. */
  uint8_t  auto_paso_1ms(auto_t *a, uint8_t boton);
  uint16_t auto_led(const auto_t *a);                       /* posicion >> 16 */
  ```
  Orden dentro de `auto_paso_1ms`: impulso si hay flanco 0→1 → recorte a `VELOCIDAD_MAXIMA` → rozamiento → `posicion += velocidad` → si pasa de `NL*65536`, se resta y `vueltas++`.

- [ ] **Paso 1: Escribir los tests que fallan:**
  - `test_flanco_suma_impulso`: con `reiniciar(a,0)`, `paso(a,1)` deja `velocidad` entre `IMPULSO_POR_PULSACION - 1` y `IMPULSO_POR_PULSACION`.
  - `test_mantener_no_acelera`: después de 100 pasos con 1, la velocidad es menor que la que quedó tras el primer paso.
  - `test_reiniciar_con_boton_apretado_no_es_flanco`: `reiniciar(a,1)` seguido de `paso(a,1)` deja `velocidad == 0`.
  - `test_rozamiento_frena`: tras un impulso y 5000 pasos con 0, la velocidad es menor que 10.
  - `test_velocidad_tope`: 200 flancos alternando 1 y 0 dejan `velocidad <= VELOCIDAD_MAXIMA`.
  - `test_avance_continuo`: a velocidad máxima, ningún paso cambia `auto_led` en más de 1.
  - `test_vuelta_se_cuenta_y_reinicia`: con `posicion = (NL*65536) - 10` y `velocidad = 100`, el paso devuelve 1, `vueltas == 1` y `auto_led == 0`.
  - `test_ritmo_6hz_vuelta_entre_4_y_9_s`: simular 6 pulsaciones/s (83 ms de período, 40 apretado) hasta la primera vuelta. El tiempo tiene que quedar entre 4000 y 9000 ms. Sirve de prueba de cordura de los valores iniciales.
- [ ] **Paso 2:** `pio test -e native -f test_autos`. Esperado: FALLA (no compila).
- [ ] **Paso 3: Implementar** `autos.c`. El comentario explica el punto fijo (por qué no se usa `float` en un AVR) y la física impulso/rozamiento.
- [ ] **Paso 4:** `pio test -e native -f test_autos`. Esperado: 8 PASAN.
- [ ] **Paso 5: Commit** `feat: física de autos con impulso por pulsación y rozamiento`

### Task 3: Máquina de estados del juego

**Archivos:**
- Crear: `lib/juego/juego.h`, `lib/juego/juego.c`
- Test: `test/test_juego/test_juego.c`

**Interfaces:**
- Consume `auto_t` y sus funciones (Tarea 2), y `ajustes.h`.
- Produce:
  ```c
  typedef enum { EST_INVITACION, EST_ARMADO, EST_SEMAFORO, EST_CARRERA, EST_FESTEJO } estado_t;
  enum { BOTON_J1 = 1, BOTON_J2 = 2, BOTON_SELECT = 4 };       /* máscara de entradas */
  enum {                                                       /* máscara de eventos */
      EV_ARMANDO = 1, EV_LUZ_SEMAFORO = 2, EV_LARGADA = 4, EV_VUELTA_J1 = 8,
      EV_VUELTA_J2 = 16, EV_DESCALIFICADO = 32, EV_GANADOR = 64, EV_A_INVITACION = 128
  };
  typedef struct {
      estado_t estado;
      uint32_t inicio_estado;     /* ms en que se entró al estado actual */
      uint8_t  luces;             /* luces de semáforo prendidas, 0..SEMAFORO_LUCES */
      uint16_t espera_apagado;    /* ms al azar con todas prendidas */
      uint32_t primera_soltada;   /* ms; para la ventana de gracia */
      uint8_t  quien_solto;       /* 0, 1 o 2 */
      uint8_t  esperar_soltar;    /* 1 = INVITACION no arma hasta que todo esté suelto */
      uint8_t  ganador;           /* 0 = nadie, 1 o 2 */
      uint8_t  lider;             /* 1 o 2 */
      auto_t   autos[2];
      uint8_t  entradas_antes;
      uint32_t ultimo_paso;       /* ms del último paso de física */
  } juego_t;
  void    juego_iniciar(juego_t *j, uint32_t ahora_ms);
  /* Llamar en cada vuelta del loop. azar: cualquier número que varíe (TCNT1 o similar),
     se usa sólo al entrar al semáforo. Devuelve la máscara de eventos ocurridos. */
  uint8_t juego_paso(juego_t *j, uint8_t entradas, uint32_t ahora_ms, uint16_t azar);
  const auto_t *juego_auto_lider(const juego_t *j);
  ```
- Reglas (las del spec, sin cambios):
  - INVITACION→ARMADO con J1 y J2 apretados y `esperar_soltar == 0` (se pone en 0 cuando las entradas valen 0). Emite `EV_ARMANDO`.
  - ARMADO: si alguno suelta → INVITACION, sin `EV_A_INVITACION`. Si pasan `ARMADO_MS` → SEMAFORO con `luces = 0`.
  - SEMAFORO: se prende una luz cada `SEMAFORO_PASO_MS` (la primera al entrar) con `EV_LUZ_SEMAFORO`. Con las 5 prendidas, `espera_apagado = SEMAFORO_ESPERA_MIN_MS + azar % (MAX - MIN + 1)`; vencida la espera → CARRERA, `EV_LARGADA`, `auto_reiniciar(&autos[i], boton_i_actual)`.
  - Soltada en SEMAFORO: se anota `quien_solto` y `primera_soltada`. Si el otro suelta dentro de `GRACIA_SOLTAR_MS` → INVITACION con `EV_A_INVITACION`. Si vence la gracia → FESTEJO, ganador = el otro, `EV_DESCALIFICADO | EV_GANADOR`.
  - CARRERA: la física avanza en pasos de 1 ms desde `ultimo_paso` hasta `ahora_ms`. Cada vuelta emite `EV_VUELTA_Jn`. Con `VUELTAS_PARA_GANAR` → FESTEJO con `EV_GANADOR` (si llegan los dos en el mismo ms, gana J1). Líder = más vueltas, o más posición; en empate exacto queda el anterior.
  - Select en ARMADO, SEMAFORO o CARRERA → INVITACION con `EV_A_INVITACION` y `esperar_soltar = 1`.
  - FESTEJO: antes de `FESTEJO_MIN_MS` ignora todo. Después, un flanco 0→1 de cualquier botón o `FESTEJO_MAX_MS` → INVITACION con `EV_A_INVITACION` y `esperar_soltar = 1`.

- [ ] **Paso 1: Escribir los tests que fallan** (un helper `avanzar(j, entradas, desde, hasta, paso)` que llama `juego_paso` y acumula eventos):
  - `test_inicia_en_invitacion`
  - `test_ambos_apretados_arma`: estado ARMADO y `EV_ARMANDO`.
  - `test_uno_solo_no_arma`
  - `test_soltar_en_armado_vuelve_sin_penalidad`
  - `test_armado_2s_pasa_a_semaforo_con_primera_luz`: en t+2000, SEMAFORO, `luces == 1`, `EV_LUZ_SEMAFORO`.
  - `test_semaforo_prende_5_luces_a_1s`: `luces == 5` a los 4000 ms de entrar al semáforo.
  - `test_largada_tras_espera_aleatoria`: con `azar = 0`, CARRERA y `EV_LARGADA` en `4000 + 200` ms (no antes); con `azar = 2800`, en `4000 + 3000`.
  - `test_largada_boton_mantenido_no_acelera`: con los botones apretados durante toda la largada y 500 ms más, la velocidad de los dos autos es 0.
  - `test_uno_suelta_descalificado_gana_otro`: J1 suelta en el semáforo y J2 no → a los +200 ms, FESTEJO, `ganador == 2`, eventos `EV_DESCALIFICADO | EV_GANADOR`.
  - `test_ambos_sueltan_en_gracia_vuelve_a_invitacion`: J1 suelta y J2 suelta 150 ms después → INVITACION, `EV_A_INVITACION`, `ganador == 0`.
  - `test_segunda_soltada_fuera_de_gracia_descalifica_al_primero`: J2 suelta 250 ms después de J1 → `ganador == 2`.
  - `test_vuelta_emite_evento`: con `autos[0].posicion` justo antes del final y velocidad > 0, aparece `EV_VUELTA_J1`.
  - `test_tres_vueltas_gana`: con `autos[1].vueltas = 2` cerca del final → FESTEJO, `ganador == 2`, `EV_GANADOR`.
  - `test_select_aborta_carrera`
  - `test_festejo_ignora_botones_antes_de_3s`
  - `test_festejo_boton_despues_de_3s_vuelve`
  - `test_festejo_20s_vuelve_solo`
  - `test_invitacion_exige_soltar_antes_de_armar`: termina el festejo con los dos botones apretados → sigue en INVITACION mientras sigan apretados; los suelta, los aprieta → ARMADO.
  - `test_lider_por_vueltas_y_posicion`
  - `test_lider_empate_mantiene_anterior`: J2 adelante, después empate exacto → `lider == 2`.
  - `test_tiempos_sobreviven_desborde`: `juego_iniciar` en `0xFFFFF000` y armado completo cruzando el desborde → SEMAFORO a los 2000 ms.
- [ ] **Paso 2:** `pio test -e native -f test_juego`. Esperado: FALLA (no compila).
- [ ] **Paso 3: Implementar** `juego.c`. Va un `switch` por estado y una función por estado (`estado_invitacion()`…), cada una de menos de 50 líneas. Comentario de cabecera con: qué es una máquina de estados, por qué se usa (nada bloquea), el diagrama ASCII del spec y por qué cada regla (gracia de 200 ms, mínimo de 3 s, `esperar_soltar`, líder con histéresis).
- [ ] **Paso 4:** `pio test -e native`. Esperado: pasan todos los tests (antirrebote, autos, juego).
- [ ] **Paso 5: Commit** `feat: máquina de estados del juego (invitación, armado, semáforo, carrera, festejo)`

### Task 4: Motor de audio (pasivo y activo)

**Archivos:**
- Crear: `lib/sonido/sonido.h`, `lib/sonido/sonido.c` (portado de `C:\Proyectos\rfid-reader\lib\buzzer\buzzer.c` y `.h`)

**Interfaces:**
- Consume `ajustes.h` (motor).
- Produce:
  ```c
  enum { SND_NINGUNO, SND_LUZ, SND_LARGADA, SND_VUELTA, SND_DESCALIFICADO,
         SND_FESTEJO, SND_ARMANDO, SND_CANTIDAD };
  void sonido_iniciar(void);
  void sonido_tocar(uint8_t id);       /* no bloquea; corta el sonido anterior */
  void sonido_motor(uint16_t hz);      /* 0 = motor apagado; ignorado en BUZZER_ACTIVO */
  uint8_t sonido_sonando(void);        /* 1 mientras suena un sonido del catálogo */
  ```
- Se porta tal cual el mecanismo (`note_t`, macros `NOTE`/`GLIDE`/`FADE`/`REST`/`NOISE`/`END`, secuenciador en Timer2, `set_tone`, LFSR) con los comentarios traducidos. Se toma la rama 328P del tono (fast PWM modo 14, TOP=ICR1) pero sobre **OC1B/PB2**: `COM1B1` y `OCR1B` en lugar de `COM1A1`/`OCR1A`. Sin ISR de Timer1. Se elimina `buzzer_beep` y el volumen maestro; no hay `config.h` (las macros de pin van en `sonido.c`). El inicializador designado `[SND_X] =` se mantiene, porque es C99.
- Catálogo nuevo:
  - `SND_LUZ`: `NOTE(880,120)`.
  - `SND_LARGADA`: `NOTE(1760,400)`.
  - `SND_VUELTA`: `NOTE(1568,60), REST(30), NOTE(2093,90)`.
  - `SND_DESCALIFICADO`: el `SND_SAD_P` original.
  - `SND_FESTEJO`: el `SND_LEVELWIN_P` original seguido del `SND_POWERUP_P`.
  - `SND_ARMANDO`: `FADE(1047,60,180,0)`.
- Motor (sólo `BUZZER_PASIVO`): con `motor_hz != 0` y sin sonido de catálogo, el secuenciador Timer2 sigue corriendo y cada 10 ms pone `set_tone(motor_hz ± MOTOR_VIBRATO_HZ alternado, MOTOR_VOLUMEN)`. Al terminar un sonido de catálogo, si el motor está activo, se sigue con el motor en vez de `buzz_stop()`.
- `BUZZER_ACTIVO`: `tone_init` no toca el Timer1. `tone_on` pone PB2 en alto y `tone_off` en bajo. `sonido_motor` no hace nada.
- Encabezado: qué hace el módulo, cómo se usa, que se portó del rfid-reader y se deja como caja negra, cómo agregar un sonido.

- [ ] **Paso 1: Portar** `sonido.c` y `sonido.h` según lo anterior.
- [ ] **Paso 2:** `pio run -e nano_pasivo` y `pio run -e nano_activo` con un `main.c` temporal que sólo llame `sonido_iniciar(); sonido_tocar(SND_FESTEJO); sei(); for(;;);`. Antes, sacar `src/main.cpp` del build moviéndolo a `src/main.cpp.viejo` (en la Tarea 5 se borra). Esperado: los dos envs dan `SUCCESS`.
- [ ] **Paso 3: Commit** `feat: motor de audio portado del rfid-reader con envs de buzzer pasivo y activo`

### Task 5: Hardware, efectos e integración

**Archivos:**
- Crear: `lib/tira/tira.h`, `lib/tira/tira.c`. Se toman `struct LED`, `led[NL]`, `mandar_byte/start/stop`, `refresh_leds` y `off_leds` de `utiles.cpp`, renombrados `tira_refrescar()`, `tira_apagar()` y `tira_pintar(uint16_t pos, uint8_t r, uint8_t g, uint8_t b, uint8_t brillo)` con la guarda `pos >= NL`.
- Crear: `lib/display/display.h`, `lib/display/display.c`: `display_vueltas(uint8_t j1, uint8_t j2)` y `display_multiplexar()` (se llama desde el ISR). Los dígitos del medio van apagados con un código "en blanco" en la tabla.
- Crear: `lib/temporizador/temporizador.h`, `lib/temporizador/temporizador.c`: configura Timer0 y define su ISR (multiplexa, cuenta ms, `antirrebote_paso` ×3). Expone `uint32_t temporizador_ms(void)` (lectura atómica) y `uint8_t temporizador_botones(void)` (máscara `BOTON_*`).
- Crear: `lib/efectos/efectos.h`, `lib/efectos/efectos.c`: `void efectos_dibujar(const juego_t *j, uint32_t ahora_ms)`. Pinta según el estado y llama `tira_refrescar()`:
  - INVITACION: arcoíris que corre por toda la tira.
  - ARMADO: la tira se llena de rojo desde las dos puntas hacia el centro, en proporción a los 2 s.
  - SEMAFORO: 5 bloques rojos de NL/5, prendidos según `luces`.
  - CARRERA: cada auto con su color y una estela de 5 LEDs que se desvanece.
  - FESTEJO: destellos al azar del color del ganador más un pulso de brillo.
- Crear: `src/main.c`. Init de pines (como el `main.cpp` actual, buzzer PB2 en salida, botones PB3–PB5 entrada con pull-up), `temporizador_iniciar()`, `sonido_iniciar()`, `sei()`. En el loop:
  - `juego_paso` → eventos → `sonido_tocar`. Prioridad: GANADOR > DESCALIFICADO > LARGADA > LUZ > VUELTA > ARMANDO.
  - Motor: en CARRERA, `sonido_motor(MOTOR_HZ_MIN + (velocidad del líder × (MAX-MIN)) / VELOCIDAD_MAXIMA)`; fuera de carrera, `sonido_motor(0)`.
  - `efectos_dibujar`, y después `display_vueltas` con las vueltas de los autos.
- Borrar: `lib/utiles/`, `src/main.cpp.viejo`.

- [ ] **Paso 1: Crear** `tira`, `display`, `temporizador` y `efectos` con los comentarios didácticos (protocolo APA102, multiplexado, por qué el ISR hace poco, por qué los efectos se calculan a partir del tiempo y no con bucles).
- [ ] **Paso 2: Escribir** `src/main.c` y borrar `lib/utiles/` y `src/main.cpp.viejo`.
- [ ] **Paso 3:** `pio run -e nano_activo` y `pio run -e nano_pasivo`. Esperado: `SUCCESS` en los dos, RAM < 75 %.
- [ ] **Paso 4:** `pio test -e native`. Esperado: todo PASA.
- [ ] **Paso 5: Commit** `feat: integración de la máquina de estados con tira, display, efectos y sonido`

### Task 6: Documentación

**Archivos:**
- Modificar: `README.md`. Actualizar "Cómo se juega" con el flujo nuevo. Sumar una sección "Buzzer activo o pasivo" (envs y cómo elegir en VS Code), una sección "Ajustes" (apunta a `lib/ajustes/ajustes.h` con una tabla síntoma → constante → hacia dónde moverla) y una sección "Tests" (`pio test -e native`). Actualizar la estructura del código.
- Modificar: `CLAUDE.md`. Comandos (envs, tests), arquitectura nueva (lógica pura vs hardware, flujo del loop, timers), y gotchas al día.

- [ ] **Paso 1: Escribir** los dos documentos.
- [ ] **Paso 2: Verificar** que cada constante nombrada en la sección "Ajustes" exista en `ajustes.h` (`grep`).
- [ ] **Paso 3: Commit** `docs: README y CLAUDE.md para la nueva arquitectura`
