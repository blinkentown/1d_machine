# Power modes

The firmware has two compile-time power profiles. The bench binary does not
contain an active full-strip-white test path. The PSU profile must be selected
explicitly when building or uploading.

## Bench profile

Environment: `sparkfun_promicro16_bench`

- Default PlatformIO environment.
- FastLED limit: 100 mA.
- Global brightness: 32/255.
- Power preflight stops after the five-pixel game-load preview.
- The external strip power supply should be off and the strip power connection
  should be disconnected while working on USB power alone.

Build and upload:

```powershell
platformio run -e sparkfun_promicro16_bench
platformio run -e sparkfun_promicro16_bench -t upload
```

## PSU profile

Environment: `sparkfun_promicro16_psu`

- Must be selected explicitly.
- FastLED limit: 6000 mA.
- Global brightness: 16/255 for the first full-strip measurement.
- Adds a timed 10-second stage with all 288 pixels white.
- The strip is cleared automatically after the measurement stage.

Build and upload:

```powershell
platformio run -e sparkfun_promicro16_psu
platformio run -e sparkfun_promicro16_psu -t upload
```

## Power wiring boundary

Do not connect USB 5 V and the external PSU 5 V rails together.

One safe arrangement for serial monitoring is:

- USB powers only the Pro Micro and the selector pixel.
- The external fused 5 V supply powers only the APA102/SK9822 strip.
- Pro Micro ground and strip/PSU ground are connected.
- The two positive 5 V rails remain separate.

Alternatively, power the Pro Micro and strip from the regulated external 5 V
supply with USB completely disconnected.

Before a PSU test, verify polarity, common ground, fusing, and that the strip
receives power injection at the planned points. The software limit and preview
do not measure current and cannot detect a wiring fault.
