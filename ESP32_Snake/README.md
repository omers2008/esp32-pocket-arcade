# ESP32 Pocket Arcade: twenty-two games

## Game controls

- At startup, tilt the joystick to scroll between the twenty-two games (four visible at a time).
  Tap for one step or hold up/down to repeat after 400 ms, then every 130 ms.
  Release to stop scrolling; press GPIO13 to select. After exiting gameplay,
  center the stick before scrolling again.
  Then tilt to choose **Easy** or **Hard**, release GPIO13 and press it again to
  start. GPIO12 returns to the game list. Retrying also opens difficulty selection.
- Battle Tanks: left/right rotates the tank and barrel; up drives forward and down
  reverses. Hold GPIO13 for normal shells or GPIO14 for four-bounce shells.
  Ricochets bounce off walls and cover four times, expire at impact five, and can
  hit you on their return. Defeat CPU tanks over five waves; two hits destroy each
  CPU. Later waves add a third opponent. Easy gives five health; Hard gives three
  and faster enemies. Wave clears heal one health. GPIO12 exits to the arcade.
- Dino Runner: an automatic dinosaur runner. GPIO13 or joystick-up jumps;
  GPIO14 or joystick-down ducks while held. Jump over cacti and low birds,
  duck under higher birds, and survive as the speed increases. Birds appear
  after 150 points. Hard starts faster with closer obstacles. Collisions end
  the run; distance scores save separately for Easy/Hard. GPIO12 exits.
- Asteroids: left/right rotates, up thrusts, and the ship coasts when released.
  Hold GPIO13 to fire; GPIO14 teleports with a four-second cooldown and a brief
  shield. Ship, shots, and rocks wrap around the playfield. Shooting large rocks
  splits them into medium rocks, then small fragments. Clear waves to progress.
  Easy has three lives, Hard two with faster/more rocks. GPIO12 exits.
- Sky Patrol: point the joystick toward the desired plane heading. The plane
  banks toward it and keeps flying when the stick is released. Hold GPIO13 to
  boost and GPIO14 to shoot (both can be held). Shoot down enemy aircraft and
  avoid the water below. Camera scrolling follows forward/reverse flight.
  Earn 100 points per kill; waves get harder every five kills. Easy has three
  lives and one-hit enemies; Hard has two lives and two-hit enemies. Water
  crashes cost a life even during respawn protection. GPIO12 exits.
- Snake: steer with the joystick.
- Space Invaders: left/right moves your ship; hold GPIO13 to fire repeatedly.
  Release the select button after entering the game, then press it to shoot.
  Destroy the aliens to advance to faster waves. You have three lives; aliens
  reaching the ship's row end the game immediately.
- Pong: you control the left paddle with joystick up/down; the ESP32 controls
  the right paddle. Press GPIO13 to serve at the start and after every point.
  First to 7 points wins. Paddle-edge hits change the bounce angle, and the ball
  speeds up during each rally. Pong's saved record is your most paddle returns
  in a single rally, shown as `Best rally` on the result screen.
- Tetris: left/right moves the falling piece; hold a direction to repeat.
  GPIO13 rotates clockwise, down makes it fall faster, and GPIO14 stores it in the
  side HOLD box. If HOLD is occupied, GPIO14 swaps the active and stored pieces.
  Hold can be used once per turn and resets only after the falling piece locks.
  Release GPIO14 before pressing again. Joystick up is unused in Tetris.
  `USED` marks an unavailable hold; `14` means hold is available.
  Stored pieces return at the top in their original orientation. A dotted ghost
  shows the landing position; NEXT shows the next piece. Full lines clear, and
  stacking to the top ends the game. Soft drops earn one point per cell; clearing
  1/2/3/4 lines earns 100/300/500/800 points times the current level.
- Castlevania: an original small-screen adaptation inspired by the classic, not
  the original NES game or ROM. Joystick left/right moves, GPIO13 whip-attacks
  (hold to repeat), and GPIO14 jumps. Jump requires a fresh press while grounded;
  no double-jump. GPIO12 returns to the menu. Cross pits and one-way platforms,
  defeat skeletons and bats, and break candles with the whip for 50 points and
  one health. Each of three stages ends with a boss; defeat it, then walk through
  the rightmost door. Whips can destroy boss projectiles. Pits cost two health
  and respawn you at your last safe ground position. The third exit wins the run.
  The HUD shows health, score, stage, and difficulty. Scores save separately for
  Easy/Hard; the other games' existing records are unchanged.
- Duck Hunt: move the crosshair with both joystick axes; tilting farther aims faster.
  Press GPIO13 to shoot (release before shooting again). GPIO14 is unused.
  Encounters contain one, two, or three flying ducks sharing three shots and a
  timer shown by the top bar. Easy spawns 1/2/3 birds with 60%/30%/10% chances;
  Hard uses 45%/35%/20%. Each extra bird adds 1.5 seconds to the timer.
  A hit earns 100 points plus 25 for each unused shot (150/125/100 for a first/
  second/third-shot hit), per bird. A shot can hit multiple overlapping birds.
  Surviving birds keep flying after a hit. Clear every bird to finish the encounter;
  running out of shots or time with survivors costs one life total, keeping points
  already earned. After a short HIT/FLEW AWAY message, another encounter starts.
  Every 10 resolved encounters
  advances the round and increases speed, capped after 10 speed increases.
  Lose all lives to end the run. The HUD shows score, round, lives, ammo (`A`),
  and birds remaining/total (`B`).
  This is original monochrome target-shooting code, not the original NES game.
- Pac-Man: steer with the joystick. Movement continues after releasing the stick;
  a requested turn is queued until the next opening, and walls stop movement.
  Eat all pellets to clear the original 21x11 maze and start a faster round.
  Small pellets earn 10 points, large corner power pellets earn 50, and clearing
  the maze earns 500. Power pellets make ghosts flee and become edible (outlined
  sprites); successive ghosts earn 200/400/800/1600 points, capped at 1600.
  The top bar shows remaining power time; ghosts flash as it runs out.
  Eaten ghosts return home after 2.2 seconds. You have three lives, a short READY
  pause, and brief protection after respawning. Eaten pellets stay gone after
  losing a life. GPIO13/14 have no in-game action; GPIO12 returns to the menu.
  This is a small-screen lookalike with original maze/sprites, not the original ROM.
- Blackjack: GPIO13 hits, GPIO14 stands. Aces automatically count as 1 or 11;
  face cards and `T` (ten) count as 10. Card ranks are displayed without suits,
  but each hand uses a freshly shuffled standard 52-card deck. The dealer's
  second card and total stay hidden until you stand or the hand ends.
  Reaching 21 automatically stands; going over 21 busts. Dealer draws are automatic.
  A two-card blackjack beats an ordinary 21; matching totals or two blackjacks
  push (tie). Easy stands on all 17s; Hard hits soft 17 (17 with an ace valued 11).
  Sessions last 10 hands. Wins earn 10 points, natural blackjacks 15, pushes 2,
  and losses 0. After a result, release and press GPIO13 for the next hand;
  after hand 10, GPIO13 shows the session score and saved best. GPIO12 can exit
  and save your current score at any time. Joystick movement has no in-game action.
  No betting, money, splitting, doubling, or insurance. If both buttons are
  pressed together, Stand takes priority. High scores are separate for Easy/Hard.
- Street Fighter: an original miniature arena brawler inspired by classic fighters,
  not a Street Fighter II ROM/port. Joystick left/right moves and faces that way;
  up jumps. Release up before jumping again; no double jumps or automatic hopping.
  GPIO13 punches (1 damage, short reach, quicker recovery); GPIO14 kicks (2 damage,
  longer reach, slower recovery). Use a fresh button press for each attack.
  Attacks also work in the air, and one swing can hit several nearby enemies.
  Simultaneous punch/kick presses choose kick. GPIO12 returns to the menu.
  Enemies approach from both sides and show `!` before attacking; jump or move
  away, or interrupt with a hit. Later waves add fast enemies and tougher brutes.
  Defeats award 100/150/200 points by enemy type. Waves start with 4 enemies and
  grow to 10, with at most 2 on screen in Easy or 3 in Hard. Clearing a wave earns
  250 points and restores one health. Lose all health to end the run. The HUD
  shows score, wave, health, and enemies remaining (including upcoming spawns).
  Saved scores are separate by difficulty; all previous games' records remain.
- Rogue Cards: a turn-based roguelike with nine fights and bosses on fights 3,
  6, and 9. Move the joystick to scroll cards or rewards, return it to center to
  scroll again, and press GPIO13 to play/choose. GPIO14 ends your turn (and wins
  if both buttons are pressed together in combat). GPIO12 exits to the arcade.
  There is no real-time pressure: the enemy acts only after you end your turn.
  You start with five cards. Easy gives two Strikes, two Guards, and one Heal.
  Hard rolls five cards from the Strike/Guard/Heal pool, allowing duplicates,
  and ensures at least one Strike. The rolled deck stays fixed for the whole run.
  Each turn shuffles those five and makes three available to play; the other two are
  unavailable until a later draw. Each basic card costs one of your three energy.
  Earned spell powers have different costs: Fireball, Ward, and Mend cost 2 energy;
  Venom, Leech, and Storm cost 3 energy. Stronger spells use more of the current
  turn's energy, but can still be saved for the right turn.
  Strike deals 5 damage, Guard gives 5 block, and Heal restores 3 HP before upgrades.
  Unused cards, remaining energy, and block reset after the enemy turn.
  The HUD shows `HP`, energy (`E`), block (`B`), fight (`F`), enemy HP, and its
  next `ATK`. The selected card/power's effect appears below the scrolling list.
  Enemy attack rises every three turns, up to 30 damage before block.
  Every victory offers **three distinct passive choices**, including boss wins.
  After taking a boss passive, choose one of three active powers as an extra reward.
  Easy spell rewards follow a simple rotation. Hard spell rewards randomly choose
  three distinct spells from the complete six-spell pool, including spell-cache
  events. Actives appear after your hand in the scrolling action list and show
  their energy cost; they can each be used once per fight (`E`/`USED`). They reset
  next fight, not next turn. Taking an owned passive stacks it; taking an owned active
  upgrades it.
  After rewards, there is a 20% chance of a free passive-choice shrine, 20% of a
  free active-choice cache, 15% of a spring healing 7 HP, or 45% of moving straight
  to the next battle. No event follows the final boss. Its passive and active
  rewards are collected before the victory screen. Upgrades reset on a new run;
  only high scores are saved, not unfinished runs.
- Temple Quest: an original six-room temple platformer inspired by Montezuma's
  Revenge, not a port or recreation of its original assets. Move left/right with
  the joystick; up/down climbs ladders. GPIO14 jumps, including off a ladder;
  GPIO13 throws a dagger in the direction you face. Each button needs a fresh press.
  GPIO12 returns to the menu. The introduction waits for GPIO13 before play.
  Collect a gem in each room, find both keys to open routes, then reach the idol
  at the right side of room 6. Walk through side doors at ground level; climb the
  long shafts to change floors. Ladders are needed to reach raised treasure ledges.
  Pits and spikes cost a life; enemy contact costs a life unless you are briefly
  protected after entering a room or respawning. Jump over hazards or use your
  limited daggers against skulls and bats. Six starting daggers, +1 per gem,
  maximum nine carried and two in flight. A blocked shot slot does not spend ammo.
  Death returns you to the room entrance checkpoint, preserving collected gems,
  keys, open doors, and defeated enemies. A new run resets all of these.
  The HUD shows score, room (`R`), lives (`L`), gems (`G` out of six), and daggers
  (`D`). Gems score 100, keys 50, skulls 50, bats 75, and the idol awards
  500 plus 100 per remaining life. Only high scores persist after power-off.
- Slot Machine: start with `$10,000`. GPIO13 raises the wager by `$100` and GPIO14
  lowers it by `$100` (from `$100` to `$10,000`); holding either button repeats the
  change, accelerating while held and resetting to the default rate when released.
  Pull the joystick down to pull
  the lever and spin the three animated reels. A pair pays 2x; triples pay more
  based on the symbol, and three 7s are the jackpot at 50x. The lever, spinning,
  hit, and jackpot states are animated on the OLED. Cash can reach zero; only a
  spin that leaves cash below zero ends the run. GPIO12 returns to the menu.
- 4 In A Row: move the column selector left/right and press GPIO13 to drop a filled
  piece. The CPU drops an outline piece after every turn. Make four connected pieces
  horizontally, vertically, or diagonally before the CPU does. Easy picks legal
  columns randomly; Hard takes winning moves, blocks you, and prefers the center.
- Tic-Tac-Toe: move the 3x3 cursor with the joystick and press GPIO13 to place X.
  The CPU places O after each move. Easy chooses an open square randomly; Hard wins
  when possible, blocks your wins, then prefers the center and corners. Three in a
  row wins; a full board is a draw. GPIO14 is unused in both games.
- Minesweeper: move the cursor with the joystick. GPIO13 digs a square and GPIO14
  flags or unflags it. The first dig is always safe; empty squares flood-clear their
  neighbors. Easy uses an 8x6 field with 8 mines, while Hard uses 12x6 with 16 mines.
  Clear every non-mine square to win; digging a mine ends the run.
- Pinball: GPIO13 controls the left flipper and GPIO14 controls the right flipper.
  Press either button to launch a ball at the start of a ball; hold the buttons to
  raise the flippers and save the ball. Bumpers score 25, flipper hits score 10,
  and the ball drain costs one life. Easy gives three balls and a slower ball;
  Hard gives two balls and a faster ball.
- Press GPIO12 to return to the game menu immediately, during play or after
  death. Press GPIO13 on the death screen to retry. The joystick's built-in
  button is unused; stick movement never selects a menu item, fires, or exits a game.
- Each game and difficulty saves its own best score after death or when returning
  to the menu. All previous records are preserved under Easy. Cutting power mid-game does not save
  that unfinished run.

## Difficulty settings

| Game | Easy | Hard |
|---|---|---|
| Snake | 200 ms starting step, speeds up to 90 ms | 125 ms starting step, speeds up to 50 ms |
| Space Invaders | 3 lives, slower aliens and enemy shots | 2 lives, faster aliens, more frequent and faster enemy shots |
| Pong | More accurate tracking, 1.65 px/frame CPU speed, slower ball | Predictive CPU at 2.0 px/frame with more aiming error; faster ball and steeper paddle-angle shots |
| Tetris | 650 ms starting gravity, 450 ms lock delay | 360 ms starting gravity, 250 ms lock delay |
| Castlevania | 6 health, slower enemies, one-hit skeletons, slower boss shots | 4 health, faster enemies, two-hit skeletons, tougher bosses and faster firing |
| Duck Hunt | 5 lives, 6.5 seconds base + 1.5 per extra bird, slower flight, forgiving aim | 3 lives, 4.5 seconds base + 1.5 per extra bird, more flocks, faster flight, tighter aim |
| Pac-Man | 2 ghosts, slower/more random chasing, 6 seconds of power | 3 ghosts, faster/more direct chasing, 3.5 seconds of power |
| Blackjack | Dealer stands on soft 17 | Dealer hits soft 17 |
| Street Fighter | 8 health, 2 simultaneous enemies, slower enemies and longer attack warnings | 6 health, 3 simultaneous enemies, tougher/faster enemies and shorter warnings |
| Rogue Cards | Fixed five-card deck, 32 starting HP, heal 2 after wins | Random five-card deck, 26 starting HP, enemies have +4 HP and +1 attack, no base post-battle heal |
| Temple Quest | 5 lives, slower enemies, longer respawn protection | 3 lives, faster enemies, shorter protection |
| Slot Machine | Even symbol odds; standard wager and payouts | Realistic weighted symbols; lucky 7 is rare |
| 4 In A Row | Random legal CPU columns | CPU wins/blocks and prefers center |
| Tic-Tac-Toe | Random legal CPU squares | CPU wins/blocks and prefers center/corners |
| Minesweeper | 8x6 field, 8 mines | 12x6 field, 16 mines |
| Pinball | 3 balls; slower launch and gravity | 2 balls; faster launch and gravity |

### Rogue Cards powers

| Passive | Effect per copy, for this run |
|---|---|
| Might | +1 damage to attacks |
| Iron guard | +2 block from Guard and Ward |
| Big heart | +5 max HP and immediately heal 5 |
| Herbs | +1 healing from Heal, Mend, and Leech |
| Thorns | Deal 2 damage whenever the enemy attacks, even if blocked |
| Campfire | Heal 3 after every victory |

| Active | First-copy effect; energy cost; once per fight |
|---|---|
| Fireball | Deal 12 damage; 2 energy |
| Ward | Gain 12 block; 2 energy |
| Mend | Heal 8 HP; 2 energy |
| Venom | Deal 4 poison damage at the start of every enemy turn this fight; 3 energy |
| Leech | Deal 8 damage and heal 4 HP; 3 energy |
| Storm | Deal 6 damage and gain 6 block; 3 energy |

Each additional active copy adds 2 to its main value (both damage and block for
Storm; Leech's healing stays at 4 before Herbs). Might/Herbs/Iron guard also apply
where described above. Poison kills prevent the enemy attack; simultaneous
player/enemy deaths from Thorns count as a loss. Each win scores `100 + 25 * fight`,
with +200 for a boss and +500 for clearing the entire run.

Tetris gravity speeds up every 10 cleared lines. Both difficulties use all seven
tetrominoes in shuffled groups of seven and have separate saved high scores.

Pong CPUs update their aim every 100 ms. Easy has +/-2 px aiming error; Hard has
+/-4 px error and starts predicting only after the ball passes x=54. Ball speeds,
paddle sizes, controls, and saved records are unchanged by this CPU rebalance.
Pong's Hard CPU has a reaction delay and limited paddle speed, so it is designed
to remain beatable. Both modes still play first to 7. The current difficulty is
shown during play and on the result screen.

Keep `ESP32_Snake.ino` and all its `.h` files together in the
`ESP32_Snake` folder.

## Temple Quest map

Room numbers match the OLED's `R` counter. Letters mark key-locked routes.

```text
[1 START] --A-- [2] ----- [3]
    |                     | B
   [4] ------- [5] --B-- [6 IDOL]
```

- Key A is on the floor at the right of room 4. It opens the route from 1 to 2.
- Key B is on the raised ledge at the right of room 2. It opens both entrances
  to room 6; each key is consumed once and its routes stay open.
- There is one gem on a raised ledge in every room. Visit room 5 via room 4
  before heading to the idol, or backtrack to collect any missed gems.
- The ladder at x=28 connects rooms 1 and 4; the ladder at x=96 connects rooms
  3 and 6. Press down at the upper room's floor hatch, or climb up to the lower
  room's ceiling. Move sideways or jump to leave a ladder at a treasure ledge.
- Jump from a ledge before its edge when crossing a pit beneath it. Spikes and
  pits remain dangerous during the blinking enemy-protection period.

## Skull Depths

Move and face your sword with the joystick. Hold GPIO13 to attack; tap GPIO14 to
dash with 0.2 seconds of invulnerability. Dash charges refill every two seconds.
Skull enemies wind up before attacking; touching them does no damage. Hat-wearing
skulls are archers. Clear three rooms, buy supplies in the shop, then defeat the
area boss. Repeat through Crypt, Ruins, and Keep to win.

After each battle, choose one of three passive upgrades with the joystick and
GPIO13. These include auto-arrows, extra dashes, stealth with double damage on the
first sword hit, sword strength/reach/speed, health, room healing, frost dashes,
gold bonuses, and healing from sword kills. Read the full
[upgrade list](../README.md#skull-depths) for stacking and stealth details.
Easy starts with eight health; Hard starts with six and adds enemies, faster
arrows and shorter windups. Scores are saved separately by difficulty.

## Wiring

Disconnect USB power while making these connections.

| Module pin | ESP32 pin | Purpose |
|---|---:|---|
| OLED `GND` | `GND` | Ground |
| OLED `VCC` | `3.3V` | Power |
| OLED `SDA` | `GPIO 21` | I2C data |
| OLED `SCL` | `GPIO 22` | I2C clock |
| Joystick `G` | `GND` | Ground |
| Joystick `V` | `3.3V` | Power; do not use 5V |
| Joystick `X` | `GPIO 34` | Horizontal analog input |
| Joystick `Y` | `GPIO 35` | Vertical analog input |
| Joystick `B` | Not needed | Built-in button is unused; existing GPIO27 wire may remain |
| Menu button | `GPIO 12` and `GND` | Normally-open switch, pressed = LOW |
| Select/action button | `GPIO 13` and `GND` | Select/fire/rotate/hit/punch by game; pressed = LOW |
| Secondary action button | `GPIO 14` and `GND` | Hold/jump/stand/kick/end turn by game; pressed = LOW |

The sketch enables internal pull-ups after startup: each new button connects
its GPIO to GND when pressed. Do not connect the buttons to 5V or 3.3V.
GPIO12 is a boot-strapping pin, so do not add an external pull-up on it or use a
button module that drives it HIGH during reset. See
[Espressif's GPIO12 guidance](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/sd_pullup_requirements.html).

The joystick board in the photo labels its five pins `Y`, `X`, `B`, `V`, and
`G`. Follow those printed letters rather than relying on connector order.

## Arduino IDE setup

1. Install the **esp32 by Espressif Systems** board package in Boards Manager.
2. In Library Manager, install **Adafruit SSD1306** and **Adafruit GFX Library**.
   Allow the IDE to install any dependencies it offers.
3. Open `ESP32_Snake.ino` and select **ESP32 Dev Module** (or the exact ESP32
   board entry if you know it).
4. Select the board's COM port and upload.
5. Leave the joystick untouched and centered while the startup calibration text
   is on screen. Select a game and then a difficulty with GPIO13.

The game-over screen shows the current score and the all-time best score. The
ESP32 saves a new best score in non-volatile flash, so it remains after USB power
is disconnected or the board is reset.

## Quick troubleshooting

- Upload fails with Windows `Access is denied`: on this PC, SignalRGB's
  `OEM_Devices_Controller.js` plugin matches the ESP32 adapter's USB ID
  `1A86:7523` and can claim its serial port as an LED controller. Exiting both
  SignalRGB and its relaunch helper released COM8 and allowed a verified upload
  on 2026-09-09. Keep SignalRGB closed while uploading, or exclude this adapter
  in SignalRGB. No startup setting was changed during troubleshooting.

- Blank OLED: change `OLED_ADDRESS` near the top of the sketch from `0x3C` to
  `0x3D`, then upload again. The solder-jumper markings on the back of this OLED
  show that those are its two address choices.
- Up/down or left/right is reversed: change `INVERT_Y` or `INVERT_X` between
  `true` and `false` near the top of the sketch.
- Snake turns by itself: increase `JOYSTICK_DEAD_ZONE` from `650` to about `800`,
  and make sure the stick is centered during boot.
- Upload fails: hold the ESP32's **BOOT** button while the IDE says
  `Connecting...`, then release it when writing begins.
