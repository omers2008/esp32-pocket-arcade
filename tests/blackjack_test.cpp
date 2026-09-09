#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <initializer_list>
#include <iostream>
#define private public
#include "../ESP32_Snake/Blackjack.h"
#undef private
uint32_t testClock = 0;
Blackjack::Hand hand(std::initializer_list<int> ranks) {
  Blackjack::Hand h;
  for (int rank : ranks) h.cards[h.count++] = rank - 1;
  return h;
}
void fixture(Blackjack &g, std::initializer_list<int> player, std::initializer_list<int> dealer) {
  g.start(false); g.player = hand(player); g.dealer = hand(dealer);
  g.phase = Blackjack::PLAYER; g.score = 0; g.phaseAt = testClock;
}
void tick(Blackjack &g, bool hit = false, bool stand = false, uint32_t ms = 600) {
  testClock += ms; g.update(hit, stand);
}
int main() {
  std::srand(42);
  bool soft = false;
  assert(Blackjack::total(hand({1, 1, 9}), &soft) == 21 && soft);
  assert(Blackjack::total(hand({1, 1, 9, 10}), &soft) == 21 && !soft);
  assert(Blackjack::total(hand({1, 1, 1, 1, 7}), &soft) == 21 && soft);
  assert(Blackjack::total(hand({11, 12, 13})) == 30);
  // Check every ordered three-rank combination against an independent ace evaluator.
  for (int a = 1; a <= 13; ++a) for (int b = 1; b <= 13; ++b) for (int c = 1; c <= 13; ++c) {
    int sum = min(a, 10) + min(b, 10) + min(c, 10);
    bool expectedSoft = (a == 1 || b == 1 || c == 1) && sum + 10 <= 21;
    if (expectedSoft) sum += 10;
    assert(Blackjack::total(hand({a, b, c}), &soft) == sum && soft == expectedSoft);
  }
  Blackjack g;
  fixture(g, {1, 13}, {10, 9}); g.checkNaturals();
  assert(g.result == Blackjack::NATURAL && g.score == 15);
  g.checkNaturals(); assert(g.score == 15); // Result scoring is idempotent.
  fixture(g, {1, 10}, {1, 12}); g.checkNaturals();
  assert(g.result == Blackjack::PUSH && g.score == 2);
  fixture(g, {7, 7, 7}, {1, 10}); g.checkNaturals();
  assert(g.result == Blackjack::LOSS && g.score == 0);
  fixture(g, {10, 6}, {9, 8}); g.deck[g.deckIndex] = 12;
  tick(g, true); assert(g.result == Blackjack::BUST && Blackjack::total(g.player) == 26 && g.score == 0);
  fixture(g, {10, 6}, {10, 7}); g.deck[g.deckIndex] = 4;
  tick(g, true); assert(g.phase == Blackjack::DEALER && Blackjack::total(g.player) == 21);
  tick(g); assert(g.result == Blackjack::WIN && g.score == 10);
  fixture(g, {10, 8}, {10, 8}); tick(g, false, true); tick(g);
  assert(g.result == Blackjack::PUSH && g.score == 2);
  fixture(g, {10, 8}, {10, 9}); tick(g, false, true); tick(g);
  assert(g.result == Blackjack::LOSS && g.score == 0);
  fixture(g, {10, 8}, {10, 6}); g.deck[g.deckIndex] = 9;
  tick(g, false, true); tick(g); assert(g.result == Blackjack::WIN && g.score == 10);
  for (bool hard : {false, true}) {
    fixture(g, {10, 8}, {1, 6}); g.hard = hard; g.deck[g.deckIndex] = 1;
    int used = g.deckIndex;
    tick(g, true, true); assert(g.phase == Blackjack::DEALER && g.player.count == 2); // Stand priority.
    tick(g, true, true, 599); assert(g.deckIndex == used); // Ignore buttons during dealer turn.
    tick(g, false, false, 1);
    if (hard) {
      assert(g.dealer.count == 3 && Blackjack::total(g.dealer) == 19);
      tick(g); assert(g.result == Blackjack::LOSS);
    } else assert(g.dealer.count == 2 && g.result == Blackjack::WIN);
    fixture(g, {10, 8}, {10, 7}); g.hard = hard;
    tick(g, false, true); tick(g); assert(g.dealer.count == 2 && g.result == Blackjack::WIN);
  }
  // Result requires a fresh press after the short guard, not a held button or Stand.
  fixture(g, {1, 10}, {9, 8}); g.checkNaturals();
  tick(g, true, false, 499); assert(g.handNumber == 1);
  tick(g, false, true); assert(g.handNumber == 1);
  tick(g, false, false); assert(g.handNumber == 1);
  tick(g, true); assert(g.handNumber == 2 && g.score >= 15);
  // Finite sessions: all ten hand results remain visible until Next/Done is pressed.
  g.start(false); g.score = 0;
  for (int n = 1; n <= 10; ++n) {
    assert(g.handNumber == n && !g.over);
    g.phase = Blackjack::PLAYER; g.finish(Blackjack::WIN);
    tick(g, false, true); assert(!g.over && g.handNumber == n);
    tick(g, true);
  }
  assert(g.over && g.handNumber == 10);
  uint32_t score = g.score; tick(g, true, true); assert(g.score == score);
  // Full-deck shuffling, distinct cards, bounded hands, and complete random play.
  Adafruit_SSD1306 display;
  for (bool hard : {false, true}) {
    g.start(hard);
    for (int i = 0; i < 20000; ++i) {
      bool seen[52] = {};
      for (int card : g.deck) { assert(card < 52 && !seen[card]); seen[card] = true; }
      assert(g.player.count >= 2 && g.player.count <= 12 && g.dealer.count >= 2 && g.dealer.count <= 12);
      assert(g.deckIndex == g.player.count + g.dealer.count && g.deckIndex <= 24);
      assert(g.score <= 15u * g.handNumber);
      bool playerHit = std::rand() % 3 != 0;
      tick(g, g.phase == Blackjack::RESULT || playerHit, g.phase == Blackjack::PLAYER && !playerHit);
      g.draw(display);
      if (g.over) g.start(hard);
    }
  }
  testClock = UINT32_MAX - 300;
  fixture(g, {10, 8}, {10, 7}); tick(g, false, true, 1); tick(g);
  assert(g.result == Blackjack::WIN);
  std::cout << "PASS: Blackjack aces, unique deck, naturals, ties, busts, dealer rules, button priority, result guards, 10-hand sessions, rollover, 40000 input/draw steps.\n";
}
