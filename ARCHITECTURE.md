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
| `games/colour_shooter` | Incoming targets, shots, lives, and impact effects |
| `games/pong_1d` | Ball, paddles, scoring, point delay, and serve states |
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

- Static SRAM: 1796 / 2560 bytes
- Flash: 22160 / 28672 bytes
- Largest game states: Snake 240 bytes, Colour Shooter 124 bytes, Pong 17
  bytes

The remaining 764 SRAM bytes also contain the runtime stack. Before adding all
four planned games, active game states should be overlaid in shared storage and
duplicate projectile/explosion code should be consolidated.

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

- Colour Shooter, 1D Pong, and Snake 1D are implemented.
- Twang, Reaction Race, Meteor Dodge, and Memory Sequence remain selectable
  placeholders and do not start when confirmed.
- Encoder decoding exists in firmware, but the current hardware build does not
  depend on the encoder.
- FastLED current limiting is an estimate. The measured 3000 mA setting draws
  approximately 3.5 A during the full-white test.
