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

Reviewed build baseline:

- Static SRAM: 1904 / 2560 bytes
- SRAM sections: 658 bytes `.data` and 1246 bytes `.bss`
- Flash: 27918 / 28672 bytes
- Largest game states: Snake 240 bytes, Colour Shooter 124 bytes, Twang 29
  bytes, Pong 24 bytes, Meteor Dodge 20 bytes, Reaction Race 18 bytes,
  Memory Sequence 15 bytes

The remaining 656 SRAM bytes also contain the runtime stack. All seven game
states currently fit as fixed globals. Any substantial future game or feature
should overlay inactive game states in shared storage and consolidate repeated
projectile/effect code first. The remaining 754 flash bytes are maintenance
reserve, not a target for another game.

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
- Input and game updates remain non-blocking. Serial messages occur on state or
  score events, not once per frame.
- Encoder A/B are polled with 2 ms debounce. Normal hand rotation and encoder
  click are hardware-validated; very fast movement can still skip transitions
  during LED output. D2/D3 support an interrupt-based decoder later if polling
  proves insufficient.
- No large local arrays were found. The 656-byte SRAM reserve still includes
  the unknown runtime stack, so a stack high-water measurement is recommended
  before adding another persistent buffer.

## Rotary encoder status

The firmware already initializes D2/D3 with `INPUT_PULLUP`, debounces both
signals, decodes the quadrature transition table, accumulates four transitions
per detent, and applies `Config::ENCODER_DIRECTION`. At the selector, rotation
moves backward or forward through all seven games and the D4 encoder click
starts the selected game. Normal selection, reverse selection, and click
behavior are hardware-validated.

## Score and game-mode display constraints

- Hardware I2C uses D2/D3, which are already assigned to the encoder.
- Hardware SPI uses D15/D16 and is occupied by the APA102 string, which has no
  chip-select input. A display cannot safely share arbitrary SPI traffic with
  that string.
- A 128x64 one-bit framebuffer needs 1024 bytes; 128x32 needs 512 bytes. Both
  are unsafe with only 656 static bytes left for stack and future state.
- Only 754 application-flash bytes remain. The 67 current flash strings occupy
  approximately 1653 bytes before print code, so optional serial diagnostics
  are the first candidate for a display build profile, but removing them alone
  may not fit a general graphics library.
- The games do not yet expose a common score model: Pong and Reaction Race have
  two scores, Twang has level/lives, and the other games use different
  score/life/round values. A small `UiSnapshot` interface should be added before
  a display driver so display code does not depend on private game internals.

Recommended integration order:

1. Select the exact display and decide between free GPIO/software bus or a
   controller change.
2. Add a fixed-size, allocation-free `UiSnapshot` for mode and scores.
3. Reclaim flash with an optional reduced-serial build profile.
4. Add a low-RAM text or numeric driver, then repeat clean build, stack, timing,
   power, and hardware tests.

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
- No display driver or shared score/mode view model is implemented.
- FastLED current limiting is an estimate. The measured 3000 mA setting draws
  approximately 3.5 A during the full-white test.
