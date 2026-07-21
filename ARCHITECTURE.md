# Firmware architecture

## Runtime flow

`src/main.cpp` performs one non-blocking update loop:

1. Debounce buttons and decode the rotary encoder.
2. Update the startup/runtime power mode.
3. Update the selector, power stress test, or active game state machine.
4. Render at the configured frame interval.

There are no gameplay delays or dynamic allocations.

## Module ownership

| Module | Responsibility |
| --- | --- |
| `input_manager` | Debounced active-low buttons and encoder decoding |
| `controls.h` | Zero-cost player and one-player aliases for color buttons |
| `led_manager` | The only 288-pixel framebuffer, selector pixel, FastLED output |
| `display_manager` | Six-digit TM1637 mode, score, and life output |
| `power_mode_manager` | Bench/PSU limits and runtime mode switching |
| `power_stress_test` | Abortable 10-second full-strip load test |
| `game_manager` | Game selection, confirmation, dispatch, and exit behavior |
| `games/twang` | Cell-mask dungeon, movement, dash, attack, and level states |
| `games/meteor_dodge` | Warning, movement, dash, shield, and impact states |
| `games/memory_sequence` | Seeded sequence playback, input, and round states |
| `games/colour_shooter` | Incoming targets, shots, lives, and impact effects |
| `games/pong_1d` | Ball, paddles, scoring, point delay, and serve states |
| `games/reaction_race` | Random start, false starts, alternating inputs, rounds |
| `games/snake_1d` | Continuous segment queue, rainbow bonus, shots, and combo |

Pins are centralized in `include/pins.h`. Tunable values are centralized in
`include/config.h`.

## Memory rules

- Keep one global RGB framebuffer: 288 x 3 = 864 bytes.
- Use fixed-size arrays only.
- Do not use Arduino `String`, heap allocation, or large local arrays.
- Keep serial strings inside `F()` so they remain in flash.
- Render game objects directly into the shared LED buffer.
- Check both SRAM and flash after every playable step.

Reviewed encoder-and-display build baseline:

- Static SRAM: 1897 / 2560 bytes
- Flash: 26664 / 28672 bytes
- Largest game states: Snake 240 bytes, Colour Shooter 124 bytes, Twang 29
  bytes, Pong 24 bytes, Meteor Dodge 20 bytes, Reaction Race 18 bytes,
  Memory Sequence 15 bytes

The remaining 663 SRAM bytes also contain the runtime stack. All seven game
states currently fit as fixed globals. Any substantial future game or feature
should overlay inactive game states in shared storage and consolidate repeated
projectile/effect code first. The remaining 2008 flash bytes are maintenance
reserve, not a target for another large game.

The AVR build enables link-time optimization, shared function prologues,
reduced small-function inlining, unsplit wide values, and linker relaxation to
preserve flash for game logic.

## Performance review

- The render interval is 20 ms, or at most 50 frames per second.
- The compiled APA102 hardware-SPI divider is 2, which is 8 MHz on the 16 MHz
  ATmega32U4. One 288-pixel APA102 transfer is about 1196 protocol bytes, or
  about 1.2 ms of raw wire time. Rendering, FastLED power calculation, and the
  one-pixel NeoPixel transfer add CPU time, but the LED transfer is well below
  the 20 ms frame budget.
- Input and game updates remain non-blocking. Detailed serial event diagnostics
  are compiled out by default; the USB startup message remains.
- The TM1637 driver retains only six previous segment bytes and one state byte.
  It compares display content every rendered frame but transmits only after a
  visible change. The hardware-validated 100 us half-clock timing follows the
  conservative reference-driver default.
- Encoder A/B are polled with 2 ms debounce. Normal hand rotation and encoder
  click are hardware-validated; very fast movement can still skip transitions
  during LED output. D2/D3 support an interrupt-based decoder later if polling
  proves insufficient.
- No large local arrays were found. The 663-byte SRAM reserve still includes
  the unknown runtime stack, so a stack high-water measurement is recommended
  before adding another persistent buffer.

## Rotary encoder status

The firmware already initializes D2/D3 with `INPUT_PULLUP`, debounces both
signals, decodes the quadrature transition table, accumulates four transitions
per detent, and applies `Config::ENCODER_DIRECTION`. At the selector, rotation
moves backward or forward through all seven games and the D4 encoder click
starts the selected game. Normal selection, reverse selection, and click
behavior are hardware-validated.

## Score and game-mode display

The six-digit TM1637 module uses A0 for `CLK` and A1 for `DIO`; its power pins
connect to 5 V and common ground. The protocol is bit-banged directly because
D2/D3 remain dedicated to the encoder. The driver ignores acknowledgements so
the firmware also runs normally when the display is disconnected.

No display library, heap allocation, text buffer, or framebuffer is used. Each
game exposes only its small numeric status through constant-time getters. The
first two digits hold a fixed game abbreviation. The last four show one
four-digit value or two two-digit player scores. Decimal points serve as the
life indicators or the divider between player scores.

The installed module exposes its physical grids in `3-2-1-6-5-4` order and is
viewed rotated by 180 degrees. The final output transform therefore remaps the
six grid addresses, reverses the viewing order, and rotates segments A through
F while preserving the center segment and decimal-point bit.

## Adding a game

1. Add a small state-machine class under `include/games` and `src/games`.
2. Use `Config::GAME_PIXEL_WIDTH` for logical object width.
3. Use `Config::EXPLOSION_INTENSITY` as the base effect scale.
4. Apply `Config::gameplayInterval()` to movement timings.
5. Use the aliases in `controls.h` when controls are actions rather than
   colors.
6. Add the game to the explicit implemented-game checks and dispatch in
   `game_manager.cpp`.
7. Keep power stress available only from the game selector.
8. Document input controls and every visible selector/strip output.
9. Build without warnings, inspect SRAM/flash, test on hardware, then commit.

## Review notes

- All seven selectable games are implemented.
- Encoder decoding, selector integration, and installed hardware behavior are
  validated.
- The encoder and six-digit TM1637 display are integrated and hardware-tested.
  All six digits and all six decimal points are validated on A0/A1.
- FastLED current limiting is an estimate. The measured 3000 mA setting draws
  approximately 3.5 A during the full-white test.
