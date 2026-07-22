# Power modes

One firmware contains two runtime power modes. Upload it once at the desk; no
upload connection is required when the machine is powered from its PSU.

## Bench mode

Bench mode is selected on every normal boot.

- FastLED limit: 100 mA.
- Global brightness: 32/255.
- Games start directly without a power preflight.

## PSU mode

To enter PSU mode, hold both the Blue and Yellow game buttons while powering
on. Keep both held for two seconds. Releasing either button early cancels the
arming sequence and leaves the firmware in bench mode.

The power mode can also be changed without restarting:

1. Return to the game selector.
2. Hold the Blue and Yellow game buttons together for two seconds.
3. Red output confirms PSU mode; green output confirms bench mode.

The confirmation lasts one second, then the illuminated selector returns to
the selected game's color. The runtime chord is ignored during a game.

These player-button chords are a temporary selector-only bridge until a
dedicated system-options control is added on the reserved A2/A3 inputs. Games
receive player inputs through a separate logical interface and cannot trigger
power-mode changes.

- FastLED limit: 3000 mA.
- Global brightness: 85/255. FastLED scales dense frames to the configured
  current ceiling while allowing sparse game effects to use the extra
  brightness.
- Measured full-white current with the 3000 mA software setting and 85/255
  brightness: approximately 3.5 A.
- The selected mode remains active until changed or reset.
- Every normal restart returns to bench mode.

## Power stress test

The stress test uses the limits of the currently selected bench or PSU mode.
It is available only at the game selector.

1. Hold the Red and Blue game buttons together for two seconds.
2. Red/blue alternating output shows hold progress.
3. At activation, all 288 strip pixels turn white for up to 10 seconds.
4. The illuminated selector output is red under PSU limits or green under
   bench limits.
5. Press any button to stop immediately. Completion also clears the strip
   automatically.

Verify actual current during the test. FastLED limiting is an estimate, not a
current sensor.

Build and upload the single firmware at the desk:

```powershell
platformio run -e sparkfun_promicro16
powershell -ExecutionPolicy Bypass -File .\tools\upload_promicro.ps1
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

Simultaneous external power and USB data requires proper isolation between the
USB and PSU positive rails. Their grounds must remain common.

Before a PSU test, verify polarity, common ground, fusing, and that the strip
receives power injection at the planned points. The software limit does not
measure current and cannot detect a wiring fault.
