// Host previews use the installed Adafruit classic font and a raster TFT double.
#define ARCADE_TEST_TFT
#define ARCADE_RENDER_PREVIEW
#include <fstream>
#include <string>
#include "../Arduino.h"
#include "../../ESP32_Snake/ArcadeDisplay.h"
#include "../../ESP32_Snake/ColorScreens.h"
#define private public
#include "../../ESP32_Snake/Invaders.h"
#include "../../ESP32_Snake/Pong.h"
#include "../../ESP32_Snake/Tetris.h"
#include "../../ESP32_Snake/Castle.h"
#include "../../ESP32_Snake/DuckHunt.h"
#include "../../ESP32_Snake/PacMan.h"
#include "../../ESP32_Snake/Blackjack.h"
#include "../../ESP32_Snake/StreetFighter.h"
#include "../../ESP32_Snake/RogueCards.h"
#include "../../ESP32_Snake/TempleQuest.h"
#include "../../ESP32_Snake/SlotMachine.h"
#include "../../ESP32_Snake/FourInRow.h"
#include "../../ESP32_Snake/TicTacToe.h"
#include "../../ESP32_Snake/Minesweeper.h"
#include "../../ESP32_Snake/Pinball.h"
#include "../../ESP32_Snake/TopdownRPG.h"
#include "../../ESP32_Snake/BattleTanks.h"
#include "../../ESP32_Snake/DinoRunner.h"
#include "../../ESP32_Snake/Asteroids.h"
#include "../../ESP32_Snake/SkyPatrol.h"
#include "../../ESP32_Snake/SkullDepths.h"
#include "../../ESP32_Snake/DonkeyKong.h"
#include "../../ESP32_Snake/BlockBreaker.h"
#undef private
uint32_t testClock=2000;
void save(const std::string &name) {
  std::ofstream out("build/"+name+".bmp",std::ios::binary);
  auto word=[&](uint32_t n,int bytes){for(int i=0;i<bytes;++i)out.put(char(n>>(i*8)));};
  out.put('B');out.put('M');word(54+320*240*3,4);word(0,4);word(54,4);
  word(40,4);word(320,4);word(240,4);word(1,2);word(24,2);word(0,4);word(320*240*3,4);
  word(0,4);word(0,4);word(0,4);word(0,4);
  for(int y=239;y>=0;--y)for(int x=0;x<320;++x){uint16_t c=tftTrace.screen[y*320+x];
    out.put(char((c&31)*255/31));out.put(char(((c>>5)&63)*255/63));out.put(char((c>>11)*255/31));}
}
int main(){
  ArcadeDisplay d;assert(d.begin());
  uint32_t best[]={123456,987654};
  d.colorFrame([&](Adafruit_GFX &v){ColorScreens::difficulty(v,9,"Rogue Cards",1,best);});save("difficulty");
  d.colorFrame([&](Adafruit_GFX &v){ColorScreens::result(v,11,"Slot Machine",true,"GAME OVER",-100,10000000,"CASH","");});save("result");
  {Invaders g;g.start(false);d.setGame(1,"Invaders",false);g.draw(d);save("Invaders");}
  {Pong g;g.start(false);g.waiting=false;d.setGame(2,"Pong",false);g.draw(d);save("Pong");}
  {Tetris g;g.start(false);d.setGame(3,"Tetris",false);g.draw(d);save("Tetris");}
  {CastleGame g;g.start(false);d.setGame(4,"Castle",false);g.draw(d);save("Castle");}
  {DuckHunt g;g.start(false);d.setGame(5,"DuckHunt",false);g.draw(d);save("DuckHunt");}
  {PacMan g;g.start(false);d.setGame(6,"PacMan",false);g.draw(d);save("PacMan");}
  {Blackjack g;g.start(false);d.setGame(7,"Blackjack",false);g.draw(d);save("Blackjack");}
  {StreetFighter g;g.start(false);d.setGame(8,"StreetFighter",false);g.draw(d);save("StreetFighter");}
  {RogueCards g;g.start(false);g.beginBattle();d.setGame(9,"Rogue Cards",false);g.draw(d);save("RogueCards");}
  {TempleQuest g;g.start(false);g.started=true;d.setGame(10,"TempleQuest",false);g.draw(d);save("TempleQuest");}
  {SlotMachine g;g.start(false);d.setGame(11,"SlotMachine",false);g.draw(d);save("SlotMachine");}
  {FourInRow g;g.start(false);for(int y=4;y<6;++y)for(int x=0;x<7;++x)g.board[y][x]=1+(x+y)%2;d.setGame(12,"FourInRow",false);g.draw(d);save("FourInRow");}
  {TicTacToe g;g.start(false);g.board[0]=1;g.board[1]=2;g.board[8]=1;d.setGame(13,"TicTacToe",false);g.draw(d);save("TicTacToe");}
  {Minesweeper g;g.start(false);g.revealed[1][1]=true;g.adjacent[1][1]=1;g.revealed[1][2]=true;g.adjacent[1][2]=2;g.flagged[2][3]=true;d.setGame(14,"Minesweeper",false);g.draw(d);save("Minesweeper");}
  {Pinball g;g.start(false);d.setGame(15,"Pinball",false);g.draw(d);save("Pinball");}
  {TopdownRPG g;g.start(false);g.started=true;d.setGame(16,"TopdownRPG",false);g.draw(d);save("TopdownRPG");}
  {BattleTanks g;g.start(false);g.started=true;d.setGame(17,"BattleTanks",false);g.draw(d);save("BattleTanks");}
  {DinoRunner g;g.start(false);g.started=true;d.setGame(18,"DinoRunner",false);g.draw(d);save("DinoRunner");}
  {Asteroids g;g.start(false);g.started=true;d.setGame(19,"Asteroids",false);g.draw(d);save("Asteroids");}
  {SkyPatrol g;g.start(false);g.started=true;d.setGame(20,"SkyPatrol",false);g.draw(d);save("SkyPatrol");}
  {SkullDepths g;g.start(false);d.setGame(21,"SkullDepths",false);g.draw(d);save("SkullIntro");
    g.phase=SkullDepths::FIGHT;g.x=37;g.y=49;g.immune=0;g.combo=3;g.comboTimer=30;g.slash=7;g.heavySlash=true;
    g.enemies[0].x=60;g.enemies[0].y=46;g.enemies[0].windup=20;g.enemies[0].tx=g.x;g.enemies[0].ty=g.y;
    g.enemies[1].windup=15;g.enemies[1].ax=-1;g.enemies[1].ay=0.6f;
    g.draw(d);save("SkullDepths");g.phase=SkullDepths::REWARD;g.choices[0]=0;g.choices[1]=2;g.choices[2]=4;
    g.selection=1;g.draw(d);save("SkullReward");g.phase=SkullDepths::SHOP;g.gold=30;g.hp=4;g.draw(d);save("SkullShop");}
  {DonkeyKong g;g.start(false);g.started=true;d.setGame(22,"Donkey Kong",false);g.draw(d);save("DonkeyKong");}
  {BlockBreaker g;g.start(false);d.setGame(23,"Block Breaker",false);g.draw(d);save("BlockBreakerStart");
    g.stage=6;g.loadStage();g.waiting=false;g.score=4800;g.hp=3;g.wideTicks=240;g.shields=1;
    g.balls[0]={120,155,1,-3,true};g.grant(BlockBreaker::MULTI);
    g.balls[1].x=145;g.balls[1].y=168;g.balls[2].x=188;g.balls[2].y=150;
    for(int i=0;i<5;++i)g.drops[i]={float(44+i*54),float(133+(i%2)*16),static_cast<BlockBreaker::Power>(i),true};
    g.draw(d);save("BlockBreakerPowers");}
}
