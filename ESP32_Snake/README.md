# ESP32 Pocket Arcade: nine games

## Game controls

- At startup, tilt the joystick to scroll between the nine games (four visible at a time).
  Return the stick to center before the next scroll; press GPIO13 to select.
  Then tilt to choose **Easy** or **Hard**, release GPIO13 and press it again to
  start. GPIO12 returns to the game list. Retrying also opens difficulty selection.
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
| Secondary action button | `GPIO 14` and `GND` | Hold/jump/stand/kick by game; pressed = LOW |

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
