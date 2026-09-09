#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Ten-hand, points-only Blackjack. No betting, splitting, doubling, or insurance.
class Blackjack {
 public:
  uint32_t score = 0;
  uint8_t handNumber = 1;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; handNumber = 1; over = false;
    deal();
  }

  void update(bool hitPressed, bool standPressed) {
    if (over) return;
    uint32_t now = millis();
    if (phase == RESULT) {
      // A fresh hit press advances; a held button never advances automatically.
      if (now - phaseAt >= 500 && hitPressed && !standPressed) {
        if (handNumber == 10) over = true;
        else { ++handNumber; deal(); }
      }
      return;
    }
    if (phase == DEALER) {
      if (now - phaseAt < 600) return;
      phaseAt = now;
      bool soft;
      int value = total(dealer, &soft);
      if (value < 17 || (hard && value == 17 && soft)) {
        add(dealer);
        if (total(dealer) > 21) finish(WIN);
      } else settle();
      return;
    }
    // Stand wins if both buttons are pressed together.
    if (standPressed) { beginDealer(); return; }
    if (hitPressed) {
      add(player);
      int value = total(player);
      if (value > 21) finish(BUST);
      else if (value == 21) beginDealer();
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(F("BJ ")); d.print(handNumber); d.print(F("/10"));
    d.setCursor(64, 0); d.print(F("S")); d.print(score);
    d.setCursor(121, 0); d.print(hard ? F("H") : F("E"));
    d.setCursor(0, 11); d.print(F("DEALER"));
    d.setCursor(48, 11);
    if (phase == PLAYER) d.print(F("?"));
    else d.print(total(dealer));
    drawHand(d, dealer, 20, phase == PLAYER);
    d.setCursor(0, 34); d.print(F("YOU"));
    d.setCursor(48, 34); d.print(total(player));
    bool soft;
    total(player, &soft);
    if (soft) { d.setCursor(72, 34); d.print(F("SOFT")); }
    drawHand(d, player, 43, false);
    d.setCursor(0, 56);
    if (phase == PLAYER) d.print(F("13 HIT   14 STAND"));
    else if (phase == DEALER) d.print(F("DEALER PLAYING..."));
    else {
      switch (result) {
        case NATURAL: d.print(F("BLACKJACK!")); break;
        case WIN: d.print(F("YOU WIN")); break;
        case PUSH: d.print(F("PUSH")); break;
        case BUST: d.print(F("BUST")); break;
        case LOSS: d.print(F("DEALER WINS")); break;
      }
      d.setCursor(80, 56); d.print(handNumber == 10 ? F("13 DONE") : F("13 NEXT"));
    }
    d.display();
  }

 private:
  struct Hand { uint8_t cards[12] = {}; uint8_t count = 0; };
  enum Phase { PLAYER, DEALER, RESULT };
  enum Result { WIN, NATURAL, PUSH, LOSS, BUST };
  Hand player, dealer;
  uint8_t deck[52] = {}, deckIndex = 0;
  Phase phase = PLAYER;
  Result result = PUSH;
  bool hard = false;
  uint32_t phaseAt = 0;

  static int total(const Hand &hand, bool *isSoft = nullptr) {
    int value = 0, aces = 0;
    for (int i = 0; i < hand.count; ++i) {
      int rank = hand.cards[i] % 13 + 1;
      if (rank == 1) { ++aces; value += 11; }
      else value += min(rank, 10);
    }
    while (value > 21 && aces) { value -= 10; --aces; }
    if (isSoft) *isSoft = aces > 0;
    return value;
  }
  void add(Hand &hand) {
    // With one deck, the longest non-bust hand has 11 cards; the 12th busts.
    if (hand.count < 12 && deckIndex < 52) hand.cards[hand.count++] = deck[deckIndex++];
  }
  void deal() {
    for (int i = 0; i < 52; ++i) deck[i] = i;
    for (int i = 51; i > 0; --i) {
      int j = random(i + 1); uint8_t saved = deck[i]; deck[i] = deck[j]; deck[j] = saved;
    }
    deckIndex = 0; player.count = dealer.count = 0;
    phase = PLAYER; phaseAt = millis();
    add(player); add(dealer); add(player); add(dealer);
    checkNaturals();
  }
  void checkNaturals() {
    bool p = player.count == 2 && total(player) == 21;
    bool d = dealer.count == 2 && total(dealer) == 21;
    if (p && d) finish(PUSH);
    else if (d) finish(LOSS);
    else if (p) finish(NATURAL);
  }
  void beginDealer() { phase = DEALER; phaseAt = millis(); }
  void settle() {
    int p = total(player), d = total(dealer);
    finish(d > 21 || p > d ? WIN : p == d ? PUSH : LOSS);
  }
  void finish(Result outcome) {
    if (phase == RESULT) return; // Results and score are awarded once per hand.
    result = outcome; phase = RESULT; phaseAt = millis();
    score += outcome == NATURAL ? 15 : outcome == WIN ? 10 : outcome == PUSH ? 2 : 0;
  }
  static void drawHand(Adafruit_SSD1306 &d, const Hand &hand, int y, bool hideHole) {
    const char ranks[] = "A23456789TJQK";
    for (int i = 0; i < hand.count; ++i) {
      int x = 2 + i * 10;
      d.drawRect(x, y, 9, 11, SSD1306_WHITE);
      d.setCursor(x + 2, y + 2);
      d.print(hideHole && i == 1 ? '?' : ranks[hand.cards[i] % 13]);
    }
  }
};
