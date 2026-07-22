# 1d_machine

`1d_machine` is a compact game console built around one 288-pixel
APA102/SK9822 strip and a SparkFun Pro Micro 5 V / 16 MHz.

## Current status

| Players | Selector output | Game | Status |
| ---: | --- | --- | --- |
| 1 | Orange | Twang | Playable |
| 1 | Yellow | Colour Shooter | Playable |
| 2 | Blue | 1D Pong | Playable |
| 2 | Green | Reaction Race | Playable |

Meteor Dodge, Snake 1D, and Memory Sequence remain in the source tree but are
intentionally not linked into the selectable firmware.

Detailed controls are in [GAMES.md](GAMES.md). Power wiring and runtime power
modes are in [POWER_MODES.md](POWER_MODES.md). Firmware structure and memory
rules are in [ARCHITECTURE.md](ARCHITECTURE.md).

## Hardware

- SparkFun Pro Micro, ATmega32U4, 5 V, 16 MHz
- One 288-pixel APA102/SK9822 strip on hardware SPI
- Four color game buttons: red, green, blue, yellow
- One illuminated selector button with a NeoPixel
- Two hardware-validated player encoders
- Six-digit, six-decimal-point TM1637 score display
- External fused 5 V supply for the LED strip

The controller, strip, and PSU require a common ground. USB and PSU positive
rails must not be tied together without proper power-source isolation.

## Pin assignment

| Function | Arduino pin |
| --- | ---: |
| APA102 clock / SCK | D15 |
| APA102 data / MOSI | D16 |
| Player 1 encoder A / B | D0 / D1 |
| Player 2 encoder A / B | D2 / D3 |
| Reserved Player 2 encoder click | D4 |
| Setup button | D5 |
| Red / green / blue / yellow buttons | D6 / D7 / D8 / D9 |
| Selector NeoPixel data | D10 |
| Selector switch | D14 |
| TM1637 display clock / CLK | A0 |
| TM1637 display data / DIO | A1 |
| Free direct GPIO | A2 / A3 |

All switches use `INPUT_PULLUP`: released is `HIGH`, pressed is `LOW`.

Player 1 owns red `P1-A` and green `P1-B`; Player 2 owns blue `P2-A` and
yellow `P2-B`. Both encoders are installed and decoded but currently reserved
while gameplay behavior is introduced one game at a time. Twang is the next
planned encoder integration. The
illuminated selector and setup button are system controls. A2 and A3 are the
only two currently exposed, completely free direct GPIOs. See
[GAMES.md](GAMES.md) for per-game assignments.

## Build and upload

```powershell
# Build only
platformio run -e sparkfun_promicro16

# Build and upload
powershell -ExecutionPolicy Bypass -File .\tools\upload_promicro.ps1
```

The Windows upload helper discovers ports from the Pro Micro USB VID/PID. It
does not assume fixed application or bootloader COM numbers. It builds the
firmware, identifies the connected application interface, and then waits for
one quick `RST`-to-`GND` contact. After Windows exposes the bootloader on
whichever COM port it assigns, the helper waits 350 ms for that interface to
stabilize before starting AVR109 programming and verification. The normal
application interface is never accepted as the bootloader if the short reset
window is missed; the helper keeps waiting instead of uploading to the wrong
port.
This unit has been observed as `1B4F:9206` in application mode and as an
Arduino Leonardo bootloader with `2341:0036` during upload. Both identities are
included; COM numbers are still discovered dynamically.

If more than one compatible Pro Micro is connected, the helper refuses to
guess. Select the intended application interface explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\upload_promicro.ps1 `
    -ApplicationPort COM7
```

The COM number above is only an example. Check the detected ports on every
upload. Use exactly one reset contact after the watcher is armed; an additional
reset can interrupt the bootloader while it is programming.

The serial monitor runs at 115200 baud.

## Score display

Connect the four-pin TM1637 module as follows: `VCC` to 5 V, `GND` to common
ground, `CLK` to A0, and `DIO` to A1. It is a two-wire TM1637 interface, not
I2C. The firmware uses a small allocation-free driver and updates the module
only when its content changes.

The installed display is viewed rotated by 180 degrees. The firmware corrects
both the module's `3-2-1-6-5-4` grid order and the rotated segment geometry;
`TM1637_ROTATE_180` in `include/config.h` records that mounting orientation.

During selection the display shows player count and a three-digit game code:
`1P tNG`, `1P CSH`, `2P PnG`, or `2P rAC`. During play the display
is split into two three-digit score fields. Player 1 is on the left and Player
2 is on the right. The right field stays blank in a single-player game. Values
are right-aligned without leading zeroes and saturate at 999. Lives remain
visible on the LED strip rather than as display decimal points.

## Main configuration

All tunable constants live in `include/config.h`.

Gameplay uses encoders for directional movement and should need one action
button where possible, never more than two. Faster encoder rotation directly
produces faster movement; simple controls take priority over extra mechanics.

- Tape pixel width: 3 LEDs
- Game segment multiplier: 4
- Logical game segment: 12 LEDs
- Global gameplay speed: 100%
- Bench limit: 100 mA at brightness 32/255
- PSU limit: FastLED estimate of 3000 mA at brightness 85/255
- Measured full-white PSU current: approximately 3.5 A
- Full-white stress-test duration: 10 seconds; any button aborts
- Render interval: 20 ms (50 frames per second)

## Memory baseline

The reviewed four-game build with two encoders and TM1637 display uses:

- SRAM: 1582 / 2560 bytes (61.8%)
- Flash: 21026 / 28672 bytes (73.3%)

The SRAM figure does not include peak stack usage. Future games must use small,
fixed state and no additional LED framebuffer.

## Player and system inputs

Both player encoders use interrupt-driven quadrature decoding. Their configured
directions have been reversed together from the last hardware test. The Player
encoder deltas are currently ignored by all four active games and the selector
while integration proceeds gradually. Twang is planned as the first active
encoder-controlled game.
Short-press the illuminated selector to cycle games, hold it for about 0.8
seconds to start, and press it during a game to return. D5 remains the dedicated
setup input. A2/A3 are reserved for future system controls; until then,
power-mode chords temporarily use player buttons only while the selector is
active.
