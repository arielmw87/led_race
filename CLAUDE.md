# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Bare-metal AVR firmware (PlatformIO, Arduino Nano / ATmega328P @ 16 MHz) for a two-player LED-strip race game: each player mashes a button to advance their light along a 240-LED strip; first to the end scores, 3 points wins. Code and comments are in Spanish.

## Commands

```sh
pio run                      # build (env: nanoatmega328new)
pio run -t upload            # flash the Nano (add --upload-port COMx if needed)
pio run -t clean
```

There are no tests (`test/` only has the PlatformIO template README) and no linter configured.

## Architecture

- `src/main.cpp` defines its own `int main()` instead of Arduino `setup()/loop()`. It configures registers directly and then calls `mi_codigo()` forever. **Timer0 is reprogrammed** (CTC, prescaler 64, OCR0A=249 → 1 ms interrupt), so Arduino `millis()`, `delay()`, `micros()` do not work — use the `hola` tick counter instead.
- `lib/utiles/` (`utiles.h` / `utiles.cpp`) holds all logic and global state. PlatformIO auto-links it via `#include <utiles.h>`.
- `ISR(TIMER0_COMPA_vect)` (1 ms) does three things: multiplexes the 4-digit 7-segment display, increments the time counters (`hola`, `hola1`, `hola2`), and debounces the three buttons (active-low, must read 0 for 20 consecutive ticks to set `flag_botN`). Main-loop code does edge detection with `estadoan*` vs `flag_bot*`.
- Main loop (`mi_codigo`): `buzzer()` state machine → if no sound is playing (`modo_buzzer == 0`), run `jugador_1/2` (advance position on button press), `efecto_ganar_1/2` (score / win animations, blocking loops) → `boton_select` (reset).
- LED strip: APA102-style protocol bit-banged on PC4 (data) / PC5 (clock): 4-byte zero start frame, then per LED `0b111xxxxx|brightness`, B, G, R, then 4-byte end frame. State lives in the global `struct LED led[NL]` (NL=240); modify it then call `refresh_leds()` to push.
- Score display: `show_disp` is shown on the 7-seg; player 1 points are added as `+1000` (left digit), player 2 as `+1` (right digit). Segments are driven by `enmascarado()` across PB0–PB1 and PD2–PD7 (inverted/common-anode); digit select on PC0–PC3.
- Buzzer on PB5, non-blocking state machine driven by `hola`: `sonido_empieza()` sets `modo_buzzer=1`, `sonido_final()` sets `modo_buzzer=2`.

## Pin map

| Pin | Use |
|-----|-----|
| PB2, PB3, PB4 | Buttons 1 (player 1), 2 (player 2), 3 (select/reset); inputs with pull-ups |
| PB5 | Buzzer |
| PC4 / PC5 | LED strip data / clock |
| PC0–PC3 | 7-seg digit enables |
| PB0–PB1, PD2–PD7 | 7-seg segments |

## Gotchas

- Positions advance by 10 per press, so `posicion`/`posicion1` can exceed `NL - 1` before `efecto_ganar_*` resets them. The LED helpers (`color_*`, `apagar_led`) take `uint16_t` and ignore out-of-range indices; keep that guard if you add new helpers that index `led[]`.
- The win/score animations are blocking `for` loops; the ISR keeps running during them but game input is not processed.
