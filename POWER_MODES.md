# Power modes

One firmware contains two runtime power modes. Upload it once at the desk; no
upload connection is required when the machine is powered from its PSU.

## Bench mode

Bench mode is selected on every normal boot.

- FastLED limit: 100 mA.
- Global brightness: 32/255.
- The power preflight stops after the five-pixel game-load preview.
- The 288-pixel white stage remains runtime-locked.

## PSU mode

To enter PSU mode, hold both the Blue and Yellow game buttons while powering
on. Keep both held for two seconds. Releasing either button early cancels the
arming sequence and leaves the firmware in bench mode.

- FastLED limit: 6000 mA.
- Global brightness: 16/255 for the first full-strip measurement.
- The power preflight adds a timed 10-second stage with all 288 pixels white.
- The strip is cleared automatically after the measurement stage.
- PSU mode remains latched until reset or power-off.
- Every normal restart returns to bench mode.

Build and upload the single firmware at the desk:

```powershell
platformio run -e sparkfun_promicro16
platformio run -e sparkfun_promicro16 -t upload
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
