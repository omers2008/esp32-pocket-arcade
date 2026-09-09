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
```

- Tetris: rotations, seven-bag distribution, hold restrictions, row clearing,
  wall kicks, soft drop, difficulty, top-out, and 20,000 randomized input steps.
- Castle: jump buffering, no double jump, pit/platform reachability, attacks,
  healing, damage immunity, respawn, boss/exit progression, victory, and
  40,000 randomized input/draw steps.

- Pong: CPU speed/reaction/accuracy, prediction threshold, serve buffering,
  unchanged ball speed caps, scoring, restart, and 40,000 input/draw steps.

- Duck Hunt: aim movement/bounds, hit detection, ammo/scoring, shot events,
  result transitions, timeouts, lives, target bounces, rounds, restart, timer
  rollover, and 40,000 input/draw steps.

- Pac-Man: maze connectivity, pellet collection, power mode, queued turns,
  movement bounds, ghost movement/respawn, collisions, lives, rounds, timer
  rollover, and 40,000 input/draw steps.

These are logic tests, not pixel-level rendering tests or hardware tests.
