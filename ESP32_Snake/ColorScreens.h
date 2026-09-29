#pragma once
#include "ColorMenu.h"

namespace ColorScreens {
inline const char *description(int game,bool hard) {
  static const char *const easy[]={
    "Slower starting speed", "3 lives / slower aliens", "Slower ball / gentler CPU",
    "Slower drop / longer lock", "6 health / slower enemies", "5 lives / forgiving aim",
    "2 ghosts / longer power", "Dealer stands on soft 17", "8 health / 2 foes at once",
    "Fixed deck / 32 starting HP", "5 lives / longer protection", "Equal symbol odds",
    "CPU picks random columns", "CPU picks random squares", "8 x 6 field / 8 mines",
    "3 balls / slower ball", "5 health / weaker enemies", "5 health / gentler foes",
    "Slower / wider gaps", "3 lives / fewer rocks", "3 lives / one-hit enemies",
    "8 health / gentler rooms", "3 lives / 120 seconds"};
  static const char *const tough[]={
    "Faster starting speed", "2 lives / faster attacks", "Faster ball / predictive CPU",
    "Faster drop / shorter lock", "4 health / tougher enemies", "3 lives / faster flocks",
    "3 ghosts / shorter power", "Dealer hits soft 17", "6 health / 3 foes at once",
    "Random deck / 26 starting HP", "3 lives / faster enemies", "Weighted odds / rare sevens",
    "CPU wins, blocks and plans", "CPU blocks / favors corners", "12 x 6 field / 16 mines",
    "2 balls / faster ball", "3 health / tougher enemies", "3 health / tougher foes",
    "Faster / closer obstacles", "2 lives / faster rocks", "2 lives / two-hit enemies",
    "6 health / fiercer enemies", "2 lives / 90 seconds"};
  return hard?tough[game]:easy[game];
}
inline void difficulty(Adafruit_GFX &d,int game,const char *name,int selected,const uint32_t *best) {
  using namespace ColorMenu;
  d.fillScreen(BG);d.setTextWrap(false);
  label(d,12,10,name,accent(game),2);
  label(d,12,34,"CHOOSE YOUR CHALLENGE",MUTED);
  for(int i=0;i<2;++i) {
    int y=53+i*77;uint16_t tint=i?0xFC19:0x5F54;
    d.fillRoundRect(10,y,300,69,7,selected==i?ACTIVE:CARD);
    d.drawRoundRect(10,y,300,69,7,selected==i?tint:CARD);
    d.fillRect(19,y+12,4,42,tint);
    label(d,32,y+9,i?"HARD":"EASY",tint,2);
    if(selected==i)label(d,238,y+13,"SELECTED",WHITE);
    label(d,32,y+32,description(game,i),WHITE);
    label(d,32,y+49,game==2?"BEST RALLY":"BEST",MUTED);
    d.setCursor(104,y+49);d.setTextColor(GOLD);d.print(best[i]);
  }
  label(d,12,212,"STICK  CHOOSE    13  START",WHITE);
  label(d,12,228,"12  BACK TO GAMES",MUTED);
}
inline void result(Adafruit_GFX &d,int game,const char *name,bool hard,const char *title,
                   int32_t score,uint32_t best,const char *scoreLabel,const char *detail) {
  using namespace ColorMenu;
  d.fillScreen(BG);d.setTextWrap(false);
  label(d,12,12,name,accent(game),2);
  label(d,12,38,hard?"HARD":"EASY",MUTED);
  label(d,28,62,title,WHITE,3);
  d.fillRoundRect(12,99,296,85,8,CARD);
  label(d,24,111,scoreLabel,MUTED);
  d.setTextSize(2);d.setTextColor(WHITE);d.setCursor(24,128);d.print(score);
  label(d,180,111,game==2?"BEST RALLY":"BEST",GOLD);
  d.setTextSize(2);d.setTextColor(GOLD);d.setCursor(180,128);d.print(best);
  label(d,24,162,detail,MUTED);
  label(d,30,200,"13  RETRY",CYAN,2);
  label(d,30,225,"12  BACK TO GAME MENU",MUTED);
}
}
