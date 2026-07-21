# Controls and games

## Selector and power-mode outputs

At the game selector, short-press the illuminated selector to advance one game.
Hold it for about 0.8 seconds to confirm the displayed game.

To change power mode without restarting, remain at the game selector and hold
the blue and yellow game buttons together for two seconds.

| Action or state | Illuminated selector output |
| --- | --- |
| Power-mode hold in progress | Yellow |
| PSU mode confirmed | Green for one second |
| Bench mode confirmed | Blue for one second |
| Colour Shooter selected/running | Yellow |
| 1D Pong selected/running | Blue |
| Snake 1D selected/running | Cyan |

After a one-second power confirmation, the selector output returns to the
selected game's color. Blue can therefore mean either the brief bench
confirmation or the persistent 1D Pong selection.

## Starting a game

1. Short-press the illuminated selector until it shows the desired game color.
2. Hold the illuminated selector for about 0.8 seconds.
3. Observe the preflight outputs:
   - Off: dark baseline.
   - White with one white strip pixel: single-pixel test.
   - White with five white strip pixels: game-load test.
   - PSU mode only: all 288 strip pixels white for 2 seconds.
   - Green: preflight ready.
4. While the selector output is green, hold it again for about 0.8 seconds.
5. The selected game starts and the selector returns to its game color.

Press the illuminated selector during any running game to return to game
selection. The selector output then shows the selected game's color.

## Colour Shooter

Selector output: yellow.

- Random colored comets move from the far end toward the player.
- Red, green, blue, or yellow launches a matching projectile.
- The nearest comet blocks the projectile.
- A correct match dissolves the target and increases score and speed.
- A wrong projectile disappears while the target remains.
- A comet reaching the player removes one of three green life indicators.
- Three lost lives produce a flashing red game-over output near the strip
  center.
- Any color game button restarts after game over.

## 1D Pong

Selector output: blue.

- Red button: left player hit.
- Blue button: right player hit.
- Yellow button: pause or resume.
- The 12-LED ball accelerates after successful returns.
- Each visible 24-LED paddle is also the complete hit zone.
- First player to five points wins.
- Game-over output: the winning 24-LED paddle flashes red on the left or blue
  on the right.
- Any color game button restarts after game over.

## Snake 1D

Selector output: cyan.

- A continuous row of 12-LED colored segments moves toward the player.
- Adjacent normal segments always have different colors.
- Fire the color matching the leading normal segment.
- Wrong shots are blocked and reset the combo.
- The rainbow bonus occupies three logical segments, or 36 LEDs.
- Any three color shots dissolve the rainbow bonus one layer at a time.
- Correct-hit combos increase blast intensity.
- Snake speed increases every four hits.
- Inside the final 60 LEDs, the snake gradually slows by up to 30%.
- A breach removes one of three green life indicators and pushes the snake
  back 24 LEDs.
- Three breaches produce a flashing 36-LED red game-over output at the strip
  center.
- Any color game button restarts after game over.
