# Firmware architecture

## Runtime flow

`src/main.cpp` performs one non-blocking update loop:

1. Debounce buttons and snapshot both interrupt-driven player encoders.
2. Update the startup/runtime power mode.
3. Update the selector, power stress test, or active game state machine.
4. Render at the configured frame interval.

There are no gameplay delays or dynamic allocations.

## Module ownership

| Module | Responsibility |
| --- | --- |
| `input_manager` | Debounced active-low buttons and encoder decoding |
| `controls.h` | Logical P1/P2 button aliases and reserved encoder adapter |
| `led_manager` | The only 288-pixel framebuffer, selector pixel, FastLED output |
| `display_manager` | Six-digit TM1637 mode, score, and life output |
| `power_mode_manager` | Bench/PSU limits and runtime mode switching |
| `power_stress_test` | Abortable 10-second full-strip load test |
| `game_manager` | Game selection, confirmation, dispatch, and exit behavior |
| `games/twang` | Cell-mask dungeon, movement, dash, attack, and level states |
| `games/meteor_dodge` | Warning, movement, dash, shield, and impact states |
| `games/memory_sequence` | Inactive retained source: sequence memory game |
| `games/colour_shooter` | Incoming targets, shots, lives, and impact effects |
| `games/pong_1d` | Ball, paddles, scoring, point delay, and serve states |
| `games/reaction_race` | Random start, false starts, alternating inputs, rounds |
| `games/snake_1d` | Inactive retained source: continuous color snake |

Pins are centralized in `include/pins.h`. Tunable values are centralized in
`include/config.h`.

## Memory rules

- Keep one global RGB framebuffer: 288 x 3 = 864 bytes.
- Use fixed-size arrays only.
- Do not use Arduino `String`, heap allocation, or large local arrays.
- Keep serial strings inside `F()` so they remain in flash.
- Render game objects directly into the shared LED buffer.
- Check both SRAM and flash after every playable step.

Reviewed five-game, two-encoder, and display build baseline:

- Static SRAM: 1606 / 2560 bytes
- Flash: 22240 / 28672 bytes
- Active game states: Colour Shooter 124 bytes, Twang 29 bytes, Pong 24 bytes,
  Meteor Dodge 19 bytes, and Reaction Race 18 bytes

The remaining 954 SRAM bytes also contain the runtime stack. Snake and Memory
are not instantiated or dispatched, so link-time optimization removes their
firmware code while their source remains available. The remaining 6432 flash
bytes provide headroom for system UI and focused gameplay improvements.

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
- Both encoder pairs use CHANGE interrupts, the shared quadrature transition
  table, four transitions per detent, and saturating pending deltas. Display
  and LED transfers therefore cannot hide normal player rotation.
- No large local arrays were found. The 954-byte SRAM reserve still includes
  the unknown runtime stack, so a stack high-water measurement is recommended
  before adding another persistent buffer.

## Player and system input boundary

Player 1 owns D0/D1 plus red/green. Player 2 owns D2/D3 plus blue/yellow. Both
encoder directions are independently configurable and were reversed together
after the last hardware check. Meteor Dodge is the first game to consume the
Player 1 encoder: one detent moves one logical cell, red dashes, and green
activates a shield. All other games and the selector ignore encoder deltas
until each use is designed and tested separately.

The illuminated selector owns cycle, start, and return behavior. D5 is the
setup input. The existing D4 encoder click is reserved and ignored during
normal selection and gameplay to keep the two player interfaces symmetric.
A2/A3 are the only two currently exposed and completely free direct GPIOs;
they remain available for future dedicated system-option buttons.
Power-mode and stress-test chords are the documented temporary exception:
they use player buttons only while the selector is active.

## Score and game-mode display

The six-digit TM1637 module uses A0 for `CLK` and A1 for `DIO`; its power pins
connect to 5 V and common ground. The protocol is bit-banged directly because
D2/D3 remain dedicated to the encoder. The driver ignores acknowledgements so
the firmware also runs normally when the display is disconnected.

No display library, heap allocation, text buffer, or framebuffer is used. At
selection, the six digits show player count plus a three-digit game
abbreviation. During a game, the left three digits are reserved for Player 1
and the right three for Player 2. Single-player games leave the right field
blank. Each score saturates at 999 and is right-aligned without leading zeroes;
lives remain represented on the LED strip.

The installed module exposes its physical grids in `3-2-1-6-5-4` order and is
viewed rotated by 180 degrees. The final output transform therefore remaps the
six grid addresses, reverses the viewing order, and rotates segments A through
F while preserving the center segment and decimal-point bit.

## Adding a game

1. Add a small state-machine class under `include/games` and `src/games`.
2. Use `Config::GAME_PIXEL_WIDTH` for logical object width.
3. Use `Config::EXPLOSION_INTENSITY` as the base effect scale.
4. Apply `Config::gameplayInterval()` to movement timings.
5. Use the documented logical button aliases in `controls.h` for gameplay;
   do not enable an encoder without a separate control-design and play test.
6. Add the game to the explicit implemented-game checks and dispatch in
   `game_manager.cpp`.
7. Keep power stress available only from the game selector.
8. Document input controls and every visible selector/strip output.
9. Build without warnings, inspect SRAM/flash, test on hardware, then commit.

## Review notes

- Five controller-focused games are selectable. Snake and Memory remain as
  inactive source code.
- Both encoder decoders and installed hardware behavior are validated. Meteor
  Dodge is the first isolated encoder gameplay test; Twang is planned next.
- The encoder and six-digit TM1637 display are integrated and hardware-tested.
  All six digits and all six decimal points are validated on A0/A1.
- FastLED current limiting is an estimate. The measured 3000 mA setting draws
  approximately 3.5 A during the full-white test.
