#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/SkullDepths.h"
#undef private
uint32_t testClock = 0;
using Game = SkullDepths;
void tick(Game &g, int sx = 0, int sy = 0, bool attack = false, bool dash = false) {
  testClock += 20; g.update(sx, sy, attack, dash);
}
void quiet(Game &g) {
  for (auto &e : g.enemies) e = {};
  for (auto &s : g.shots) s = {};
  g.enemies[0].x = 115; g.enemies[0].y = 45;
  g.enemies[0].hp = g.enemies[0].maxHp = 1000; g.enemies[0].cooldown = 10000;
  g.immune = 0;
}
void begin(Game &g, bool hard = false) { g.start(hard); tick(g, 0, 0, true); tick(g); quiet(g); }
int shots(Game &g) { int n = 0; for (auto &s : g.shots) n += s.life > 0; return n; }
int main() {
  Game g; Adafruit_SSD1306 display;
  g.start(false); assert(g.phase == Game::INTRO); g.draw(display);
  tick(g, 0, 0, true); assert(g.phase == Game::FIGHT && g.hp == 8);
  begin(g, true); assert(g.hp == 6);

  // Touching even an overlapping skull never causes damage.
  begin(g); auto &e = g.enemies[0]; e.x = g.x; e.y = g.y;
  for (int i = 0; i < 60; ++i) tick(g);
  assert(g.hp == 8);
  e.cooldown = 0; tick(g); assert(e.windup == 30 && g.hp == 8);
  for (int i = 0; i < 29; ++i) tick(g);
  assert(g.hp == 8); tick(g); assert(g.hp == 7);
  // A melee warning locks its location; leaving that circle dodges the hit.
  begin(g); e.x = 20; e.y = 32; e.cooldown = 0;
  tick(g); assert(e.windup); g.y = 46;
  for (int i = 0; i < 30; ++i) tick(g);
  assert(g.hp == 8);
  // Archers emit projectiles only after the aiming period.
  begin(g); e.archer = true; e.x = 110; e.y = 45; e.cooldown = 0;
  tick(g); assert(e.windup && shots(g) == 0);
  for (int i = 0; i < 30; ++i) tick(g);
  assert(shots(g) == 1);

  // Both attacks and arrows respect dash invulnerability; recovery still hurts.
  begin(g); tick(g, 1000, 0, false, true);
  assert(g.charges == 0 && g.dashTicks == 9);
  g.hurt(2); assert(g.hp == 8);
  g.shoot(g.x, g.y, 1, 0, false, 2); g.shotTick(); assert(g.hp == 8);
  for (int i = 0; i < 9; ++i) tick(g, 0, 0, false, true);
  assert(!g.dashTicks && g.charges == 0);
  g.hurt(2); assert(g.hp == 6);
  for (int i = 0; i < 110; ++i) tick(g, 0, 0, false, true);
  assert(g.charges == 1 && !g.dashTicks); // Holding never spends another dash.
  g.grant(Game::DASH); g.grant(Game::DASH); g.resetArena(); quiet(g);
  assert(g.charges == 3);

  // Brief input between physics frames is preserved.
  begin(g); g.update(0, 0, false, true); g.update(0, 0, false, false);
  tick(g); assert(g.charges == 0);
  // Dash and regular movement cannot cross cover or leave the playfield.
  begin(g); g.x = 43; g.y = 31; tick(g, 1000, 0, false, true);
  for (int i = 0; i < 20; ++i) tick(g, 1000);
  assert(g.x < 46 && !g.solid(g.x, g.y, 2));

  begin(g); g.grant(Game::SHADOW); tick(g, 0, 0, false, true);
  assert(g.stealth == 250);
  for (int i = 0; i < 249; ++i) tick(g);
  assert(g.stealth == 1); tick(g); assert(!g.stealth);
  // A whiff does not spend stealth; its first connected swing deals 200%.
  begin(g); g.stealth = 250; g.swing(); assert(g.stealth == 250);
  e.x = g.x + 8; e.y = g.y; e.hp = 20;
  g.swing(); assert(e.hp == 16 && !g.stealth);
  g.swing(); assert(e.hp == 12 && g.heavySlash); // Third swing adds two damage.
  e.x = g.x - 8; g.swing(); assert(e.hp == 10); // Auto-aim also reaches behind the player.
  g.grant(Game::MIGHT); e.x = g.x + 8; g.swing(); assert(e.hp == 7);

  // Aim at the closest reachable foe, while keeping movement/dash facing intact.
  begin(g);g.x=30;g.y=45;e.x=22;e.y=45;e.hp=40;
  g.enemies[1]=e;g.enemies[1].x=42;
  g.swing();assert(e.hp==38 && g.enemies[1].hp==40);
  assert(g.slashFx==-1 && g.slashFy==0 && g.fx==1 && g.fy==0);
  // Cover disqualifies a nearer target; sword reach still limits auto-aim.
  begin(g);g.x=44;g.y=33;e.x=53;e.y=33;e.hp=40;
  g.enemies[1]=e;g.enemies[1].x=32;
  g.swing();assert(e.hp==40 && g.enemies[1].hp==38 && g.slashFx==-1);
  begin(g);g.x=30;g.y=45;e.x=49;e.y=45;e.hp=40;
  g.swing();assert(e.hp==40 && g.slashFx==g.fx && g.slashFy==g.fy);
  // Finishers and reach upgrades extend the eligible auto-aim radius.
  e.x=30;e.y=60;g.combo=2;g.comboTimer=50;
  g.swing();assert(g.heavySlash && e.hp==36 && g.slashFy==1);
  begin(g);g.x=30;g.y=45;e.x=30;e.y=59;e.hp=40;
  g.grant(Game::REACH);g.swing();assert(e.hp==38 && g.slashFy==1);

  // Finishers interrupt normal windups, knock back, and respect boss resistance.
  begin(g); e.x=g.x+9;e.y=g.y;e.hp=30;e.windup=20;
  g.swing();g.swing();assert(e.windup==20 && e.hp==26);
  float oldX=e.x;g.swing();assert(e.hp==22 && e.stagger==18 && !e.windup && e.x>oldX);
  e.boss=true;e.x=g.x+9;e.windup=20;e.stagger=0;g.combo=2;g.comboTimer=50;
  g.swing();assert(e.windup==20 && !e.stagger);
  g.comboTimer=1;tick(g);assert(g.combo==0);
  g.combo=2;g.comboTimer=50;g.immune=0;g.hurt(1);assert(!g.combo && !g.comboTimer);
  // Sword attacks cannot hit through solid pillars.
  begin(g);g.x=43;g.y=33;e.x=55;e.y=33;e.hp=30;
  g.swing();assert(e.hp==30);
  e.windup=1;e.tx=g.x;e.ty=g.y;g.enemyTick(e);assert(g.hp==8);
  // A nearby hostile arrow is reflected once, aimed toward a live skull.
  begin(g);e.x=30;e.y=g.y;g.dashTicks=5;
  g.shoot(g.x,g.y,-1,0,false,1);g.shotTick();
  assert(g.shots[0].friendly && g.shots[0].vx>0 && g.shots[0].damage==g.damage());
  assert(g.parryFlash==20 && g.hp==8);
  for(int i=0;i<20;++i)g.shotTick();assert(e.hp==1000-g.damage());
  // The expanded arena has usable lower space; cover still blocks motion.
  begin(g);g.y=65;for(int i=0;i<30;++i)tick(g,0,-1000);
  assert(g.y>70 && g.y<=75 && !g.solid(g.x,g.y,2));
  // Five batched ticks equal five individual ticks; long stalls are bounded.
  begin(g);Game batched=g;
  for(int i=0;i<5;++i)tick(g,1000);
  batched.update(1000,0,false,false);
  assert(abs(g.x-batched.x)<0.001f && g.frames==batched.frames);
  uint32_t beforeFrames=batched.frames;testClock+=2000;batched.update(0,0,false,false);
  assert(batched.frames==beforeFrames+5);

  // Auto-arrow locks the nearest enemy and fires exactly once every second.
  begin(g); g.grant(Game::ARROW); e.x = 100; e.y = 45;
  g.enemies[1] = e; g.enemies[1].x = 30; g.enemies[1].y = 32;
  g.autoTimer = 50;
  for (int i = 0; i < 49; ++i) tick(g);
  assert(shots(g) == 0); tick(g); assert(shots(g) == 1);
  assert(abs(g.shots[0].vy) < 0.001f && g.shots[0].vx > 0);
  g.grant(Game::FROST); tick(g, 0, 0, false, true); assert(g.slow == 100);
  g.hp = 4; g.grant(Game::HEART); assert(g.maxHp == 10 && g.hp == 6);
  g.grant(Game::LEECH); g.meleeKills = 5; e.hp = 1; g.hit(e, 1, true); assert(g.hp == 7);
  int gold = g.gold; g.grant(Game::GOLD); e.hp = 1; g.hit(e, 1, false); assert(g.gold == gold + 11);

  // Entire campaign: unique choices, no skipped menus, shop BEFORE each boss.
  begin(g); g.grant(Game::MEND); g.hp = 4;
  for (int area = 1; area <= 3; ++area) {
    for (int room = 1; room <= 4; ++room) {
      assert(g.stage == area && g.room == room && g.phase == Game::FIGHT);
      for (auto &foe : g.enemies) { if (room == 4 && foe.hp) assert(foe.boss); foe.hp = 0; }
      tick(g); g.draw(display);
      if (area == 3 && room == 4) { assert(g.won && g.over); break; }
      assert(g.phase == Game::REWARD);
      assert(g.choices[0] != g.choices[1] && g.choices[1] != g.choices[2] && g.choices[0] != g.choices[2]);
      uint32_t savedScore = g.score;
      for (int j = 0; j < 5; ++j) tick(g);
      assert(g.score == savedScore); // Paused clear can't issue duplicate rewards.
      tick(g, 0, 0, true);
      if (room == 3) {
        assert(g.phase == Game::SHOP); g.draw(display);
        int before = g.gold; tick(g, 0, 0, true); assert(g.gold == before); // Held sword won't buy.
        g.selection = 3; tick(g); tick(g, 0, 0, true);
        assert(g.room == 4 && g.phase == Game::FIGHT);
      }
      tick(g);
    }
  }
  // Shop prices, full-health guard, and funded healing.
  begin(g); g.phase = Game::SHOP; g.selection = 0; g.gold = 0; g.hp = 2;
  g.buy(); assert(g.gold == 0 && g.hp == 2 && g.shopNotice);
  g.gold = 50; g.buy(); assert(g.gold == 30 && g.hp == 6);
  g.hp = g.maxHp; g.buy(); assert(g.gold == 30);
  g.selection = 1; g.gold = 35; g.buy(); assert(g.maxHp == 10 && g.hp == 10 && g.gold == 0);
  // Long mixed-input smoke runs check bounds and numerical stability.
  for (int mode = 0; mode < 2; ++mode) {
    begin(g, mode != 0);
    for (int i = 0; i < 10000 && !g.over; ++i) {
      tick(g, int(random(-1, 2))*1000, int(random(-1, 2))*1000, i%3 == 0, i%17 == 0);
      assert(std::isfinite(g.x) && std::isfinite(g.y) && !g.solid(g.x, g.y, 2));
      assert(g.charges >= 0 && g.charges <= g.maxCharges() && g.hp >= 0);
      if (i % 50 == 0) g.draw(display);
    }
  }
  std::cout << "PASS: Skull Depths combat, dash i-frames, stealth, arrows, perks, shops, progression and bounds.\n";
}
