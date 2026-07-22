# Controls and games

## Physical button labels

The four game buttons are mounted left-to-right as red, green, blue, yellow.
The four buttons form two symmetric player interfaces:

| Player | Primary | Secondary |
| --- | --- | --- |
| Player 1 | Red / P1-A | Green / P1-B |
| Player 2 | Blue / P2-A | Yellow / P2-B |

The two installed player encoders are decoded but never navigate the selector.
No active game currently consumes encoder movement. Twang is the next planned
encoder integration; Colour Shooter, Pong, and Reaction Race retain their
button controls. Both encoders and the Player 2 encoder click remain reserved.

## Gameplay UI rule

Use an encoder for active directional control. Turning it faster must produce
faster movement without a separate speed button. A game should ideally require
only one action button and may use at most two. Prefer a simple, immediately
readable control scheme over additional mechanics.

## Selector and power-mode outputs

At the game selector, short-press the illuminated selector to advance one game.
Hold it for about 0.8 seconds to start the displayed game. The player encoders
never change menu state.

The selector order is grouped by player count:

```text
1P Twang -> 1P Colour Shooter -> 2P Pong -> 2P Reaction Race -> repeat
```

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
| Twang selected/running | Orange |
| Colour Shooter selected/running | Yellow |
| 1D Pong selected/running | Blue |
| Reaction Race selected/running | Green |

After a one-second power confirmation, the selector output returns to the
selected game's color. Red is the PSU confirmation; green can be the brief
bench confirmation or the Reaction Race selection.

## Starting a game

Short-press the illuminated selector until it shows the desired game color,
then hold it for about 0.8 seconds.

The selected game starts immediately and the selector output remains its game
color.

Press the illuminated selector or the setup button during any running game to
return to game selection. The selector output then shows the selected game's
color.

## Six-digit display

The selection display identifies player count and game:

| Players | Game | Selection display |
| ---: | --- | --- |
| 1 | Twang | `1P tNG` |
| 1 | Colour Shooter | `1P CSH` |
| 2 | 1D Pong | `2P PnG` |
| 2 | Reaction Race | `2P rAC` |

During play, the left three digits show Player 1 and the right three show
Player 2. Twang uses its level as the Player 1 value; Colour Shooter uses score
and leaves the Player 2 field blank. Pong and Reaction Race show
both scores. Each field is right-aligned, has no leading zeroes, and is limited
to 999. Lives remain visible on the LED strip.

## Twang

Selector output: orange.

- Red moves the white player left by one 12-LED cell; green moves right.
- The blue direction marker shows which way the player faces.
- Blue sends a blue-white twang up to three cells in that direction.
- A twang destroys the first pulsing red enemy in range but stops at lava.
- Yellow dashes two cells in the facing direction, with a 600 ms
  recharge. A dash can cross one dangerous cell but must land safely.
- Orange flickering cells are lava. Landing on lava removes one of three green
  life indicators and clears that lava cell.
- The green cell at the far end is the dungeon exit. Reaching it produces a
  green strip sweep and generates a denser next level.
- Three lost lives produce a red-white game-over explosion at strip center.
- Any color button restarts after game over.

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

- This is currently the 1P Advanced variant with all four colors.
- Random colored comets move from the far end toward the player.
- Red, green, blue, and yellow each launch a projectile of the matching color.
- The nearest comet blocks the projectile.
- A correct match dissolves the target and increases score and speed.
- A wrong projectile disappears while the target remains.
- A comet reaching the player removes one of three green life indicators.
- Three lost lives produce a flashing red game-over output near the strip
  center.
- Any color button restarts after game over.

A later two-player Colour Shooter variant will assign only red/green to Player
1 and blue/yellow to Player 2. That variant is planned, not yet selectable.

## 1D Pong

Selector output: blue.

- Player 1 hits with red / P1-A.
- Player 2 hits with blue / P2-A.
- Green / P1-B and yellow / P2-B are reserved for later specials.
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
- Any color button restarts after game over.

## Reaction Race

Selector output: green.

- The center pulses amber during a randomized 1.5- to 4-second start delay.
- Any early player-button press is a false start and awards the round to the other player.
  If both players false-start together, neither scores.
- The two center cells turn green when the race begins.
- Player 1 alternates red and green; Player 2 alternates blue and yellow. A red
  trail advances from the left and a blue trail from the right.
- The moving tips use each player's primary/secondary colors to indicate the
  required next button. Pressing the other button does not advance.
- The first racer to reach the center wins the round. A simultaneous finish is
  a tie and awards no point.
- First to three rounds wins. The winning color alternates with white across
  the full strip.
- Any color game button restarts after game over.

## Snake 1D (inactive source)

Snake 1D is retained in the repository but is not selectable in the focused
four-game firmware.

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

## Meteor Dodge (inactive source)

Meteor Dodge is retained as an encoder-tested prototype but is not selectable
in the focused four-game firmware.

- The white player begins at strip center. Each Player 1 encoder detent moves
  one 12-LED cell left or right.
- Turning the encoder faster moves the player faster; there is no dash.
- An orange pulsing cell warns where the next meteor will strike.
- The impact expands across five cells as a rapid red-white blast. Being
  within two cells of its center costs one of three green lives.
- Red / P1-A spends one of three cyan shield indicators. The active
  shield colors the player cyan and absorbs the next otherwise damaging hit.
- Each clean dodge increases score. Warning time falls from 1.2 seconds toward
  465 ms as score rises; the 360 ms impact animation remains constant.
- Three hits produce a flashing red game-over blast at strip center.
- Red / P1-A or Player 1 encoder movement restarts after game over. Green is
  currently unused.

## Memory Sequence (inactive source)

Memory Sequence is retained in the repository but is not selectable in the
focused four-game firmware.

- Four dim 12-LED stations represent red, green, blue, and yellow at fixed
  positions along the strip.
- During playback, the required station becomes fully bright for 400 ms,
  followed by a 180 ms dark gap.
- Repeat the sequence using the matching red, green, blue, and yellow game
  buttons. Correct input briefly brightens its station.
- Completing a round produces a green sweep and adds one new color.
- The sequence grows from one to a maximum of 32 colors without allocating a
  sequence array; it is regenerated from a fixed seed.
- A wrong color or 3.5 seconds without input produces a flashing red
  game-over block at strip center.
- Completing all 32 colors produces a full-strip four-color victory output.
- Any color game button restarts after game over or victory.
