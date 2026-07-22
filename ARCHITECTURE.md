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
| `display_manager` | Six-digit TM1637 mode and score output |
| `power_mode_manager` | Bench/PSU limits and runtime mode switching |
| `power_stress_test` | Abortable 10-second full-strip load test |
| `game_manager` | Game selection, confirmation, dispatch, and exit behavior |
| `games/catch_1d` | Source-games profile: accelerating one-button timing game |
| `games/colour_gate` | Source-games profile: four-color timing gate |
| `games/twang` | Encoder dungeon movement, attack, jump, score, and level states |
| `games/meteor_dodge` | Inactive retained source: encoder-tested dodge prototype |
| `games/memory_sequence` | Inactive retained source: four-button sequence memory game |
| `games/colour_shooter` | Incoming targets, shots, lives, and impact effects |
| `games/colour_snake_duel` | Pixel-moving snake halves, shots, rounds, and scores |
| `games/pong_1d` | Ball, paddles, scoring, point delay, and serve states |
| `games/tennis_1d` | Dual-encoder rackets, flight, bounce, returns, and score |
| `games/reaction_race` | Random start, false starts, alternating inputs, rounds |
| `games/snake_1d` | Inactive retained source: continuous four-button color snake |

Pins are centralized in `include/pins.h`. Tunable values are centralized in
`include/config.h`.

## Memory rules

- Keep one global RGB framebuffer: 288 x 3 = 864 bytes.
- Use fixed-size arrays only.
- Do not use Arduino `String`, heap allocation, or large local arrays.
- Keep serial strings inside `F()` so they remain in flash.
- Render game objects directly into the shared LED buffer.
- Check both SRAM and flash after every playable step.

Enhanced six-game default build, before the Tennis 1D hardware play test:

- Static SRAM: 1748 / 2560 bytes
- Flash: 27508 / 28672 bytes
- Active game states: Colour Shooter 124 bytes, Colour Snake Duel 82 bytes,
  Tennis 52 bytes, Twang 34 bytes, Pong 24 bytes, and Reaction Race 24 bytes

The remaining 812 SRAM bytes also contain the runtime stack. Meteor Dodge,
Snake, and Memory are not instantiated or dispatched in the default profile,
so link-time optimization removes their firmware code. Only 1164 flash bytes
remain there.

The `sparkfun_promicro16_source_games` profile uses a separate compile-time
catalog containing only the new Catch 1D and Colour Gate games. It uses 1499
bytes of SRAM and 19250 bytes of flash. This keeps experimental games isolated
from the default image and leaves ample room for further prototypes.

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
- `GAME_PIXEL_WIDTH` defines object width only. Colour Shooter targets and
  shots, the Pong ball, Colour Snake growth, Twang player interpolation, and
  Reaction Race trails all advance through physical LED coordinates rather
  than jumping by a full 12-LED game segment.
- Tennis rackets move in physical pixels. Its airborne ball uses time-based
  interpolation, and its post-bounce ball advances one physical pixel per step.
- The TM1637 driver retains only six previous segment bytes and one state byte.
  It compares display content every rendered frame but transmits only after a
  visible change. The hardware-validated 100 us half-clock timing follows the
  conservative reference-driver default.
- Both encoder pairs use CHANGE interrupts, the shared quadrature transition
  table, four transitions per detent, and saturating pending deltas. Display
  and LED transfers therefore cannot hide normal player rotation.
- No large local arrays were found. The 812-byte SRAM reserve still includes
  the unknown runtime stack, so a stack high-water measurement is recommended
  before adding another persistent buffer.

## Player and system input boundary

Player 1 owns D0/D1 plus red/green. Player 2 owns D2/D3 plus blue/yellow. Both
encoder directions are independently configurable and were reversed together
after the last hardware check. Twang consumes Player 1 encoder movement, uses
red for attack, and green for a contextual one-obstacle jump. Tennis consumes
both encoder deltas and no gameplay button. No other active game or selector
consumes encoder deltas.

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
6. Encoder speed should directly control movement speed. Prefer one action
   button and never exceed two; do not add mechanics without a clear purpose.
7. Add the mode glyph to `display_manager` and place the game in the appropriate
   compile-time `GAME_CATALOG` and dispatch blocks in `game_manager.cpp`.
8. Keep power stress available only from the game selector.
9. Document input controls and every visible selector/strip output.
10. Build without warnings, inspect SRAM/flash, test on hardware, then commit.

## Review notes

- Six controller-focused games are selectable in the default profile. The
  source-games profile selects Catch 1D and Colour Gate; Meteor Dodge, Memory
  Sequence, and Snake 1D remain inactive source code.
- Both encoder decoders and installed hardware behavior are validated. Twang is
  the validated 1P encoder game; Tennis is the first 2P dual-encoder game and
  still needs its gameplay hardware test.
- The encoder and six-digit TM1637 display are integrated and hardware-tested.
  All six digits and all six decimal points are validated on A0/A1.
- FastLED current limiting is an estimate. The measured 3000 mA setting draws
  approximately 3.5 A during the full-white test.
