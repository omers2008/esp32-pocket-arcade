#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Invaders.h"
#include "Buttons.h"
#include "Pong.h"
#include "Tetris.h"
#include "Castle.h"
#include "DuckHunt.h"
#include "PacMan.h"
#include "Blackjack.h"
#include "StreetFighter.h"
#include "RogueCards.h"
#include "TempleQuest.h"
#include "SlotMachine.h"
#include "FourInRow.h"
#include "TicTacToe.h"
#include "Minesweeper.h"
#include "Pinball.h"
#include "TopdownRPG.h"

// ---------- Hardware ----------
constexpr uint8_t OLED_SDA_PIN = 21;
constexpr uint8_t OLED_SCL_PIN = 22;
constexpr uint8_t OLED_ADDRESS = 0x3C;  // Try 0x3D if the display stays blank.

constexpr uint8_t JOY_X_PIN = 34;
constexpr uint8_t JOY_Y_PIN = 35;
constexpr uint8_t MENU_BUTTON_PIN = 12;
constexpr uint8_t ACTION_BUTTON_PIN = 13;
constexpr uint8_t HOLD_BUTTON_PIN = 14;
constexpr uint32_t SLOT_HARD_SCORE_RESET_VERSION = 1;

// Change either value if that joystick axis moves in the opposite direction.
constexpr bool INVERT_X = false;
constexpr bool INVERT_Y = true;

constexpr int JOYSTICK_DEAD_ZONE = 650;

// ---------- Display and playfield ----------
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int HEADER_HEIGHT = 8;
constexpr int CELL_SIZE = 4;
constexpr int GRID_WIDTH = SCREEN_WIDTH / CELL_SIZE;                  // 32
constexpr int GRID_HEIGHT = (SCREEN_HEIGHT - HEADER_HEIGHT) / CELL_SIZE; // 14
constexpr int MAX_SNAKE_LENGTH = GRID_WIDTH * GRID_HEIGHT;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Preferences preferences;
Invaders invaders;
Pong pong;
Tetris tetris;
CastleGame castle;
DuckHunt duckHunt;
PacMan pacMan;
Blackjack blackjack;
StreetFighter streetFighter;
RogueCards rogueCards;
TempleQuest templeQuest;
SlotMachine slotMachine;
FourInRow fourInRow;
TicTacToe ticTacToe;
Minesweeper minesweeper;
Pinball pinball;
TopdownRPG topdownRPG;
constexpr uint8_t GAME_COUNT = 17;
const char *const GAME_NAMES[GAME_COUNT] = {"Snake", "Space Invaders", "Pong", "Tetris", "Castlevania", "Duck Hunt", "Pac-Man", "Blackjack", "Street Fighter", "Rogue Cards", "Temple Quest", "Slot Machine", "4 In A Row", "Tic-Tac-Toe", "Minesweeper", "Pinball", "Forest Quest"};
const char *const SCORE_NAMESPACES[GAME_COUNT] = {"snake", "invaders", "pong", "tetris", "castle", "duckhunt", "pacman", "blackjack", "fighter", "rogue", "temple", "slots", "fourrow", "tictactoe", "mines", "pinball", "rpg"};
uint8_t selectedGame = 0;  // Game order matches GAME_NAMES and SCORE_NAMESPACES.
uint8_t selectedDifficulty = 0; // 0 = Easy, 1 = Hard
uint32_t bestScores[GAME_COUNT][2] = {}; // Pong records paddle returns per rally.
bool menuStickReady = true;
ArcadeButton menuButton(MENU_BUTTON_PIN);
ArcadeButton actionButton(ACTION_BUTTON_PIN);
ArcadeButton holdButton(HOLD_BUTTON_PIN);
bool holdArmed = false;
bool actionArmed = false;
uint32_t lastArcadeDraw = 0;

enum Direction : uint8_t { UP, DOWN, LEFT, RIGHT };
enum GameState : uint8_t { TITLE, DIFFICULTY, PLAYING, GAME_OVER };

uint8_t snakeX[MAX_SNAKE_LENGTH];
uint8_t snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 0;
uint8_t foodX = 0;
uint8_t foodY = 0;
Direction direction = RIGHT;
Direction nextDirection = RIGHT;
GameState gameState = TITLE;

int joystickCenterX = 2048;
int joystickCenterY = 2048;
uint32_t lastMoveTime = 0;

bool isOpposite(Direction a, Direction b) {
  return (a == UP && b == DOWN) || (a == DOWN && b == UP) ||
         (a == LEFT && b == RIGHT) || (a == RIGHT && b == LEFT);
}

bool snakeOccupies(uint8_t x, uint8_t y, int segmentsToCheck) {
  for (int i = 0; i < segmentsToCheck; ++i) {
    if (snakeX[i] == x && snakeY[i] == y) return true;
  }
  return false;
}

void placeFood() {
  // At most 448 cells exist, so this simple search is fast even late in a game.
  do {
    foodX = random(GRID_WIDTH);
    foodY = random(GRID_HEIGHT);
  } while (snakeOccupies(foodX, foodY, snakeLength));
}

void startGame() {
  holdArmed = false; // Require release before the first hold in a new game.
  actionArmed = false;  // Release the select button before shooting.
  if (selectedGame == 10) {
    templeQuest.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 11) {
    slotMachine.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 12) {
    fourInRow.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 13) {
    ticTacToe.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 14) {
    minesweeper.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 15) {
    pinball.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 16) {
    topdownRPG.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 9) {
    rogueCards.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 8) {
    streetFighter.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 7) {
    blackjack.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 6) {
    pacMan.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 5) {
    duckHunt.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 4) {
    castle.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 3) {
    tetris.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 2) {
    pong.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  if (selectedGame == 1) {
    invaders.start(selectedDifficulty == 1);
    gameState = PLAYING;
    return;
  }
  snakeLength = 3;
  snakeX[0] = 16; snakeY[0] = 7;
  snakeX[1] = 15; snakeY[1] = 7;
  snakeX[2] = 14; snakeY[2] = 7;
  direction = RIGHT;
  nextDirection = RIGHT;
  placeFood();
  lastMoveTime = millis();
  gameState = PLAYING;
}

uint32_t currentScore() {
  if (selectedGame == 0) return uint32_t(snakeLength - 3);
  if (selectedGame == 1) return invaders.score;
  if (selectedGame == 2) return pong.bestRally;
  if (selectedGame == 3) return tetris.score;
  if (selectedGame == 4) return castle.score;
  if (selectedGame == 5) return duckHunt.score;
  if (selectedGame == 6) return pacMan.score;
  if (selectedGame == 7) return blackjack.score;
  if (selectedGame == 8) return streetFighter.score;
  if (selectedGame == 9) return rogueCards.score;
  if (selectedGame == 10) return templeQuest.score;
  if (selectedGame == 11) return slotMachine.score;
  if (selectedGame == 12) return fourInRow.score;
  if (selectedGame == 13) return ticTacToe.score;
  if (selectedGame == 14) return minesweeper.score;
  if (selectedGame == 15) return pinball.score;
  return topdownRPG.score;
}

void finishGame() {
  gameState = GAME_OVER;
  actionArmed = false;  // Held fire must not become an accidental retry.
  uint32_t score = currentScore();
  uint32_t &best = bestScores[selectedGame][selectedDifficulty];
  const char *storage = SCORE_NAMESPACES[selectedGame];

  // Flash is written only when the player sets a new record.
  if (score > best) {
    best = score;
    if (preferences.begin(storage, false)) {
      if (preferences.putUInt(selectedDifficulty == 0 ? "highscore" : "highscore_hard", best) != sizeof(uint32_t)) {
        Serial.println(F("Could not save high score."));
      }
      preferences.end();
    } else {
      Serial.println(F("Could not open score storage."));
    }
  }
}

void calibrateJoystick() {
  // Keep the joystick centered while the ESP32 starts.
  long totalX = 0;
  long totalY = 0;
  constexpr int SAMPLES = 64;
  for (int i = 0; i < SAMPLES; ++i) {
    totalX += analogRead(JOY_X_PIN);
    totalY += analogRead(JOY_Y_PIN);
    delay(3);
  }
  joystickCenterX = totalX / SAMPLES;
  joystickCenterY = totalY / SAMPLES;
}

int joystickX() {
  int value = analogRead(JOY_X_PIN) - joystickCenterX;
  return INVERT_X ? -value : value;
}

int joystickY() {
  int value = analogRead(JOY_Y_PIN) - joystickCenterY;
  return INVERT_Y ? -value : value;
}

void readJoystick() {
  int x = analogRead(JOY_X_PIN) - joystickCenterX;
  int y = analogRead(JOY_Y_PIN) - joystickCenterY;

  if (INVERT_X) x = -x;
  if (INVERT_Y) y = -y;

  Direction wanted = nextDirection;
  bool moved = false;

  // Prefer the axis pushed farther so diagonal readings feel predictable.
  if (abs(x) > abs(y)) {
    if (x > JOYSTICK_DEAD_ZONE) {
      wanted = RIGHT;
      moved = true;
    } else if (x < -JOYSTICK_DEAD_ZONE) {
      wanted = LEFT;
      moved = true;
    }
  } else {
    if (y > JOYSTICK_DEAD_ZONE) {
      wanted = UP;
      moved = true;
    } else if (y < -JOYSTICK_DEAD_ZONE) {
      wanted = DOWN;
      moved = true;
    }
  }

  // A snake cannot reverse directly into itself.
  if (moved && !isOpposite(wanted, direction)) nextDirection = wanted;
}

uint16_t movementInterval() {
  int score = snakeLength - 3;
  int interval = (selectedDifficulty == 0 ? 200 : 125) - score * 5;
  return max(interval, selectedDifficulty == 0 ? 90 : 50);
}

void moveSnake() {
  direction = nextDirection;

  int newX = snakeX[0];
  int newY = snakeY[0];
  if (direction == UP) --newY;
  if (direction == DOWN) ++newY;
  if (direction == LEFT) --newX;
  if (direction == RIGHT) ++newX;

  if (newX < 0 || newX >= GRID_WIDTH || newY < 0 || newY >= GRID_HEIGHT) {
    finishGame();
    return;
  }

  bool grows = (newX == foodX && newY == foodY);
  // Moving onto the old tail is legal when the snake is not growing.
  int collisionSegments = grows ? snakeLength : snakeLength - 1;
  if (snakeOccupies(newX, newY, collisionSegments)) {
    finishGame();
    return;
  }

  if (grows && snakeLength < MAX_SNAKE_LENGTH) {
    for (int i = snakeLength; i > 0; --i) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }
    ++snakeLength;
  } else {
    for (int i = snakeLength - 1; i > 0; --i) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }
  }

  snakeX[0] = newX;
  snakeY[0] = newY;

  if (grows) {
    if (snakeLength == MAX_SNAKE_LENGTH) {
      finishGame();  // The player filled the entire screen.
    } else {
      placeFood();
    }
  }
}

void drawTitle() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(4, 1);
  display.print(F("POCKET ARCADE"));
  display.setCursor(98, 1); display.print(selectedGame + 1); display.print('/'); display.print(GAME_COUNT);
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  int firstVisible = max(0, int(selectedGame) - 3);
  for (int i = firstVisible; i < min(firstVisible + 4, int(GAME_COUNT)); ++i) {
    int y = 13 + (i - firstVisible) * 10;
    if (selectedGame == i) display.fillRect(0, y - 1, 128, 10, SSD1306_WHITE);
    display.setTextColor(selectedGame == i ? SSD1306_BLACK : SSD1306_WHITE);
    display.setCursor(4, y);
    display.print(selectedGame == i ? F("> ") : F("  "));
    display.print(GAME_NAMES[i]);
  }
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 55);
  display.print(F("Scroll / 13 select"));
  display.display();
}

void drawDifficulty() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0); display.print(GAME_NAMES[selectedGame]);
  display.setCursor(0, 11); display.print(F("Choose difficulty"));
  for (int i = 0; i < 2; ++i) {
    int y = 24 + i * 12;
    if (selectedDifficulty == i) display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
    display.setTextColor(selectedDifficulty == i ? SSD1306_BLACK : SSD1306_WHITE);
    display.setCursor(4, y);
    display.print(selectedDifficulty == i ? F("> ") : F("  "));
    display.print(i == 0 ? F("Easy") : F("Hard"));
  }
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 47);
  display.print(selectedGame == 2 ? F("Best rally: ") : F("Best: "));
  display.print(bestScores[selectedGame][selectedDifficulty]);
  display.setCursor(0, 56); display.print(F("13 play / 12 back"));
  display.display();
}

void showDifficulty() {
  gameState = DIFFICULTY;
  actionArmed = false; // Require a fresh press after selecting the game/retry.
  menuStickReady = false;
  drawDifficulty();
}

void drawGame() {
  if (selectedGame == 16) { topdownRPG.draw(display); return; }
  if (selectedGame == 15) { pinball.draw(display); return; }
  if (selectedGame == 14) { minesweeper.draw(display); return; }
  if (selectedGame == 13) { ticTacToe.draw(display); return; }
  if (selectedGame == 12) { fourInRow.draw(display); return; }
  if (selectedGame == 11) { slotMachine.draw(display); return; }
  if (selectedGame == 10) { templeQuest.draw(display); return; }
  if (selectedGame == 9) { rogueCards.draw(display); return; }
  if (selectedGame == 8) { streetFighter.draw(display); return; }
  if (selectedGame == 7) { blackjack.draw(display); return; }
  if (selectedGame == 6) { pacMan.draw(display); return; }
  if (selectedGame == 5) { duckHunt.draw(display); return; }
  if (selectedGame == 4) { castle.draw(display); return; }
  if (selectedGame == 3) { tetris.draw(display); return; }
  if (selectedGame == 2) { pong.draw(display); return; }
  if (selectedGame == 1) { invaders.draw(display); return; }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Score:"));
  display.print(snakeLength - 3);
  display.setCursor(100, 0);
  display.print(selectedDifficulty == 0 ? F("EASY") : F("HARD"));
  display.drawLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH - 1,
                   HEADER_HEIGHT - 1, SSD1306_WHITE);

  int foodPixelX = foodX * CELL_SIZE;
  int foodPixelY = HEADER_HEIGHT + foodY * CELL_SIZE;
  display.drawRect(foodPixelX, foodPixelY, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);

  for (int i = snakeLength - 1; i >= 0; --i) {
    int px = snakeX[i] * CELL_SIZE;
    int py = HEADER_HEIGHT + snakeY[i] * CELL_SIZE;
    if (i == 0) {
      display.fillRect(px, py, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
    } else {
      display.fillRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, SSD1306_WHITE);
    }
  }
  display.display();
}

void drawGameOver() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(selectedGame == 2 ? 16 : 10, 0);
  display.print(selectedGame == 2 ? (pong.playerScore >= 7 ? F("YOU WIN!") : F("CPU WINS")) :
                (selectedGame == 4 && castle.won) || (selectedGame == 9 && rogueCards.won) ||
                ((selectedGame == 10 && templeQuest.won) ||
                 (selectedGame == 12 && fourInRow.won) ||
                 (selectedGame == 13 && ticTacToe.won) ||
                 (selectedGame == 14 && minesweeper.won) ||
                 (selectedGame == 15 && pinball.won) ||
                 (selectedGame == 16 && topdownRPG.won)) ? F("YOU WIN!") :
                selectedGame == 7 ? F("FINISHED") : F("GAME OVER"));
  display.setTextSize(1);
  if (selectedGame == 2) {
    display.setCursor(15, 20);
    display.print(F("YOU ")); display.print(pong.playerScore);
    display.print(F(" - ")); display.print(pong.cpuScore); display.print(F(" CPU"));
    display.setCursor(0, 31);
    display.print(F("Best rally: ")); display.print(bestScores[selectedGame][selectedDifficulty]);
  } else {
  display.setCursor(23, 20);
  display.print(selectedGame == 11 ? F("Cash: ") : F("Score: "));
  if (selectedGame == 11) display.print(slotMachine.cash);
  else display.print(currentScore());
  display.setCursor(23, 31);
  display.print(F("Best:  "));
  display.print(bestScores[selectedGame][selectedDifficulty]);
  }
  display.setCursor(0, 45);
  display.print(selectedDifficulty == 0 ? F("EASY - 13: retry") : F("HARD - 13: retry"));
  display.setCursor(0, 56);
  display.print(F("12: game menu"));
  display.display();
}

void setup() {
  Serial.begin(115200);
  menuButton.begin();
  actionButton.begin();
  holdButton.begin();
  analogReadResolution(12);

  for (int i = 0; i < GAME_COUNT; ++i) {
    if (preferences.begin(SCORE_NAMESPACES[i], true)) {
      // Existing records stay available under Easy; Hard gets a separate key.
      bestScores[i][0] = preferences.getUInt("highscore", 0);
      bestScores[i][1] = preferences.getUInt("highscore_hard", 0);
      preferences.end();
    }
  }
  // Reset the Slot Machine Hard record once for the new realistic-odds mode;
  // the marker keeps later boots and future scores persistent as normal.
  if (preferences.begin("slots", false)) {
    if (preferences.getUInt("hard_reset_v", 0) < SLOT_HARD_SCORE_RESET_VERSION) {
      preferences.putUInt("highscore_hard", 0);
      preferences.putUInt("hard_reset_v", SLOT_HARD_SCORE_RESET_VERSION);
      bestScores[11][1] = 0;
    }
    preferences.end();
  }

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("OLED not found. Check wiring/address (0x3C or 0x3D)."));
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(12, 26);
  display.print(F("Center joystick..."));
  display.display();
  calibrateJoystick();

  randomSeed(micros() ^ analogRead(JOY_X_PIN) ^ analogRead(JOY_Y_PIN));
  drawTitle();
}

void loop() {
  menuButton.update();
  actionButton.update();
  holdButton.update();
  if (holdButton.released()) holdArmed = true;
  if (actionButton.released()) actionArmed = true;
  bool pressed = actionArmed && actionButton.pressed;

  if (menuButton.pressed) {
    if (gameState == PLAYING) finishGame();  // Save a record even when leaving mid-game.
    gameState = TITLE;
    actionArmed = false;
    menuStickReady = false;
    drawTitle();
    return;
  }
  // Back wins over select/fire if both buttons are held together.
  if (!menuButton.released()) {
    actionArmed = false;
    delay(1);
    return;
  }

  if (gameState == TITLE) {
    int x = joystickX(), y = joystickY();
    if (abs(x) < 350 && abs(y) < 350) menuStickReady = true;
    if (menuStickReady && (abs(x) > JOYSTICK_DEAD_ZONE || abs(y) > JOYSTICK_DEAD_ZONE)) {
      int step = abs(y) >= abs(x) ? (y > 0 ? -1 : 1) : (x > 0 ? 1 : -1);
      selectedGame = (selectedGame + GAME_COUNT + step) % GAME_COUNT;
      menuStickReady = false; // Return to center before scrolling again.
      drawTitle();
    }
    if (pressed) {
      showDifficulty();
    }
    return;
  }

  if (gameState == DIFFICULTY) {
    int x = joystickX(), y = joystickY();
    if (abs(x) < 350 && abs(y) < 350) menuStickReady = true;
    if (menuStickReady && (abs(x) > JOYSTICK_DEAD_ZONE || abs(y) > JOYSTICK_DEAD_ZONE)) {
      selectedDifficulty = 1 - selectedDifficulty;
      menuStickReady = false;
      drawDifficulty();
    }
    if (pressed) {
      startGame();
      drawGame();
    }
    return;
  }

  if (gameState == GAME_OVER) {
    if (pressed) showDifficulty();
    return;
  }

  if (selectedGame != 0) {
    if (selectedGame == 1) invaders.update(joystickX(), actionArmed && actionButton.held());
    else if (selectedGame == 2) pong.update(joystickY(), pressed);
    else if (selectedGame == 3) tetris.update(joystickX(), joystickY(), pressed, holdArmed && holdButton.pressed);
    else if (selectedGame == 4) castle.update(joystickX(), actionArmed && actionButton.held(), holdArmed && holdButton.pressed);
    else if (selectedGame == 5) duckHunt.update(joystickX(), joystickY(), pressed);
    else if (selectedGame == 6) pacMan.update(joystickX(), joystickY());
    else if (selectedGame == 7) blackjack.update(pressed, holdArmed && holdButton.pressed);
    else if (selectedGame == 8) streetFighter.update(joystickX(), joystickY(), pressed, holdArmed && holdButton.pressed);
    else if (selectedGame == 9) rogueCards.update(joystickX(), joystickY(), pressed, holdArmed && holdButton.pressed);
    else if (selectedGame == 10) templeQuest.update(joystickX(), joystickY(), pressed, holdArmed && holdButton.pressed);
    else if (selectedGame == 11) slotMachine.update(joystickY(), actionArmed && actionButton.held(), holdArmed && holdButton.pressed);
    else if (selectedGame == 12) fourInRow.update(joystickX(), pressed);
    else if (selectedGame == 13) ticTacToe.update(joystickX(), joystickY(), pressed);
    else if (selectedGame == 14) minesweeper.update(joystickX(), joystickY(), pressed, holdArmed && holdButton.pressed);
    else if (selectedGame == 15) pinball.update(actionArmed && actionButton.held(), holdArmed && holdButton.held());
    else topdownRPG.update(joystickX(), joystickY(), actionArmed && actionButton.held(), holdArmed && holdButton.pressed);
    bool ended = selectedGame == 1 ? invaders.over : selectedGame == 2 ? pong.over :
                 selectedGame == 3 ? tetris.over : selectedGame == 4 ? castle.over :
                 selectedGame == 5 ? duckHunt.over : selectedGame == 6 ? pacMan.over :
                 selectedGame == 7 ? blackjack.over : selectedGame == 8 ? streetFighter.over :
                 selectedGame == 9 ? rogueCards.over : selectedGame == 10 ? templeQuest.over :
                 selectedGame == 11 ? slotMachine.over : selectedGame == 12 ? fourInRow.over :
                 selectedGame == 13 ? ticTacToe.over : selectedGame == 14 ? minesweeper.over :
                 selectedGame == 15 ? pinball.over : topdownRPG.over;
    if (ended) {
      finishGame();
      drawGameOver();
    } else if (millis() - lastArcadeDraw >= 33) {
      lastArcadeDraw = millis();
      drawGame();
    }
    delay(1);
    return;
  }

  readJoystick();
  if (millis() - lastMoveTime >= movementInterval()) {
    lastMoveTime = millis();
    moveSnake();
    if (gameState == GAME_OVER) drawGameOver();
    else drawGame();
  }
}
