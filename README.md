# ESP32 Pocket Arcade

A twenty-two-game handheld arcade for an ESP32, a 128x64 SSD1306 I2C OLED,
an analog joystick, and three buttons. Includes Easy/Hard selection and
separate high scores stored in flash for each game and difficulty.

| Game | Joystick | GPIO13 | GPIO14 |
| --- | --- | --- | --- |
| Snake | Steer | Select / retry | Unused |
| Space Invaders | Move left/right | Fire (hold to repeat) | Unused |
| Pong | Paddle up/down | Serve | Unused |
| Tetris | Move; down = soft drop | Rotate clockwise | Hold / swap |
| Castlevania-style adventure | Move left/right | Whip attack | Jump |
| Duck Hunt-style targets | Aim crosshair in any direction | Shoot (one shot per press) | Unused |
| Pac-Man-style maze | Steer; turns can be queued | Select / retry | Unused |
| Blackjack | Unused during play | Hit / next hand | Stand |
| Street Fighter-style brawler | Move left/right; up = jump | Punch | Kick |
| Rogue Cards | Scroll cards / rewards | Play card / choose reward | End turn |
| Temple Quest | Left/right move; up/down climb | Throw dagger | Jump |
| Slot Machine | Pull down to spin | Raise wager $100 | Lower wager $100 |
| 4 In A Row | Move column left/right | Drop piece | Unused |
| Tic-Tac-Toe | Move cursor | Place X | Unused |
| Minesweeper | Move cursor | Dig | Flag |
| Pinball | Unused | Left flipper / launch | Right flipper / launch |
| Forest Quest | Move | Punch / sword | Pick or throw rock; bow |
| Battle Tanks | Left/right rotate; up forward, down reverse | Normal shell | Four-bounce shell |
| Dino Runner | Up jump; down duck | Jump | Hold to duck |
| Asteroids | Left/right rotate; up thrust | Hold to shoot | Hyperspace |
| Sky Patrol | Point toward desired heading | Hold to boost | Hold to fire |
| Skull Depths | Move / face sword direction; choose upgrades | Sword / confirm | Dash with invulnerability |

GPIO12 returns to the game menu in every game. Tap the joystick to step through
games, or hold up/down to scroll repeatedly until you release it. Repeating starts
after 400 ms, then advances every 130 ms. Center the stick after returning from a game;
press GPIO13 to select a game, then choose Easy or Hard and press GPIO13 again.
Tetris hold is available once per piece, resetting after the piece locks.
The castle adventure is an original miniature three-stage game, not a port of
the original NES ROM; no original game assets are included.
Duck Hunt is also an original small-screen lookalike, with flying targets,
occasional two/three-bird flocks, three shots per encounter, and a countdown bar.
Hit every bird before they escape; overlapping birds can share a single shot.
Pac-Man uses an original compact maze: eat all pellets, avoid ghosts, and eat
power pellets to turn the tables. Clear a maze to start a faster round.
Blackjack is points-only: play 10 hands against the dealer, with automatically
valued aces and a freshly shuffled 52-card deck each hand. No betting or money.
Street Fighter is an original tiny arena brawler, not a Street Fighter II port:
fight enemy waves using punches, kicks, and jumps with original monochrome sprites.
Rogue Cards is a nine-fight turn-based roguelike: start with five cards and draw
three each turn. Easy uses two Strikes, two Guards, and one Heal; Hard rolls
five cards from that pool once per run, with at least one Strike. Choose one
of three passives after each battle, earn active powers from bosses, and find
  random free-power events. Basic cards cost 1 energy; earned spells cost 2 or 3,
  and Hard spell rewards are random three-of-six selections from the full pool.
  Powers last for the run; only high scores persist.
Temple Quest is an original Montezuma-style six-room exploration platformer:
find keys, collect six gems, dodge pits/spikes/enemies, and reach the final idol.
See the [room map and temple guide](ESP32_Snake/README.md#temple-quest-map).
Slot Machine starts with $10,000. GPIO13 raises the wager, GPIO14 lowers it, and
holding either button repeats the change faster and faster, up to a safe limit;
releasing resets the repeat speed. Pulling the joystick down spins three
animated reels with classic fruit-machine symbols. Hard mode weights the
symbols like a real slot machine, making fruit common and lucky 7 rare. Pairs and triples pay out;
three lucky 7s are the jackpot. Cash may reach zero, but the game ends only after
a spin leaves the bankroll negative.
4 In A Row is a seven-column, six-row connect-four-style game: drop filled pieces
with GPIO13 and stop the CPU from making four. Tic-Tac-Toe uses the joystick to
move a 3x3 cursor and GPIO13 to place X; the CPU plays O. Hard mode searches for
wins and blocks, while Easy chooses legal moves randomly.
Minesweeper uses a smaller 8x6 field with 8 mines on Easy and a larger 12x6
field with 16 mines on Hard. GPIO13 digs and GPIO14 flags; the first dig is safe.
Pinball uses GPIO13 for the left flipper and GPIO14 for the right flipper. Press
either button to launch a new ball, then hold the buttons to save it with the
flippers. Bumpers and flipper hits add points; three balls are available on Easy
and two on Hard.
Forest Quest is an original top-down RPG. Start with a punch and no inventory;
GPIO14 picks one rock and throws it, while GPIO13 attacks. Defeat the marked
reward fights to unlock the sword and then the bow. The bow starts with five
arrows, can recover arrows from the ground, and crafts five arrows when you have
none and stand on a rock.

Battle Tanks pits your tank against CPU tanks across five arena waves. The barrel
follows the hull: left/right rotates in place, up drives forward, and down reverses.
Hold GPIO13 for normal shells or GPIO14 for ricochet shells (GPIO14 wins if both
are held). Ammo is unlimited with a firing cooldown. Normal shells stop at walls
and cover; ricochet shells reflect four times and disappear on the fifth impact.
Any tank hit consumes the shell. Your returning ricochets can damage you.
CPU tanks steer, aim, and fire normal shells when they have a clear shot.
Each CPU takes two hits; later waves add a third CPU. Easy starts with five health,
Hard with three and faster CPUs. Clearing a wave restores one health and earns
200 points; each defeated tank earns 100. Clearing wave five wins the run.
Scores save separately for Easy and Hard. GPIO12 returns to the arcade menu.

Dino Runner is an original OLED adaptation of the offline dinosaur runner.
Run automatically, jump over cacti and low birds with GPIO13 or joystick-up,
and hold GPIO14 or joystick-down to duck under higher birds. Release up before
jumping again; there are no double jumps. Cactus singles, tall cacti, and groups
appear first, with birds joining after 150 points. Running earns points and
gradually increases speed to a cap. Hard starts faster and has tighter obstacle
spacing. A collision ends the run; GPIO13 retries through difficulty selection,
and GPIO12 returns to the arcade. High scores persist separately for Easy/Hard.

Asteroids is an original monochrome space shooter. Rotate with left/right, thrust
with up, and coast when you release the stick. The ship, rocks, and shots wrap at
all screen edges. Hold GPIO13 to shoot: large asteroids split into two medium ones,
then into small fragments. Hits award 20/50/100 points for large/medium/small rocks;
clearing a wave adds 200 and starts the next, with increasing rock counts and speed.
GPIO14 teleports to a clearer location, cancels drift, and grants a brief shield;
the HUD's `H` countdown shows seconds until hyperspace is ready again (four seconds).
Easy starts with three ships; Hard has two and more, faster rocks. Respawns provide
two seconds of protection. Scores save separately for each difficulty; GPIO12 exits.

Sky Patrol is a side-scrolling air-combat game over the sea. Point the joystick
in any direction and the plane banks toward that heading; release to keep flying
that way. Hold GPIO13 to boost and GPIO14 to fire along the nose; both work together.
The camera follows horizontal flight, including leftward flight and reversals.
Shoot down enemy aircraft for 100 points each. Every five kills increases the wave
and ramps up enemy speed/spawning, up to a cap. Easy has three lives and enemies
that take one hit; Hard has two lives, tougher enemies, and faster enemy fire.
Collisions with aircraft or enemy bullets cost a life. Touching the water also
costs a life, even during the brief respawn shield. GPIO12 exits; high scores save
separately for Easy and Hard.

## Skull Depths

An action roguelike with three areas (Crypt, Ruins, Keep). Each area contains
three battle rooms, a shop, then a boss. Joystick movement sets the sword's facing;
hold GPIO13 to swing and press GPIO14 to dash. A dash grants 0.2 seconds of
invulnerability and recharges one charge every two seconds. Release GPIO14 between
dashes. GPIO12 exits. Easy has eight starting health; Hard has six, more enemies,
faster arrows and shorter enemy windups. Runs start fresh; best scores persist.

Skulls only hurt you when their attack lands, never from touching. Melee enemies
mark their attack area before striking, giving you time to move or dash. Skulls
wearing hats aim, then fire arrows. Bosses alternate marked slams and arrow bursts,
with stronger bursts in later areas. Pillars block movement and projectiles.

Each battle clear offers three randomly selected, distinct passive upgrades. Tilt
the stick to inspect each description, then press GPIO13 to choose. Upgrades stay
for the run and can stack to their stated limits:

| Upgrade | Effect |
|---|---|
| Sentry arrow | An arrow toward the nearest enemy every second; each rank adds 1 damage |
| Extra dash | +1 dash charge, up to three total |
| Shadow dash | Dash grants five seconds of stealth; the first connected sword swing does 200% damage and ends stealth |
| Sharp steel | +1 sword damage per rank |
| Long blade | +2 pixels of sword reach per rank |
| Quick hands | Faster sword swings per rank |
| Vital heart | +2 maximum health and heal 2 |
| Room mend | Heal 1 per rank after clearing a battle |
| Frost dash | Dash slows enemy movement for two seconds |
| Gold hunter | +5 gold per enemy defeated per rank |
| Soul drinker | Heal 1 after every six sword kills |

Stealth prevents enemies from starting new attacks; arrows and attacks already
in progress remain dangerous after dash invulnerability ends. Missing a swing
does not consume the stealth damage bonus. Auto-arrows do not consume it either.
The shop sells healing (20 gold), maximum health (35), and sword damage (40).
Select `Enter boss arena` when ready. Defeat the third boss to win.

## Hardware and setup

- ESP32 Dev Module (classic ESP32/WROOM).
- SSD1306 OLED: SDA = GPIO21, SCL = GPIO22, VCC = 3.3V, GND = GND.
- Joystick: X = GPIO34, Y = GPIO35, power = 3.3V, ground = GND.
- Normally-open buttons from GPIO12, GPIO13, and GPIO14 to GND.
  Internal pull-ups are enabled; do not connect buttons to a supply voltage.
- GPIO12 is a boot-strapping pin: do not add an external pull-up on it.

Disconnect power before changing wiring. See the
[complete wiring, controls, difficulty, and troubleshooting guide](ESP32_Snake/README.md).

1. Install **esp32 by Espressif Systems** in Arduino IDE Boards Manager.
2. Install **Adafruit SSD1306** and **Adafruit GFX Library**, including dependencies.
3. Open [ESP32_Snake/ESP32_Snake.ino](ESP32_Snake/ESP32_Snake.ino).
4. Select **ESP32 Dev Module**, choose the serial port, and upload.
5. Keep the joystick centered during startup calibration.

The sketch retains its original `ESP32_Snake` folder/name for Arduino compatibility.
All twenty-two games are compiled into the same firmware. Tested with ESP32 core 3.3.11.
The Adafruit libraries are external dependencies and are not vendored here.

If Arduino CLI is installed and the core/libraries are already configured:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32_Snake
arduino-cli board list
arduino-cli upload --port COM8 --fqbn esp32:esp32:esp32 ESP32_Snake
```

Replace `COM8` with your board's port. Records save on game over or returning
to the menu; cutting power in the middle of a run does not save that run.

## Tests

[Host-side gameplay tests](tests/README.md) exercise the actual Pong, Tetris, castle, Duck Hunt, Pac-Man, Blackjack, brawler, Rogue Cards, Temple Quest, Slot Machine, 4 In A Row, Tic-Tac-Toe, Pinball, Forest Quest, Battle Tanks, Dino Runner, Asteroids, Sky Patrol, and Skull Depths
headers with lightweight Arduino/display stubs. They cover mechanics and bounds,
but do not replace testing the physical buttons, OLED, and ESP32.

Local build outputs, executables, and credentials are excluded from Git.
