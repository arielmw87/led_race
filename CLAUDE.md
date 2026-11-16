# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Bare-metal AVR firmware (PlatformIO, Arduino Nano / ATmega328P @ 16 MHz) for a two-player LED-strip race game: players mash a button to accelerate their car along a 240-LED strip; F1-style start, 3 laps to win. Originally written by a student. **Code, comments and docs are in Spanish and written to be didactic**: keep that style (explain the *why*, no magic numbers) when changing anything except `lib/sonido/`, which is a ported black box.

## Commands

```sh
pio run -e nano_activo            # build for active buzzer (default env)
pio run -e nano_pasivo            # build for passive buzzer (tones + engine sound)
pio run -e nano_activo -t upload  # flash (add --upload-port COMx if needed)
pio test -e native                # host unit tests (Unity, needs gcc)
pio test -e native -f test_juego  # a single test suite
```

## Architecture

- **Pure logic vs hardware.** `lib/antirrebote`, `lib/autos` and `lib/juego` don't touch registers and are unit-tested on the host (`test/test_*`). The `native` env `lib_ignore`s the hardware libs (`tira`, `display`, `temporizador`, `sonido`, `efectos`); keep new hardware code out of the pure modules so the tests keep compiling.
- `src/main.c` uses its own `main()` (no `setup/loop`). Loop: `temporizador_ms()` + `temporizador_botones()` → `juego_paso()` returns an `EV_*` bitmask → `sonar_eventos()` picks one sound by priority → `actualizar_motor()` (engine pitch from the leader's speed, only in `EST_CARRERA`) → `efectos_dibujar()` → `display_vueltas()`.
- `lib/juego`: state machine INVITACION → ARMADO → SEMAFORO → CARRERA → FESTEJO. The header comment of `juego.c` has the state diagram. Inputs are a `BOTON_*` mask of debounced *held* states; edges are derived inside. Physics runs in 1 ms steps catching up from `ultimo_paso` to `ahora`, because the loop can take several ms (strip refresh).
- `lib/autos`: fixed-point physics (position in 1/65536 LED, velocity in 1/65536 LED/ms). Impulse on rising edge only, proportional friction (rounded up so it reaches 0), cap below 1 LED/ms so movement never skips LEDs.
- **Timers.** Timer0 is reprogrammed to a 1 ms CTC interrupt (`lib/temporizador`): ms clock, display multiplexing and two-edge debounce of the 3 buttons. Arduino `millis()`/`delay()` don't work. Timer1 makes the tone as hardware PWM on OC1B/PB2 (passive) and Timer2 is the 100 Hz audio sequencer (`lib/sonido`). Timer1 is unused with the active buzzer.
- Buzzer type is chosen at compile time by env (`-D BUZZER_ACTIVO` / `-D BUZZER_PASIVO`). `sonido.c` `#error`s if neither is defined. In active mode a note just drives PB2 high while its volume ≥ 64; `sonido_motor()` is a no-op.
- All tunables live in `lib/ajustes/ajustes.h` (each documented with unit and effect). The README "Ajustes" table maps symptoms to these names; keep both in sync.
- LED buffer: a single `leds[NL]` in `lib/tira` (960 B, about half the RAM). Don't add a second buffer. `tira_pintar()` ignores `pos >= NL`.

## Pin map

| Pin | Use |
|-----|-----|
| PB2 (D10) | Buzzer (OC1B) |
| PB3, PB4, PB5 | Buttons J1, J2, select. Active-low with internal pull-ups; select (D13, onboard LED) needs a strong external pull-up (~470 Ω; 4.7 kΩ is NOT enough: the LED+1 kΩ to GND holds the pin near 2.3 V, read as pressed) |
| PC4 / PC5 | LED strip data / clock (APA102/SK9822) |
| PC0–PC3 | 7-seg digit enables (left = J1 laps, right = J2 laps) |
| PB0–PB1, PD2–PD7 | 7-seg segments (common anode, active-low) |

## Design docs

`docs/superpowers/specs/` and `docs/superpowers/plans/` hold the design and implementation plan for the state-machine/audio rewrite, including which decisions came from the user.
