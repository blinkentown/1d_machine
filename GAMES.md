# Controls and games

## Physical button labels

The four game buttons are mounted left-to-right as red, green, blue, yellow.
Use these additional labels for games that do not directly match colors:

| Physical color | Two-player label | One-player label |
| --- | --- | --- |
| Red | P1-A | Left |
| Green | P1-B | Right |
| Blue | P2-A | Action |
| Yellow | P2-B | Special |

Color-matching games use red, green, blue, and yellow as colors rather than
these action labels. There is currently no dedicated pause input.

## Selector and power-mode outputs

At the game selector, short-press the illuminated selector to advance one game.
Hold it for about 0.8 seconds to confirm the displayed game.

To change power mode without restarting, remain at the game selector and hold
the blue and yellow game buttons together for two seconds.

| Action or state | Illuminated selector output |
| --- | --- |
| Power-mode hold in progress | Yellow |
| PSU mode confirmed | Red for one second |
| Bench mode confirmed | Green for one second |
| Stress-test hold progress | Alternating red and blue |
| Stress test with PSU limits | Red; strip output is solid white |
| Stress test with bench limits | Green; strip output is solid white |
| Colour Shooter selected/running | Yellow |
| 1D Pong selected/running | Blue |
| Snake 1D selected/running | Cyan |

After a one-second power confirmation, the selector output returns to the
selected game's color. Red can therefore be the brief PSU confirmation or the
Meteor Dodge selection; green can be the brief bench confirmation or the
Reaction Race selection.

## Starting a game

1. Short-press the illuminated selector until it shows the desired game color.
2. Hold the illuminated selector for about 0.8 seconds.
3. An implemented game starts immediately. Its selector output remains the
   selected game's color.

Press the illuminated selector during any running game to return to game
selection. The selector output then shows the selected game's color.

## Running the power stress test

1. Return to game selection.
2. Hold the red and blue game buttons together for two seconds.
3. During the hold, the selector and progress output alternate red and blue.
4. The strip output becomes solid white for up to 10 seconds.
5. Selector output red means PSU limits; selector output green means bench
   limits.
6. Press any button to stop immediately. The strip turns off and the selector
   returns to the selected game's color.

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

- Red / P1-A: left player hit.
- Blue / P2-A: right player hit.
- Green / P1-B and yellow / P2-B: unused.
- The 12-LED ball accelerates after successful returns.
- Each visible 24-LED paddle is also the complete hit zone.
- The hit zone has three 8-LED accuracy bands. Deeper hits speed up the ball by
  2, 4, or 6 ms for the current rally.
- After every point, the next serve starts 1 ms faster until its baseline
  reaches 14 ms. Rally boosts can reduce the interval further to 8 ms.
- A perfect hit in the final 8 LEDs produces a 240 ms expanding white and
  player-colour explosion from that end of the strip.
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
