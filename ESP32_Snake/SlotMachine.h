#pragma once
#include <Arduino.h>
#include "ArcadeDisplay.h"

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
    for (int &reel : reels) reel = randomSymbol();
    resultKind = 0;
    leverDown = false;
    stickReady = false;
    phase = IDLE;
    phaseAt = millis();
    // Permit the first held-button repeat immediately after entering the game.
    lastWagerAdjust = phaseAt - 160;
    wagerRepeatMs = 160;
  }

  void update(int stickY, bool increaseHeld, bool decreaseHeld) {
    uint32_t now = millis();
    if (abs(stickY) < 350) stickReady = true;

    if (phase == IDLE) {
      if (increaseHeld || decreaseHeld) {
        if (now - lastWagerAdjust >= wagerRepeatMs) {
          adjustWager(increaseHeld ? 100 : -100);
          wagerRepeatMs = wagerRepeatMs > 57 ? wagerRepeatMs - 12 : 45;
        }
      } else {
        // A new hold always starts at the comfortable default repeat rate.
        wagerRepeatMs = 160;
        lastWagerAdjust = now;
      }
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
      if (elapsed < 1200) {
        if (now - lastReelTick >= 75) {
          lastReelTick = now;
          reels[0] = randomSymbol();
          reels[1] = randomSymbol();
          reels[2] = randomSymbol();
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

  void draw(ArcadeDisplay &d) {
    d.clearDisplay();
    d.setTextColor(SSD1306_WHITE);
    d.setTextSize(1);
    d.setCursor(0, 0); d.print(F("C$")); d.print(cash);
    d.setCursor(78, 0); d.print(F("B$")); d.print(wager);
    d.drawFastHLine(0, 9, 128, Ink::Wall);

    for (int i = 0; i < 3; ++i) {
      int x = 5 + i * 36;
      bool flash=phase==RESULT && resultKind>0 && (millis()/150)%2;
      d.drawRoundRect(x, 17, 32, 28, 3, flash?Ink::Pink:Ink::Gold);
      drawSymbol(d,reels[i],x+16,30);
    }

    // Animated lever: it drops during the pull and rises after the result.
    int leverY = leverDown ? 45 : 27;
    d.drawLine(116, 20, 116, leverY, Ink::Muted);
    d.fillCircle(116, leverY, 4, Ink::Red);
    d.drawCircle(116, 20, 2, Ink::Gold);

    d.setTextSize(1);
    if (phase == IDLE) {
      d.setCursor(0, 48); d.print(F("13:+$100  14:-$100"));
      d.setCursor(0, 57); d.print(F("DOWN: SPIN"));
    } else if (phase == LEVER || phase == SPIN) {
      d.setCursor(39, 53); d.print(phase == LEVER ? F("PULL!") : F("SPINNING..."));
    } else {
      d.setTextColor(resultKind>0?Ink::Gold:Ink::Muted);
      if (resultKind == 2) { d.setCursor(31, 53); d.print(F("JACKPOT!")); }
      else if (resultKind == 3) { d.setCursor(31, 53); d.print(F("TRIPLE HIT!")); }
      else if (resultKind == 1) { d.setCursor(43, 53); d.print(F("PAIR HIT!")); }
      else { d.setCursor(46, 53); d.print(F("NO WIN")); }
    }
    d.display();
  }

 private:
  static void drawSymbol(ArcadeDisplay &d,int symbol,int x,int y) {
    if(symbol==0) {
      d.drawLine(x-5,y,x,y-9,Ink::Green);d.drawLine(x+5,y,x,y-9,Ink::Green);
      d.fillCircle(x-5,y+3,4,Ink::Red);d.fillCircle(x+5,y+3,4,Ink::Red);
      d.drawPixel(x-6,y+1,Ink::White);d.drawPixel(x+4,y+1,Ink::White);
    } else if(symbol==1) {
      d.fillRect(x-12,y-6,24,13,Ink::White);
      d.setTextColor(Ink::Dark);d.setTextSize(1);d.setCursor(x-9,y-3);d.print(F("BAR"));
    } else if(symbol==2) {
      d.fillCircle(x,y-5,4,Ink::Gold);d.fillRect(x-5,y-5,11,10,Ink::Gold);
      d.drawFastHLine(x-7,y+5,15,Ink::Gold);d.fillCircle(x,y+7,2,Ink::Orange);
      d.drawFastVLine(x-3,y-5,7,Ink::White);
    } else if(symbol==3) {
      d.fillCircle(x-3,y,5,Ink::Gold);d.fillCircle(x+3,y,5,Ink::Gold);
      d.drawPixel(x-9,y,Ink::Gold);d.drawPixel(x+9,y,Ink::Gold);
      d.drawFastHLine(x-3,y-3,5,Ink::White);
    } else if(symbol==4) {
      d.fillCircle(x,y+1,7,Ink::Purple);d.drawFastVLine(x,y-9,4,Ink::Brown);
      d.drawLine(x,y-7,x+5,y-9,Ink::Green);d.drawFastVLine(x-3,y-2,4,Ink::Pink);
    } else {
      d.setTextSize(2);d.setTextColor(Ink::Red);d.setCursor(x-5,y-7);d.print('7');
      d.drawPixel(x-10,y-6,Ink::Gold);d.drawPixel(x+10,y+6,Ink::Gold);
    }
    d.setTextSize(1);d.setTextColor(Ink::White);
  }
  enum Phase { IDLE, LEVER, SPIN, RESULT };
  Phase phase = IDLE;
  int reels[3] = {0, 1, 2};
  int resultKind = 0;
  bool hard = false, leverDown = false, stickReady = false;
  uint32_t phaseAt = 0, lastReelTick = 0, lastWagerAdjust = 0;
  uint16_t wagerRepeatMs = 160;

  int randomSymbol() const {
    if (!hard) return random(6);
    // Hard uses a weighted reel, closer to a real machine than six equally
    // likely symbols. Lucky 7 is intentionally rare.
    static const int weights[6] = {30, 20, 18, 16, 12, 4};
    int roll = random(100);
    for (int symbol = 0; symbol < 6; ++symbol) {
      if (roll < weights[symbol]) return symbol;
      roll -= weights[symbol];
    }
    return 5;
  }

  void adjustWager(int delta) {
    wager = constrain(wager + delta, 100, 10000);
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
      resultKind = reels[0] == 5 ? 2 : 3;
    } else if (pair) {
      cash += wager * 2;
      resultKind = 1;
    } else {
      resultKind = 0;
    }
    if (cash > int32_t(score)) score = uint32_t(cash);
  }

  static const char *symbolText(int symbol) {
    static const char *names[] = {"CHRY", "BAR", "BELL", "LEMN", "PLUM", "7"};
    return names[constrain(symbol, 0, 5)];
  }
  static int symbolOffset(int symbol) {
    return symbol == 1 ? 7 : symbol == 5 ? 13 : 4;
  }
};
