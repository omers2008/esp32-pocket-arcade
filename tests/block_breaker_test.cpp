#define ARCADE_TEST_TFT
#include <Arduino.h>
#include "../ESP32_Snake/ArcadeDisplay.h"
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/BlockBreaker.h"
#undef private
uint32_t testClock=0;
using Game=BlockBreaker;
void launch(Game &g) {
  g.update(0,true);g.update(0,false);testClock+=20;g.update(0,false);
  assert(!g.waiting && g.balls[0].vy<0);
}
int main() {
  Game g;g.start(false);assert(g.hp==5 && g.stage==1 && g.waiting && g.remaining==40);
  launch(g);
  g.balls[0].x=g.paddle;g.balls[0].y=Game::PADDLE_Y-4;
  g.balls[0].vx=0;g.balls[0].vy=4;g.ballTick(g.balls[0]);
  assert(g.balls[0].vy<0 && abs(g.balls[0].vx)>0);
  g.balls[0].x=11;g.balls[0].y=150;g.balls[0].vx=-4;g.balls[0].vy=0;
  g.ballTick(g.balls[0]);assert(g.balls[0].vx>0 && g.balls[0].x>=11);
  for(int i=0;i<100;++i)g.tick(2047);assert(g.paddle+g.width()/2<=312);
  for(int i=0;i<100;++i)g.tick(-2048);assert(g.paddle-g.width()/2>=8);

  // Armored bricks take one damage per contact, then award a single destruction.
  g.start(false);g.waiting=false;for(auto &brick:g.bricks)brick=0;
  g.bricks[0]=3;g.remaining=1;
  for(int hit=0;hit<3;++hit) {
    auto &ball=g.balls[0];ball.x=Game::brickX(0)+13;
    ball.y=Game::brickY(0)+Game::BRICK_H+Game::RADIUS+0.4f;ball.vx=0;ball.vy=-g.speed();
    g.ballTick(ball);assert(g.bricks[0]==2-hit);
  }
  assert(g.remaining==0 && g.score==70);
  g.tick(0);assert(g.stage==2 && g.waiting && g.score==320);

  // All stages can clear, with a distinct victory after stage six.
  for(int stage=2;stage<=6;++stage) {
    assert(g.stage==stage && g.remaining>0);
    for(auto &brick:g.bricks)brick=0;g.remaining=0;g.waiting=false;
    g.balls[0].y=160;g.balls[0].vx=1;g.balls[0].vy=-1;g.tick(0);
  }
  assert(g.won && g.over && g.stage==6);

  // Each power has an effect; healing is capped and temporary powers expire.
  g.start(false);launch(g);g.grant(Game::WIDE);assert(g.width()==82);
  g.grant(Game::SLOW);assert(g.ballSpeed()<g.speed());
  g.grant(Game::MULTI);for(auto &ball:g.balls)assert(ball.active);
  g.hp=4;g.grant(Game::HEAL);g.grant(Game::HEAL);assert(g.hp==5);
  g.grant(Game::SHIELD);g.grant(Game::SHIELD);g.grant(Game::SHIELD);assert(g.shields==2);
  for(int i=1;i<3;++i)g.balls[i].active=false;
  auto &ball=g.balls[0];ball.x=20;ball.y=205;ball.vx=0;ball.vy=g.speed();
  g.ballTick(ball);assert(g.shields==1 && ball.active && ball.vy<0 && g.hp==5);
  g.wideTicks=g.slowTicks=1;ball.y=160;g.tick(0);
  assert(g.width()==56 && g.ballSpeed()==g.speed());

  // Catching a falling power changes HP rather than merely passing the paddle.
  g.hp=4;g.drops[0]={g.paddle,194,Game::HEAL,true};g.tick(0);
  assert(g.hp==5 && !g.drops[0].active);
  // HP only decreases after ALL balls are lost; bricks survive losing a life.
  g.shields=0;ball.y=217;ball.vy=1;g.balls[1]=ball;
  g.balls[1].y=160;g.balls[1].vy=-1;int remaining=g.remaining;
  g.tick(0);assert(!ball.active && g.hp==5 && !g.waiting);
  g.balls[1].active=false;g.tick(0);assert(g.hp==4 && g.waiting && g.remaining==remaining);
  while(!g.over){g.waiting=false;for(auto &b:g.balls)b.active=false;g.tick(0);}
  assert(g.hp==0 && !g.won);
  g.start(true);assert(g.hp==3 && g.width()==44 && g.speed()>3.2f);

  ArcadeDisplay d;assert(d.begin());
  for(int stage=1;stage<=6;++stage) {
    g.stage=stage;g.loadStage();auto score=g.score;int hp=g.hp,remaining=g.remaining;
    int transactions=tftTrace.transactions;g.draw(d);
    assert(tftTrace.transactions==transactions+6 && !tftTrace.writing);
    assert(g.score==score && g.hp==hp && g.remaining==remaining);
  }
  std::cout<<"PASS: launch, collisions, armor, stages, powers, HP, difficulty and native rendering.\n";
}
