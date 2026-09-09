# ESP32 Pocket Arcade

A seven-game handheld arcade for an ESP32, a 128x64 SSD1306 I2C OLED,
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

GPIO12 returns to the game menu in every game. Scroll with the joystick,
press GPIO13 to select a game, then choose Easy or Hard and press GPIO13 again.
Tetris hold is available once per piece, resetting after the piece locks.
The castle adventure is an original miniature three-stage game, not a port of
the original NES ROM; no original game assets are included.
Duck Hunt is also an original small-screen lookalike, with flying targets,
three shots per target, and a countdown bar. Hit targets before they escape.
Pac-Man uses an original compact maze: eat all pellets, avoid ghosts, and eat
power pellets to turn the tables. Clear a maze to start a faster round.

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
All seven games are compiled into the same firmware. Tested with ESP32 core 3.3.11.
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

[Host-side gameplay tests](tests/README.md) exercise the actual Pong, Tetris, castle, Duck Hunt, and Pac-Man
headers with lightweight Arduino/display stubs. They cover mechanics and bounds,
but do not replace testing the physical buttons, OLED, and ESP32.

Local build outputs, executables, and credentials are excluded from Git.
