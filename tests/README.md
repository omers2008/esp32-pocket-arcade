# Gameplay regression tests

These tests compile the actual game headers, using `Arduino.h` for a controlled
clock/random generator and `Adafruit_SSD1306.h` for no-op drawing calls.
Keep assertions enabled (do not define `NDEBUG`). No external libraries needed.

From the repository root, with a C++17 compiler:

```sh
mkdir -p build
g++ -std=c++17 -Itests tests/test.cpp -o build/tetris_test
./build/tetris_test
g++ -std=c++17 -Itests tests/castle_test.cpp -o build/castle_test
./build/castle_test
g++ -std=c++17 -Itests tests/pong_test.cpp -o build/pong_test
./build/pong_test
g++ -std=c++17 -Itests tests/duckhunt_test.cpp -o build/duckhunt_test
./build/duckhunt_test
g++ -std=c++17 -Itests tests/pacman_test.cpp -o build/pacman_test
./build/pacman_test
g++ -std=c++17 -Itests tests/blackjack_test.cpp -o build/blackjack_test
./build/blackjack_test
g++ -std=c++17 -Itests tests/fighter_test.cpp -o build/fighter_test
./build/fighter_test
g++ -std=c++17 -Itests tests/rogue_test.cpp -o build/rogue_test
./build/rogue_test
g++ -std=c++17 -Itests tests/temple_test.cpp -o build/temple_test
./build/temple_test
```

On Windows, open an **x64 Native Tools Command Prompt for Visual Studio**,
change to the repository root, and run:

```bat
if not exist build mkdir build
cl /nologo /EHsc /std:c++17 /Itests tests\test.cpp /Fobuild\tetris_test.obj /Febuild\tetris_test.exe
build\tetris_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\castle_test.cpp /Fobuild\castle_test.obj /Febuild\castle_test.exe
build\castle_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\pong_test.cpp /Fobuild\pong_test.obj /Febuild\pong_test.exe
build\pong_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\duckhunt_test.cpp /Fobuild\duckhunt_test.obj /Febuild\duckhunt_test.exe
build\duckhunt_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\pacman_test.cpp /Fobuild\pacman_test.obj /Febuild\pacman_test.exe
build\pacman_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\blackjack_test.cpp /Fobuild\blackjack_test.obj /Febuild\blackjack_test.exe
build\blackjack_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\fighter_test.cpp /Fobuild\fighter_test.obj /Febuild\fighter_test.exe
build\fighter_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\rogue_test.cpp /Fobuild\rogue_test.obj /Febuild\rogue_test.exe
build\rogue_test.exe
cl /nologo /EHsc /std:c++17 /Itests tests\temple_test.cpp /Fobuild\temple_test.obj /Febuild\temple_test.exe
build\temple_test.exe
```

- Tetris: rotations, seven-bag distribution, hold restrictions, row clearing,
  wall kicks, soft drop, difficulty, top-out, and 20,000 randomized input steps.
- Castle: jump buffering, no double jump, pit/platform reachability, attacks,
  healing, damage immunity, respawn, boss/exit progression, victory, and
  40,000 randomized input/draw steps.

- Pong: CPU speed/reaction/accuracy, prediction threshold, serve buffering,
  unchanged ball speed caps, scoring, restart, and 40,000 input/draw steps.

- Duck Hunt: single/flock spawn frequencies, overlapping multi-hit shots,
  no duplicate scoring, partial-flock completion, aim movement/bounds, ammo, shot events,
  result transitions, timeouts, lives, target bounces, rounds, restart, timer
  rollover, and 40,000 input/draw steps.

- Pac-Man: maze connectivity, pellet collection, power mode, queued turns,
  movement bounds, ghost movement/respawn, collisions, lives, rounds, timer
  rollover, and 40,000 input/draw steps.

- Blackjack: ace valuation, unique deck, natural blackjacks, ties/busts,
  dealer soft-17 rules, button priority, result guards, ten-hand sessions,
  timer rollover, and 40,000 input/draw steps.

- Fighter: short-press queue, jump/rearm, punch/kick ranges and cooldowns,
  group hits, damage/immunity, wave progression, spawning, timer rollover,
  and 60,000 input/draw steps.

- Rogue Cards: turn order, cards/energy, passive stacks, active upgrades and uses,
  poison/thorns, boss reward ordering, free events, campaign win/loss, run reset,
  timer rollover, 40,000 input/draw steps, and 200 complete policy-driven runs.

- Temple Quest: intro/input queue, jumping, ladders and ledges, pit/spike clearance,
  room connections, key gates, gem pickups, dagger ammo/collisions, checkpoint
  persistence, win/loss, and 60,000 input/draw steps. The quest-progression fixture
  positions the player at pickups; movement and hazard clearance are tested separately.

These are logic tests, not pixel-level rendering tests or hardware tests.
