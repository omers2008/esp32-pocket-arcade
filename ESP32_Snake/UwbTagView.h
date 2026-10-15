#pragma once
#include <Arduino.h>
#include <math.h>
#include "ArcadeDisplay.h"
#include "UwbProtocol.h"

// Passive tag viewer; UART ownership stays in the sketch. No module commands.
class UwbTagView {
 public:
  static constexpr int RX_PIN=16;
  static constexpr uint32_t BAUD=115200;
  void start() {
    parser.reset();for(auto &tag:tags)tag=Tag{};
    selected=-1;scaleIndex=0;autoRadius=2;smooth=showRaw=false;
    stickReady=false;dirty=true;lastDraw=0;lastStatus=-1;resetPosition();
  }
  void receive(uint8_t byte,uint32_t now) {
    UwbReading reading;
    if(!parser.feed(byte,reading))return;
    int slot=-1;
    for(int i=0;i<MAX_TAGS;++i)if(tags[i].used && tags[i].reading.tag==reading.tag && tags[i].reading.anchor==reading.anchor){slot=i;break;}
    if(slot<0)for(int i=0;i<MAX_TAGS;++i)if(!tags[i].used){slot=i;break;}
    if(slot<0) {
      uint32_t age=0;
      for(int i=0;i<MAX_TAGS;++i)if(i!=selected && (slot<0 || now-tags[i].stamp>age)){slot=i;age=now-tags[i].stamp;}
    }
    tags[slot]={reading,now,true};
    if(selected<0)selected=slot;
    if(slot==selected) {
      if(reading.valid)updatePosition(reading,now);
      else resetPosition();
    }
    dirty=true;
  }
  void update(int sx,int sy,bool nextTag,bool toggleSmooth,uint32_t now) {
    if(!live(now))resetPosition();
    if(nextTag && selected>=0) {
      for(int n=1;n<=MAX_TAGS;++n) {
        int i=(selected+n)%MAX_TAGS;
        if(tags[i].used){selected=i;resetPosition();if(live(now))updatePosition(tags[i].reading,tags[i].stamp);break;}
      }
      dirty=true;
    }
    if(toggleSmooth){smooth=!smooth;resetPosition();if(live(now))updatePosition(tags[selected].reading,tags[selected].stamp);dirty=true;}
    if(abs(sx)<350 && abs(sy)<350)stickReady=true;
    if(stickReady && (abs(sx)>650 || abs(sy)>650)) {
      stickReady=false;
      if(abs(sx)>=abs(sy))scaleIndex=(scaleIndex+SCALE_COUNT+(sx>0?1:-1))%SCALE_COUNT;
      else showRaw=!showRaw;
      dirty=true;
    }
    if(live(now) && scaleIndex==0) {
      float needed=distance;
      if(showRaw)needed=max(needed,tags[selected].reading.rawDistanceCm/100.0f);
      if(needed>autoRadius*0.92f) {
        autoRadius=750;
        for(int i=1;i<SCALE_COUNT;++i)if(scale(i)>needed*1.15f){autoRadius=scale(i);break;}
      }
    }
  }
  void draw(ArcadeDisplay &display,uint32_t now) {
    int state=status(now);
    if(lastDraw && now-lastDraw<100)return;
    if(!dirty && state==lastStatus && now-lastDraw<500)return;
    display.setGame(-1);
#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
    if(!display.colorFrame([&](Adafruit_GFX &d){drawColor(d,now);}))return;
#else
    display.clearDisplay();display.setTextColor(SSD1306_WHITE);display.setTextSize(1);
    display.setCursor(0,0);display.print(F("UWB TAG VIEW"));display.display();
#endif
    dirty=false;lastDraw=now;lastStatus=state;
  }

 private:
  static constexpr int MAX_TAGS=8,TRAIL=32,SCALE_COUNT=10;
  struct Tag {UwbReading reading;uint32_t stamp=0;bool used=false;};
  struct Point {float angle=0,distance=0;uint32_t stamp=0;};
  UwbParser parser;Tag tags[MAX_TAGS];Point trail[TRAIL];
  int selected=-1,scaleIndex=0,trailHead=0,trailCount=0,lastStatus=-1;
  bool smooth=false,showRaw=false,positionValid=false,stickReady=false,dirty=true;
  float angle=0,distance=0,autoRadius=2;
  uint32_t positionStamp=0,lastDraw=0;
  static float scale(int i){const float values[]={0,1,2,5,10,20,50,100,200,700};return values[i];}
  bool live(uint32_t now) const {return selected>=0 && tags[selected].used && tags[selected].reading.valid && now-tags[selected].stamp<1000;}
  int status(uint32_t now) const {
    if(live(now))return 0;
    if(selected>=0)return 1;
    return parser.bytesSeen?2:3;
  }
  void resetPosition(){positionValid=false;trailCount=trailHead=0;}
  void updatePosition(const UwbReading &r,uint32_t now) {
    float nextAngle=float(r.angle),nextDistance=r.distanceCm/100.0f;
    if(positionValid && smooth && now-positionStamp<1000) {
      float alpha=1-expf(-float(now-positionStamp)/250.0f);
      float delta=fmodf(nextAngle-angle+540.0f,360.0f)-180.0f;
      nextAngle=angle+alpha*delta;nextDistance=distance+alpha*(nextDistance-distance);
    }
    angle=fmodf(nextAngle+540.0f,360.0f)-180.0f;distance=nextDistance;
    positionStamp=now;positionValid=true;
    trail[trailHead]={angle,distance,now};trailHead=(trailHead+1)%TRAIL;trailCount=min(TRAIL,trailCount+1);
  }
#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
  static constexpr uint16_t BG=0x08A4,PANEL=0x1106,GRID=0x29CA,TEAL=0x5F18,INK=0xF7BF,MUTED=0x9517,AMBER=0xFE90;
  static void label(Adafruit_GFX &d,int x,int y,const char *s,uint16_t c,int size=1) {
    d.setTextSize(size);d.setTextColor(c);d.setCursor(x,y);d.print(s);
  }
  static void point(float a,float range,float radius,int &x,int &y) {
    float radians=a*0.01745329252f;x=104+int(sinf(radians)*range*88/radius);y=180-int(cosf(radians)*range*88/radius);
  }
  static bool inView(float a,float range,float radius){return abs(a)<=90 && range>0 && range<=radius;}
  void drawColor(Adafruit_GFX &d,uint32_t now) const {
    d.fillScreen(BG);d.setTextWrap(false);
    label(d,10,10,"TAG VIEW",INK,2);
    int state=status(now);
    d.fillRoundRect(233,8,77,20,4,PANEL);
    label(d,241,14,state==0?"LIVE":state==1?"NO SIGNAL":state==2?"FORMAT?":"NO DATA",state==0?TEAL:AMBER);
    char text[48];
    if(selected>=0)snprintf(text,sizeof(text),"TAG 0x%04X / A%u",unsigned(tags[selected].reading.tag),unsigned(tags[selected].reading.anchor));
    else snprintf(text,sizeof(text),"WAITING FOR GEN2 TAG");
    label(d,10,36,text,MUTED);label(d,223,36,"UART 115200",MUTED);
    label(d,12,58,"RELATIVE POSITION",MUTED);
    float radius=scaleIndex?scale(scaleIndex):autoRadius;
    for(int ring=1;ring<=4;++ring) {
      float rr=88*ring/4.0f;
      for(int a=-90;a<90;a+=4) {
        float t=a*0.01745329252f,u=(a+4)*0.01745329252f;
        d.drawLine(104+int(sinf(t)*rr),180-int(cosf(t)*rr),104+int(sinf(u)*rr),180-int(cosf(u)*rr),GRID);
      }
    }
    for(int a=-90;a<=90;a+=30) {
      int px,py;point(float(a),radius,radius,px,py);d.drawLine(104,180,px,py,GRID);
      if(a>-90 && a<90){snprintf(text,sizeof(text),"%+d",a);label(d,px-8,py-18,text,MUTED);}
    }
    bool isLive=live(now) && positionValid;
    if(isLive && inView(angle,distance,radius)) {
      for(int n=0;n<trailCount;++n) {
        const Point &p=trail[(trailHead+TRAIL-trailCount+n)%TRAIL];
        if(now-p.stamp<2000 && inView(p.angle,p.distance,radius)) {
          int px,py;point(p.angle,p.distance,radius,px,py);d.fillCircle(px,py,1,0x338D);
        }
      }
      int px,py;point(angle,distance,radius,px,py);
      d.drawLine(104,180,px,py,TEAL);d.drawCircle(px,py,7,TEAL);d.fillCircle(px,py,3,INK);
    } else {
      label(d,44,133,isLive?"OUTSIDE VIEW":"WAITING FOR TAG",isLive?AMBER:MUTED);
      if(!isLive)label(d,30,145,parser.bytesSeen?"CHECK OUTPUT / TAG":"CHECK POWER / UART",MUTED);
    }
    if(isLive && showRaw) {
      const UwbReading &r=tags[selected].reading;
      float rawDistance=r.rawDistanceCm/100.0f;
      if(inView(float(r.rawAngle),rawDistance,radius)) {
        int px,py;point(float(r.rawAngle),rawDistance,radius,px,py);d.drawCircle(px,py,4,AMBER);
      }
    }
    d.fillTriangle(99,188,104,176,109,188,TEAL);label(d,86,195,"ANCHOR",TEAL);
    d.fillRoundRect(212,54,98,60,5,PANEL);label(d,221,62,"DISTANCE",MUTED);
    if(isLive)snprintf(text,sizeof(text),"%.2f",double(distance));else snprintf(text,sizeof(text),"--");
    label(d,221,80,text,TEAL,2);label(d,290,102,"m",MUTED);
    d.fillRoundRect(212,120,98,60,5,PANEL);label(d,221,128,"ANGLE",MUTED);
    if(isLive)snprintf(text,sizeof(text),"%+.0f",double(angle));else snprintf(text,sizeof(text),"--");
    label(d,221,146,text,INK,2);label(d,276,153,"deg",MUTED);
    label(d,221,169,isLive?(abs(angle)<0.5f?"FORWARD":angle<0?"LEFT":"RIGHT"):"--",MUTED);
    if(isLive && showRaw) {
      const UwbReading &r=tags[selected].reading;
      snprintf(text,sizeof(text),"RAW %.2fm",r.rawDistanceCm/100.0);label(d,215,187,text,AMBER);
      snprintf(text,sizeof(text),"%+d deg",int(r.rawAngle));label(d,215,198,text,AMBER);
    } else {
      snprintf(text,sizeof(text),"BAD %lu",static_cast<unsigned long>(parser.badFrames));label(d,215,187,text,MUTED);
      if(selected>=0)snprintf(text,sizeof(text),"AGE %.1fs",double(now-tags[selected].stamp)/1000);
      else snprintf(text,sizeof(text),"RX %luB",static_cast<unsigned long>(parser.bytesSeen));
      label(d,215,198,text,MUTED);
    }
    snprintf(text,sizeof(text),"%s %gm  RAW %s",scaleIndex?"RANGE":"AUTO",double(radius),showRaw?"ON":"OFF");label(d,10,210,text,MUTED);
    label(d,215,210,smooth?"SMOOTH ON":"SMOOTH OFF",smooth?TEAL:MUTED);
    d.drawFastHLine(10,220,300,GRID);
    label(d,10,224,"13 TAG   14 SMOOTH   12 BACK",INK);
    label(d,10,233,"L/R RANGE   U/D RAW   AHEAD=0 DEG",MUTED);
  }
#endif
};
