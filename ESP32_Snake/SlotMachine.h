#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// A small animated three-reel slot machine. Cash is allowed to reach zero;
// the run ends only after a spin leaves the player in debt.
class SlotMachine {
 public:
  int32_t cash = 10000;
  int32_t wager = 100;
  uint32_t score = 10000;
  bool over = false;
  bool won = false;

  void start(bool hardMode) {
    hard = hardMode;
    cash = 10000;
    wager = 100;
    score = 10000;
    over = won = false;
    for (int &reel : reels) reel = random(6);
    resultKind = 0;
    leverDown = false;
    stickReady = false;
    phase = IDLE;
    phaseAt = millis();
    // Permit the first held-button repeat immediately after entering the game.
    lastWagerAdjust = phaseAt - 160;
  }

  void update(int stickY, bool increaseHeld, bool decreaseHeld) {
    uint32_t now = millis();
    if (abs(stickY) < 350) stickReady = true;

    if (phase == IDLE) {
      if (increaseHeld && now - lastWagerAdjust >= 160) adjustWager(100);
      if (decreaseHeld && now - lastWagerAdjust >= 160) adjustWager(-100);
      if (stickReady && stickY < -650) {
        stickReady = false;
        beginSpin(now);
      }
      return;
    }

    if (phase == LEVER) {
      if (now - phaseAt >= 240) {
        phase = SPIN;
        phaseAt = now;
        lastReelTick = now;
      }
      return;
    }

    if (phase == SPIN) {
      uint32_t elapsed = now - phaseAt;
      if (elapsed < 2000) {
        if (now - lastReelTick >= 75) {
          lastReelTick = now;
          reels[0] = random(6);
          reels[1] = random(6);
          reels[2] = random(6);
        }
      } else {
        settleSpin();
        phase = RESULT;
        phaseAt = now;
      }
      return;
    }

    // Leave the result on screen long enough to see the hit or jackpot.
    if (phase == RESULT && now - phaseAt >= 1400) {
      if (cash < 0) over = true;
      else {
        phase = IDLE;
        leverDown = false;
        stickReady = false;
      }
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay();
    d.setTextColor(SSD1306_WHITE);
    d.setTextSize(1);
    d.setCursor(0, 0); d.print(F("C$")); d.print(cash);
    d.setCursor(78, 0); d.print(F("B$")); d.print(wager);
    d.drawFastHLine(0, 9, 128, SSD1306_WHITE);

    for (int i = 0; i < 3; ++i) {
      int x = 5 + i * 36;
      d.drawRoundRect(x, 17, 32, 28, 3, SSD1306_WHITE);
      drawSymbol(d, reels[i], x, 17);
    }

    // Animated lever: it drops during the pull and rises after the result.
    int leverY = leverDown ? 45 : 27;
    d.drawLine(116, 20, 116, leverY, SSD1306_WHITE);
    d.fillCircle(116, leverY, 4, SSD1306_WHITE);
    d.drawCircle(116, 20, 2, SSD1306_WHITE);

    d.setTextSize(1);
    if (phase == IDLE) {
      d.setCursor(0, 48); d.print(F("13:+$100  14:-$100"));
      d.setCursor(0, 57); d.print(F("DOWN: SPIN"));
    } else if (phase == LEVER || phase == SPIN) {
      d.setCursor(39, 53); d.print(phase == LEVER ? F("PULL!") : F("SPINNING..."));
    } else {
      if (resultKind == 2) { d.setCursor(31, 53); d.print(F("JACKPOT!")); }
      else if (resultKind == 1) { d.setCursor(31, 53); d.print(F("TRIPLE HIT!")); }
      else { d.setCursor(46, 53); d.print(F("NO WIN")); }
    }
    d.display();
  }

 private:
  enum Phase { IDLE, LEVER, SPIN, RESULT };
  Phase phase = IDLE;
  int reels[3] = {0, 1, 2};
  int resultKind = 0;
  bool hard = false, leverDown = false, stickReady = false;
  uint32_t phaseAt = 0, lastReelTick = 0, lastWagerAdjust = 0;

  void adjustWager(int delta) {
    wager = constrain(wager + delta, 100, 5000);
    lastWagerAdjust = millis();
  }

  void beginSpin(uint32_t now) {
    cash -= wager;
    leverDown = true;
    phase = LEVER;
    phaseAt = now;
  }

  void settleSpin() {
    bool triple = reels[0] == reels[1] && reels[1] == reels[2];
    bool pair = reels[0] == reels[1] || reels[1] == reels[2] || reels[0] == reels[2];
    if (triple) {
      // Lucky seven is the top jackpot; classic machine symbols pay by rarity.
      static const int multipliers[6] = {3, 8, 10, 4, 6, 50};
      cash += wager * multipliers[reels[0]];
      resultKind = reels[0] == 5 ? 2 : 1;
    } else if (pair) {
      cash += wager * 2;
      resultKind = 1;
    } else {
      resultKind = 0;
    }
    if (cash > int32_t(score)) score = uint32_t(cash);
  }

  static void drawSymbol(Adafruit_SSD1306 &d, int symbol, int x, int y) {
    int cx = x + 16, cy = y + 14;
    switch (constrain(symbol, 0, 5)) {
      case 0: // Cherries.
        d.drawLine(cx - 5, cy - 5, cx - 2, cy - 11, SSD1306_WHITE);
        d.drawLine(cx + 5, cy - 5, cx + 2, cy - 11, SSD1306_WHITE);
        d.drawCircle(cx - 6, cy - 1, 5, SSD1306_WHITE);
        d.drawCircle(cx + 6, cy - 1, 5, SSD1306_WHITE);
        break;
      case 1: // BAR symbol: three stacked bars, no text required.
        d.drawRect(x + 4, y + 5, 24, 18, SSD1306_WHITE);
        d.drawFastHLine(x + 6, y + 10, 20, SSD1306_WHITE);
        d.drawFastHLine(x + 6, y + 17, 20, SSD1306_WHITE);
        break;
      case 2: // Lucky seven.
        d.drawFastHLine(x + 7, y + 6, 18, SSD1306_WHITE);
        d.drawLine(x + 24, y + 6, x + 13, y + 22, SSD1306_WHITE);
        d.drawLine(x + 13, y + 22, x + 10, y + 22, SSD1306_WHITE);
        break;
      case 3: // Liberty Bell silhouette.
        d.drawCircle(cx, cy - 2, 8, SSD1306_WHITE);
        d.fillRect(x + 8, y + 16, 16, 5, SSD1306_WHITE);
        d.drawFastHLine(x + 6, y + 22, 20, SSD1306_WHITE);
        d.fillCircle(cx, y + 22, 2, SSD1306_WHITE);
        break;
      case 4: // Lemon.
        d.drawCircle(cx, cy, 9, SSD1306_WHITE);
        d.drawLine(cx - 6, cy + 6, cx + 5, cy - 6, SSD1306_WHITE);
        break;
      default: // Plum.
        d.drawCircle(cx, cy + 1, 8, SSD1306_WHITE);
        d.drawLine(cx, cy - 7, cx + 5, cy - 11, SSD1306_WHITE);
        d.drawLine(cx + 4, cy - 10, cx + 9, cy - 9, SSD1306_WHITE);
        break;
    }
  }
};
