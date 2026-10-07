#pragma once
#include <Arduino.h>
#include "ArcadeDisplay.h"
#include <math.h>

class BlockBreaker {
 public:
  uint32_t score=0;
  bool over=false,won=false;
  int hp=5,stage=1;

  void start(bool hardMode) {
    hard=hardMode;score=0;over=won=false;hp=maxHp=hard?3:5;stage=1;
    paddle=160;lastFrame=millis();loadStage();
  }

  void update(int stickX,bool launchPressed) {
    if(over)return;
    if(waiting && launchPressed)launchQueued=true;
    uint32_t now=millis();
    if(now-lastFrame>100)lastFrame=now-100;
    while(now-lastFrame>=20 && !over) {
      lastFrame+=20;tick(stickX);
    }
  }

  void draw(ArcadeDisplay &d) {
#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
    if(d.colorFrame([&](Adafruit_GFX &canvas){drawColor(canvas);}))return;
#endif
    d.clearDisplay();d.setTextSize(1);d.setTextColor(SSD1306_WHITE);
    d.setCursor(0,0);d.print(F("BRK S"));d.print(stage);d.print(F(" HP"));d.print(hp);
    d.setCursor(88,0);d.print(score);
    for(int i=0;i<60;++i)if(bricks[i])d.fillRect(int(brickX(i)*0.4f),int(brickY(i)*0.25f),10,2,brickColor(i));
    d.fillRect(int((paddle-width()/2)*0.4f),55,int(width()*0.4f),2,Ink::Cyan);
    for(auto &b:balls)if(b.active)d.fillRect(int(b.x*0.4f),int(b.y*0.25f)+5,2,2,Ink::Gold);
    if(waiting){d.setCursor(36,40);d.print(F("13 LAUNCH"));}
    d.display();
  }

 private:
  static constexpr int STAGES=6,COLS=10,BRICK_W=27,BRICK_H=10,PADDLE_Y=199;
  static constexpr float RADIUS=3;
  enum Power { WIDE,SLOW,MULTI,HEAL,SHIELD };
  struct Ball {float x=160,y=195,vx=0,vy=0;bool active=false;};
  struct Drop {float x=0,y=0;Power power=WIDE;bool active=false;};
  Ball balls[3];Drop drops[6];uint8_t bricks[60]={};
  bool hard=false,waiting=true,launchQueued=false;
  int maxHp=5,remaining=0,wideTicks=0,slowTicks=0,shields=0,hitFlash=0;
  float paddle=160;
  uint32_t lastFrame=0;

  float width() const {return wideTicks?82.0f:(hard?44.0f:56.0f);}
  float speed() const {return (hard?4.1f:3.2f)+(stage-1)*0.28f;}
  float ballSpeed() const {return speed()*(slowTicks?0.7f:1.0f);}
  static float brickX(int i){return 16.0f+(i%COLS)*29;}
  static float brickY(int i){return 53.0f+(i/COLS)*14;}
  uint16_t brickColor(int i) const {
    if(bricks[i]>=3)return Ink::Gold;
    if(bricks[i]==2)return Ink::Purple;
    const uint16_t colors[]={Ink::Cyan,Ink::Green,Ink::Orange,Ink::Pink,Ink::Blue,Ink::Red};
    return colors[(i/COLS+stage-1)%6];
  }
  void ready() {
    for(auto &b:balls)b=Ball{};
    balls[0].active=true;balls[0].x=paddle;balls[0].y=PADDLE_Y-RADIUS-1;
    waiting=true;launchQueued=false;
  }
  void loadStage() {
    remaining=0;wideTicks=slowTicks=shields=hitFlash=0;
    for(auto &drop:drops)drop=Drop{};
    int rows=min(6,3+(stage+1)/2);
    for(int i=0;i<60;++i) {
      int row=i/COLS,col=i%COLS;
      bool gap=stage==2?(row%2 && (col==0 || col==9)):
        stage==3?((row+col)%4==0):stage==4?(col==4 || col==5):
        stage==5?((row+col)%5==0):false;
      bricks[i]=row<rows && !gap?uint8_t(1+(stage>=3 && row<2)+(stage>=5 && (row+col)%3==0)):0;
      if(bricks[i])++remaining;
    }
    ready();
  }
  void spawnDrop(float px,float py) {
    if(random(100)>=30)return;
    for(auto &d:drops)if(!d.active){d={px,py,static_cast<Power>(random(5)),true};return;}
  }
  void grant(Power power) {
    score+=25;
    if(power==WIDE)wideTicks=500;
    else if(power==SLOW)slowTicks=400;
    else if(power==HEAL)hp=min(maxHp,hp+1);
    else if(power==SHIELD)shields=min(2,shields+1);
    else {
      Ball source;bool found=false;
      for(auto &b:balls)if(b.active){source=b;found=true;break;}
      if(!found || waiting)return;
      int n=0;
      for(auto &b:balls)if(!b.active) {
        float angle=(++n==1?-0.6f:0.6f);
        b=source;b.vx=source.vx*cosf(angle)-source.vy*sinf(angle);
        b.vy=-abs(source.vx*sinf(angle)+source.vy*cosf(angle));
        if(abs(b.vy)<1)b.vy=-1;
      }
    }
  }
  void advance() {
    score+=250;
    if(stage==STAGES){won=over=true;score+=1000;return;}
    ++stage;loadStage();
  }
  void paddleBounce(Ball &b) {
    float offset=constrain((b.x-paddle)/(width()/2),-1.0f,1.0f);
    float magnitude=ballSpeed();b.vx=offset*magnitude*0.85f;
    if(abs(b.vx)<0.65f)b.vx=b.vx<0?-0.65f:0.65f;
    b.vy=-sqrtf(max(0.1f,magnitude*magnitude-b.vx*b.vx));
    b.y=PADDLE_Y-RADIUS-0.1f;
  }
  void ballTick(Ball &b) {
    float magnitude=sqrtf(b.vx*b.vx+b.vy*b.vy);
    if(magnitude>0){b.vx*=ballSpeed()/magnitude;b.vy*=ballSpeed()/magnitude;}
    for(int step=0;step<4 && b.active;++step) {
      float oldX=b.x,oldY=b.y;b.x+=b.vx/4;b.y+=b.vy/4;
      if(b.x<11){b.x=11;b.vx=abs(b.vx);}
      if(b.x>309){b.x=309;b.vx=-abs(b.vx);}
      if(b.y<49){b.y=49;b.vy=abs(b.vy);}
      if(b.vy>0 && oldY+RADIUS<=PADDLE_Y && b.y+RADIUS>=PADDLE_Y &&
          abs(b.x-paddle)<=width()/2+RADIUS)paddleBounce(b);
      for(int i=0;i<60;++i)if(bricks[i]) {
        float bx=brickX(i),by=brickY(i);
        float nearX=constrain(b.x,bx,bx+BRICK_W),nearY=constrain(b.y,by,by+BRICK_H);
        float tx=b.x-nearX,ty=b.y-nearY;
        if(tx*tx+ty*ty>RADIUS*RADIUS)continue;
        if(oldY+RADIUS<=by){b.y=by-RADIUS-0.1f;b.vy=-abs(b.vy);}
        else if(oldY-RADIUS>=by+BRICK_H){b.y=by+BRICK_H+RADIUS+0.1f;b.vy=abs(b.vy);}
        else if(oldX<bx){b.x=bx-RADIUS-0.1f;b.vx=-abs(b.vx);}
        else {b.x=bx+BRICK_W+RADIUS+0.1f;b.vx=abs(b.vx);}
        --bricks[i];score+=10;
        if(!bricks[i]){--remaining;score+=40;spawnDrop(bx+BRICK_W/2,by+BRICK_H/2);}
        break;
      }
      if(b.y>205 && shields){--shields;b.y=204;b.vy=-abs(b.vy);hitFlash=10;}
      if(b.y>215)b.active=false;
    }
  }
  void tick(int sx) {
    if(wideTicks)--wideTicks;
    if(slowTicks)--slowTicks;
    if(hitFlash)--hitFlash;
    paddle+=constrain(float(sx)/2048,-1.0f,1.0f)*6;
    paddle=constrain(paddle,8+width()/2,312-width()/2);
    if(waiting) {
      balls[0].x=paddle;
      if(launchQueued) {
        waiting=launchQueued=false;balls[0].vx=ballSpeed()*0.35f;
        balls[0].vy=-sqrtf(ballSpeed()*ballSpeed()-balls[0].vx*balls[0].vx);
      }
      return;
    }
    for(auto &b:balls)if(b.active)ballTick(b);
    if(!remaining){advance();return;}
    bool alive=false;for(auto &b:balls)alive|=b.active;
    if(!alive) {
      --hp;hitFlash=30;
      if(hp<=0){over=true;return;}
      wideTicks=slowTicks=shields=0;
      for(auto &drop:drops)drop=Drop{};
      ready();return;
    }
    for(auto &drop:drops)if(drop.active) {
      float oldY=drop.y;drop.y+=1.7f;
      if(oldY-5<=PADDLE_Y+5 && drop.y+5>=PADDLE_Y && abs(drop.x-paddle)<=width()/2+5) {
        grant(drop.power);drop.active=false;
      } else if(drop.y>216)drop.active=false;
    }
  }
#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
  void drawColor(Adafruit_GFX &d) {
    using namespace ColorMenu;
    d.fillScreen(BG);d.setTextWrap(false);
    label(d,10,9,"BLOCK BREAKER",CYAN,2);
    d.setTextSize(1);d.setTextColor(WHITE);d.setCursor(218,13);d.print("SCORE ");d.print(score);
    d.setCursor(10,33);d.print("STAGE ");d.print(stage);d.print("/6  ");d.print(hard?"HARD":"EASY");
    label(d,180,33,"HP",MUTED);
    for(int i=0;i<maxHp;++i) {
      uint16_t color=i<hp?Ink::Red:CARD;int hx=204+i*18;
      d.fillCircle(hx,34,3,color);d.fillCircle(hx+5,34,3,color);
      d.fillTriangle(hx-3,35,hx+8,35,hx+2,41,color);
    }
    d.drawRect(7,45,306,168,hitFlash?Ink::Red:Ink::Wall);
    for(int i=0;i<60;++i)if(bricks[i]) {
      int bx=int(brickX(i)),by=int(brickY(i));uint16_t color=brickColor(i);
      d.fillRoundRect(bx,by,BRICK_W,BRICK_H,2,color);
      d.drawFastHLine(bx+3,by+1,BRICK_W-6,Ink::White);
      for(int n=1;n<bricks[i];++n)d.fillRect(bx+7+n*4,by+5,2,3,BG);
    }
    if(shields)d.drawFastHLine(9,207,302,Ink::Green);
    const char *letters[]={"W","S","M","+","H"};
    const uint16_t colors[]={Ink::Cyan,Ink::Blue,Ink::Gold,Ink::Red,Ink::Green};
    for(auto &drop:drops)if(drop.active && drop.y<207) {
      d.fillRoundRect(int(drop.x)-6,int(drop.y)-6,12,12,2,colors[drop.power]);
      label(d,int(drop.x)-3,int(drop.y)-4,letters[drop.power],BG);
    }
    d.fillRoundRect(int(paddle-width()/2),PADDLE_Y,int(width()),5,2,wideTicks?Ink::Green:Ink::Cyan);
    for(auto &b:balls)if(b.active && b.y<=208) {
      d.fillCircle(int(b.x),int(b.y),3,slowTicks?Ink::Blue:Ink::Gold);
      d.drawPixel(int(b.x)-1,int(b.y)-1,WHITE);
    }
    if(waiting) {
      d.fillRoundRect(88,148,144,32,5,CARD);
      label(d,104,155,"13  LAUNCH BALL",WHITE);
      label(d,101,169,"CATCH FALLING POWERS",MUTED);
    }
    label(d,10,219,"STICK MOVE   13 LAUNCH   12 MENU",WHITE);
    d.setTextSize(1);d.setTextColor(MUTED);d.setCursor(10,232);
    if(wideTicks){d.print("WIDE ");d.print((wideTicks+49)/50);d.print("s  ");}
    if(slowTicks){d.print("SLOW ");d.print((slowTicks+49)/50);d.print("s  ");}
    if(shields){d.print("SHIELD ");d.print(shields);}
    if(!wideTicks && !slowTicks && !shields)d.print("W WIDE  S SLOW  M MULTI  + HP  H SHIELD");
  }
#endif
};
