# Controls and games

## Physical button labels

The four game buttons are mounted left-to-right as red, green, blue, yellow.
The four buttons form two symmetric player interfaces:

| Player | Primary | Secondary |
| --- | --- | --- |
| Player 1 | Red / P1-A | Green / P1-B |
| Player 2 | Blue / P2-A | Yellow / P2-B |

The two installed player encoders are decoded but never navigate the selector.
The Player 1 encoder controls Twang and solo Lights Out. Both
encoders control Tennis 1D and Lights Out Duel. Colour Shooter, Pong, Reaction Race, and Colour
Snake Duel retain their button controls. Catch 1D uses red; Colour Gate,
Codebreaker, and Whack 1D use the four color buttons in the source-games
profile. Firefighter 1D uses Player 1's encoder and the blue button. The Player
2 encoder click remains reserved.

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
1P Twang -> 1P Colour Shooter -> 2P Pong -> 2P Tennis -> 2P Reaction Race
-> 2P Colour Snake Duel -> repeat
```

The `sparkfun_promicro16_source_games` profile has its own compile-time
catalog so unused games are removed by link-time optimization:

```text
1P Catch 1D -> 1P Colour Gate -> 1P Codebreaker -> 1P Lights Out ->
2P Lights Out -> 1P Whack 1D -> 1P Firefighter 1D -> repeat
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
| Tennis 1D selected/running | Cyan |
| Reaction Race selected/running | Green |
| Colour Snake Duel selected/running | Violet |
| Catch 1D selected/running | Magenta |
| Colour Gate selected/running | Azure |
| Codebreaker selected/running | Violet |
| Lights Out selected/running | Deep blue |
| Lights Out Duel selected/running | Azure |
| Whack 1D selected/running | Magenta |
| Firefighter 1D selected/running | Orange |

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
| 2 | Tennis 1D | `2P tEn` |
| 2 | Reaction Race | `2P rAC` |
| 2 | Colour Snake Duel | `2P CSn` |
| 1 | Catch 1D | `1P CtC` |
| 1 | Colour Gate | `1P CGt` |
| 1 | Codebreaker | `1P COd` |
| 1 | Lights Out | `1P OFF` |
| 2 | Lights Out Duel | `2P OFF` |
| 1 | Whack 1D | `1P HIt` |
| 1 | Firefighter 1D | `1P FIr` |

During play, the left three digits show Player 1 and the right three show
Player 2. Twang, Colour Shooter, Catch 1D, Colour Gate, Codebreaker, solo
Lights Out, Whack 1D, and Firefighter 1D use the left field and leave the right
field blank. Pong, Tennis, Reaction Race, Colour Snake Duel, and Lights Out
Duel show both scores. Each
field is right-aligned, has no leading
zeroes, and is limited to 999. Lives remain visible on the LED strip.

## Colour Snake Duel

Selector output: violet.

- Player 1 is at LED 0 and uses red / P1-A plus green / P1-B.
- Player 2 is at LED 287 and uses blue / P2-A plus yellow / P2-B.
- Two continuous snakes begin at the center and grow outward simultaneously.
- Every colored segment is one 12-LED logical cell. Player 1 sees only red and
  green segments; Player 2 sees only blue and yellow segments.
- A button launches a three-LED matching-color projectile from that player's
  endpoint. Projectiles travel through physical LEDs rather than game cells.
- A correct projectile dissolves the nearest segment and retracts that half of
  the snake toward the center.
- A wrong projectile queues one 12-LED penalty segment. That penalty slides in
  one physical LED every 25 ms instead of appearing as a block.
- Natural growth also moves exactly one physical LED per step. It starts at
  90 ms per LED and accelerates every four seconds toward 38 ms per LED; the
  colored sections remain 12 LEDs wide and move continuously.
- If a snake reaches a player's endpoint, the opponent wins the round. A
  simultaneous breakthrough is a tie and awards no point.
- Both halves reset after each round. First to five round points wins.
- The display shows Player 1 rounds on the left and Player 2 rounds on the
  right. Neither encoder is used.

## Twang

Selector output: orange.

- The goal is to move the white player from the left start to the green exit.
- Each Player 1 encoder detent moves one 12-LED cell left or right. Turning the
  encoder faster moves through multiple clear cells faster.
- The 12-LED player glides to each logical target one physical LED at a time;
  obstacle collision remains aligned to the safe 12-LED cell grid.
- The blue direction marker shows which way the player faces.
- Red / P1-A sends a blue-white twang up to three cells in that direction.
- A twang destroys the first pulsing red enemy in range, adds one point, and
  stops at lava.
- Green / P1-B jumps over exactly one adjacent red enemy or orange lava cell,
  but only when the landing cell is free. It cannot be used for faster travel.
- Red enemies block normal movement. They can be attacked or jumped.
- Orange flickering cells are lava and every generated lava cell has a safe
  landing cell behind it. Trying to walk into lava removes one of three lives;
  the lava remains and must be jumped.
- The green cell at the far end is the dungeon exit. Reaching it produces a
  green strip sweep and generates a denser next level while preserving score.
- Three lost lives produce a red-white game-over explosion at strip center.
- Either Player 1 button or Player 1 encoder movement restarts after game over.

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
- Comets and projectiles advance by physical LEDs; 12 LEDs define their width,
  not their movement step.
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
- Ball position advances one physical LED per movement step.
- Each visible paddle is also the complete hit zone. Both start at 24 LEDs and
  shrink by one physical LED after every point, down to a 12-LED minimum.
- The current hit zone is divided into three accuracy bands. Deeper hits speed
  up the ball by 2, 4, or 6 ms for the current rally.
- Pressing before the ball reaches the correct player's hit zone awards the
  point to the opponent.
- After every point, the next serve starts 1 ms faster until its baseline
  reaches 14 ms. Rally boosts can reduce the interval further to 8 ms.
- A perfect hit in the final 8 LEDs produces a 240 ms expanding white and
  player-colour explosion from that end of the strip.
- First player to five points wins.
- Game-over output: the winning current-width paddle flashes red on the left
  or blue on the right.
- Any color button restarts after game over.

## Tennis 1D

Selector output: cyan.

- Player 1's encoder moves the red racket inside the left 72-LED court;
  Player 2's encoder moves the blue racket inside the right 72-LED court.
- Each racket is 12 LEDs wide. Its bright three-LED edge faces the center and
  is the only part that returns the ball; the dim remainder shows its body.
- Encoder movement is physical-pixel based. Faster detents produce a faster
  swing without an action button.
- During flight, a dim marker shows the landing position. The cyan halo grows
  toward the trajectory apex and contracts toward the landing point.
- The opponent cannot touch the ball while it is airborne. After landing, the
  ball continues toward that player's outside edge and becomes returnable.
- Swinging the racket edge toward the center at contact determines return
  power. Stationary or wrong-direction contact produces only a soft return.
- Power levels move the next landing point progressively deeper and shorten
  both flight and post-bounce time. A maximum-power ball lands only 14 LEDs
  from the outside edge.
- A player waiting near the center can therefore be overflown: the ball lands
  behind the racket and keeps moving outward. Returning a deep shot strongly
  requires early positioning plus a fast inward swing at the hit edge.
- A missed ball reaching LED 0 or LED 287 awards the opponent one point.
  Serves alternate automatically; first to five points wins.
- During play the display shows Player 1 on the left and Player 2 on the right.
  Encoder movement or either primary button restarts after game over.

## Reaction Race

Selector output: green.

- The center pulses amber during a randomized 1.5- to 4-second start delay.
- Any early player-button press is a false start and awards the round to the other player.
  If both players false-start together, neither scores.
- The two center cells turn green when the race begins.
- Player 1 alternates red and green; Player 2 alternates blue and yellow. A red
  trail advances from the left and a blue trail from the right.
- Every correct button adds a 12-pixel movement impulse, but the visible trail
  and colored tip advance one physical LED at a time. The result animation
  waits until the winning tip has visibly reached the center.
- The moving tips use each player's primary/secondary colors to indicate the
  required next button. Pressing the other button does not advance.
- The first racer to reach the center wins the round. A simultaneous finish is
  a tie and awards no point.
- First to three rounds wins. The winning color alternates with white across
  the full strip.
- Any color game button restarts after game over.

## Catch 1D (source-games profile)

Selector output: magenta.

- A white three-LED marker bounces between the two ends of the strip.
- The dim green target starts 36 LEDs wide at strip center.
- Press red / P1-A while the marker is inside the target.
- A correct catch produces an expanding green effect, adds one point, makes
  subsequent movement faster down to a six-millisecond pixel step, and shrinks
  the target by two physical LEDs down to a 12-LED minimum.
- Pressing outside the target ends the run and flashes the target red.
- The display shows the catch count in the left score field.
- Any color button restarts after game over.

## Colour Gate (source-games profile)

Selector output: azure.

- A three-LED cue travels from the far end toward a dim white 24-LED gate.
- The cue is randomly red, green, blue, or yellow.
- Press the matching color button while the cue is inside the gate.
- Correct timing and color add one point and flash the gate green.
- A wrong color, early press, late press, or missed cue costs one of three
  green life indicators and flashes the gate red.
- Every three successful cues shorten the physical-pixel movement interval by
  one millisecond, from 10 ms down to 5 ms.
- After three mistakes the gate flashes red; any color button restarts.
- The display shows the score in the left field.

After eight correct gates, a green sweep starts the Boss Deflect second stage:

- A colored three-LED boss attack travels from the red boss toward the dim
  white defense gate.
- Press the matching red, green, blue, or yellow button while the attack is
  inside the gate.
- A correct defense turns the attack white and reflects it into the boss.
- Wrong timing, wrong color, or a missed attack costs one of three green lives.
- The first boss needs four reflected hits. Each cleared level adds one boss
  hit point, accelerates incoming attacks, and restores one lost life.
- The boss health is represented by the bright portion of its 24-LED body.
- Defeating a boss produces a green strip sweep. Three mistakes produce a
  flashing red game-over gate; any color button restarts.
- The display continues the stage-one score and adds total reflected hits.

## Codebreaker (source-games profile)

Selector output: violet.

- The center of the strip shows four empty white slots. Enter a four-color code
  with the red, green, blue, and yellow buttons; repeated colors are allowed.
- After four presses, four feedback blocks appear to the left: green means the
  right color in the right position, orange means a right color in a different
  position, and dim red means that color did not contribute to a match.
- The completed guess colors and feedback remain visible until the next color
  press. That press also becomes the first color of the next guess.
- The eight small green LEDs at the strip start show remaining attempts.
- Solve the secret within eight attempts to earn one point and receive a new
  code. A green sweep confirms the solution.
- If all attempts are used, the center slots reveal the secret. Press any color
  button to restart. The display counts solved codes.

## Lights Out (source-games profile)

Selector output: deep blue.

- The strip is divided into 24 cells. Blue cells are on; dark cells are off.
- Player 1's encoder moves the white cursor, red toggles the selected cell and
  its immediate neighbors, and green restores the puzzle's original pattern.
- The cursor wraps at either end.
- Turn every cell off to solve the puzzle. A green sweep confirms success and
  starts a more heavily scrambled, guaranteed-solvable puzzle.
- The display counts solved puzzles.

The following selector entry, `2P OFF`, is the competitive version:

- The strip is split into two identical, independently playable 12-cell
  puzzles. Player 1 owns the left half and Player 2 owns the right half.
- Player 1 uses their encoder, red to toggle, and green to reset. Their cursor
  is red.
- Player 2 uses their encoder, blue to toggle, and yellow to reset. Their cursor
  is yellow.
- The first player to turn off every cell on their half scores the round. If
  both finish in the same update, the orange tie round awards no point.
- Both display fields show match score. First to three wins; any color button
  starts a new match after the winner animation.

## Whack 1D (source-games profile)

Selector output: magenta.

- The strip is divided into four large dim red, green, blue, and yellow zones.
- A target becomes bright and gets a white center. Its bright colored width
  contracts continuously toward the center to show the remaining response
  time. Press that zone's matching color button before it closes.
- A correct press clears the target and adds one point. A wrong button or a
  timeout costs one of three lives and flashes the strip red.
- Waves begin with one target. Two targets appear from score 8, and three from
  score 24; clear every lit target to complete the wave.
- The deadline falls from 1100 ms toward 420 ms as the score rises.
- The left display field shows score. After all three lives are lost, any color
  button starts a new run.

## Firefighter 1D (source-games profile)

Selector output: orange.

- A small cyan marker with a white center is the firefighter. Turn Player 1's
  encoder to move it along the strip. The firefighter may safely cross a fire;
  walking on it does not extinguish it.
- Fires flicker yellow, orange, and red while spreading outward one physical
  LED at a time. Their width is the visible burn timer.
- A dim-blue area with bright blue endpoints constantly shows the hose range.
  Put a fire's center inside that area and tap the blue button once. A visible
  water jet and cyan splash extinguish the nearest fire in range and add one
  point; spraying empty space has no penalty and water is unlimited.
- One fire is active initially, two from score 5, and three from score 15.
  Burn time falls from 6 seconds toward 3 seconds as score rises.
- A fire that fully spreads costs one of three lives and clears the field for
  the next attempt. The left display field shows extinguished-fire score.
- After all three lives are lost, turn Player 1's encoder or press blue to
  restart.

## Hanoi 1D (inactive prototype)

Retained in source after proving insufficiently intuitive in play testing; not
selectable.

- The strip is divided into red/left, green/middle, and blue/right peg zones.
- Red, green, and blue select a source peg and then a destination peg. Yellow
  resets the tower.
- Only the top disk can move, and a larger disk cannot be placed on a smaller
  one. The goal is to move the complete tower from red to blue.

## Minefield 1D (inactive prototype)

Retained in source after a medium play-test rating; not selectable.

- The strip is divided into 24 initially dim-white hidden cells. Player 1's
  encoder moves the white cursor.
- Red reveals the selected cell. The first reveal is always made safe.
- Green places or removes a yellow flag on an unrevealed cell.
- A dim-green revealed cell has no mine next to it; a blue cell has exactly one
  neighboring mine; an orange cell has mines on both immediate sides.
- Pressing red again on a revealed clue opens its other neighboring cells when
  the clue's required number of adjacent flags has been placed.
- Revealing a cell with no adjacent mine automatically opens the connected
  safe run and its numbered boundary cells.
- Reveal every safe cell to clear the board. Flags are aids and do not need to
  be placed to finish.
- Revealing a mine exposes all mines in red. Move the encoder or press red or
  green to restart.
- Cleared-board count appears on the display. Mine count starts at four and
  increases every two cleared boards, up to eight.

## Nim Duel (inactive prototype)

Retained in source after a medium play-test rating; not selectable.

- Twenty-one visible white stones form a shared pile. There is no random or
  hidden state.
- The colored stone group shows how many stones the active player is preparing
  to take. A red pixel at the left end indicates Player 1's turn; a blue pixel
  at the right end indicates Player 2's turn.
- On Player 1's turn, their encoder selects one, two, or three stones and red
  confirms the move. On Player 2's turn, their encoder selects and blue
  confirms.
- The player taking the final stone wins the round. The winning half flashes
  in that player's color and their display score increases.
- The starting player alternates every round; play continues until the players
  leave through the selector.

## Snake 1D (inactive source)

Snake 1D remains in the source tree but is excluded from both compiled game
profiles.

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
in either compiled game profile.

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

Memory Sequence remains in the source tree but is excluded from both compiled
game profiles.

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
