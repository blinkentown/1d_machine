# 1d_machine

`1d_machine` is a compact game console built around one 288-pixel
APA102/SK9822 strip and a SparkFun Pro Micro 5 V / 16 MHz.

## Current status

| Selector output | Game | Status |
| --- | --- | --- |
| Orange | Twang | Planned |
| Yellow | Colour Shooter | Playable |
| Blue | 1D Pong | Playable |
| Green | Reaction Race | Planned |
| Cyan | Snake 1D | Playable |
| Red | Meteor Dodge | Planned |
| Purple | Memory Sequence | Planned |

Detailed controls are in [GAMES.md](GAMES.md). Power wiring and runtime power
modes are in [POWER_MODES.md](POWER_MODES.md). Firmware structure and memory
rules are in [ARCHITECTURE.md](ARCHITECTURE.md).

## Hardware

- SparkFun Pro Micro, ATmega32U4, 5 V, 16 MHz
- One 288-pixel APA102/SK9822 strip on hardware SPI
- Four color game buttons: red, green, blue, yellow
- One illuminated selector button with a NeoPixel
- Optional rotary encoder and setup button inputs
- External fused 5 V supply for the LED strip

The controller, strip, and PSU require a common ground. USB and PSU positive
rails must not be tied together without proper power-source isolation.

## Pin assignment

| Function | Arduino pin |
| --- | ---: |
| APA102 clock / SCK | D15 |
| APA102 data / MOSI | D16 |
| Encoder A / B | D2 / D3 |
| Encoder click | D4 |
| Setup button | D5 |
| Red / green / blue / yellow buttons | D6 / D7 / D8 / D9 |
| Selector NeoPixel data | D10 |
| Selector switch | D14 |

All switches use `INPUT_PULLUP`: released is `HIGH`, pressed is `LOW`.

For non-color games, the additional labels are red `P1-A/Left`, green
`P1-B/Right`, blue `P2-A/Action`, and yellow `P2-B/Special`. See
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

## Main configuration

All tunable constants live in `include/config.h`.

- Tape pixel width: 3 LEDs
- Game segment multiplier: 4
- Logical game segment: 12 LEDs
- Global gameplay speed: 100%
- Snake gameplay speed: 80%
- Bench limit: 100 mA at brightness 32/255
- PSU limit: FastLED estimate of 3000 mA at brightness 85/255
- Measured full-white PSU current: approximately 3.5 A
- Full-white stress-test duration: 10 seconds; any button aborts

## Memory baseline

The reviewed three-game build uses:

- SRAM: 1802 / 2560 bytes (70.4%)
- Flash: 22724 / 28672 bytes (79.3%)

The SRAM figure does not include peak stack usage. Future games must use small,
fixed state and no additional LED framebuffer.
