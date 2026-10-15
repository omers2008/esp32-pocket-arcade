#define ARCADE_TEST_TFT
#include <Arduino.h>
#include "../ESP32_Snake/ArcadeDisplay.h"
#include <cassert>
#include <iostream>
#include <vector>
#define private public
#include "../ESP32_Snake/UwbTagView.h"
#undef private
uint32_t testClock=0;
std::vector<uint8_t> frame(bool rssi=false,int anchor=0,int tag=0x154f,int angle=-30,int cm=250,bool valid=true) {
  std::vector<uint8_t> f(rssi?89:81,0);
  auto word=[&](int at,int value){f[at]=uint8_t(value);f[at+1]=uint8_t(value>>8);};
  f[0]=0x2a;word(1,int(f.size())-5);f[19]=rssi?0x64:1;f[20]=6;
  word(25,anchor);word(27,tag);f[29]=12;f[30]=valid?uint8_t(1<<anchor):0;
  int at=31+(rssi?6:4)*anchor;
  if(anchor<4){word(at,angle);word(at+2,cm);}
  word(rssi?71:63,-32);word(rssi?73:65,245);
  f.back()=0x23;for(size_t i=3;i<f.size()-2;++i)f[f.size()-2]^=f[i];
  return f;
}
void checksum(std::vector<uint8_t> &f){f[f.size()-2]=0;for(size_t i=3;i<f.size()-2;++i)f[f.size()-2]^=f[i];}
bool feed(UwbParser &p,const std::vector<uint8_t> &f,UwbReading &r) {
  bool decoded=false;for(auto b:f)decoded=p.feed(b,r)||decoded;return decoded;
}
void send(UwbTagView &v,const std::vector<uint8_t> &f,uint32_t now){for(auto b:f)v.receive(b,now);}
int main() {
  UwbParser p;UwbReading r;
  for(int mode=0;mode<2;++mode)for(int anchor=0;anchor<4;++anchor) {
    auto f=frame(mode!=0,anchor);p.reset();
    for(size_t i=0;i<f.size()-1;++i)assert(!p.feed(f[i],r));
    assert(p.feed(f.back(),r));
    assert(r.anchor==anchor && r.tag==0x154f && r.sequence==12 && r.valid);
    assert(r.angle==-30 && r.distanceCm==250 && r.rawAngle==-32 && r.rawDistanceCm==245);
  }
  auto f=frame();p.reset();f[40]^=1;assert(!feed(p,f,r) && p.badFrames>0);
  assert(feed(p,frame(),r)); // Re-sync after corrupt payload.
  f=frame();f.back()=0;assert(!feed(p,f,r));assert(feed(p,frame(true),r));
  f=frame();f[19]=2;checksum(f);assert(!feed(p,f,r));
  f=frame();f[20]=2;checksum(f);assert(!feed(p,f,r));
  assert(!feed(p,frame(false,4),r));
  assert(feed(p,frame(false,0,10,181),r) && !r.valid);
  assert(feed(p,frame(false,0,10,-180),r) && r.valid);
  assert(feed(p,frame(false,0,10,0,0),r) && !r.valid);
  assert(feed(p,frame(false,0,10,0,100,false),r) && !r.valid);
  f=frame();f[30]=2;checksum(f);assert(feed(p,f,r) && !r.valid); // Wrong mask bit.
  p.reset();for(int i=0;i<20000;++i)p.feed(uint8_t(i*37),r);
  assert(feed(p,frame(),r));assert(p.bytesSeen==20081);
  p.reset();f=frame();for(int i=0;i<15;++i)p.feed(f[i],r);
  for(int n=0;n<3;++n)feed(p,frame(true),r);
  assert(p.frames>=1); // Recover from a truncated prefix without exceeding the fixed buffer.

  UwbTagView v;v.start();assert(v.status(0)==3 && !v.live(0));
  send(v,frame(),100);v.update(0,0,false,false,100);
  assert(v.live(100) && v.distance==2.5f && v.angle==-30 && v.autoRadius==5 && v.trailCount==1);
  assert(!v.live(1100));v.update(0,0,false,false,1100);assert(!v.positionValid && v.trailCount==0);
  send(v,frame(false,0,0x154f,30,300),1120);assert(v.live(1120));
  send(v,frame(false,0,0x154f,30,300,false),1130);
  assert(!v.live(1130) && !v.positionValid); // Invalid report must remove the old live target.
  send(v,frame(false,0,0x154f,30,300),1140);
  send(v,frame(false,1,0x9876,-20,150),1150);
  v.update(0,0,true,false,1150);assert(v.tags[v.selected].reading.tag==0x9876 && v.angle==-20);
  v.update(0,0,false,true,1150);assert(v.smooth);
  send(v,frame(false,1,0x9876,20,250),1250);
  assert(v.angle>-20 && v.angle<20 && v.distance>1.5f && v.distance<2.5f);
  // Wrap-aware smoothing takes the short way around +/-180 degrees.
  v.resetPosition();send(v,frame(false,1,0x9876,179,250),1300);
  send(v,frame(false,1,0x9876,-179,250),1400);assert(abs(v.angle)>175);
  v.update(0,0,false,false,1400);v.update(1500,0,false,false,1400);assert(v.scaleIndex==1);
  v.update(1500,0,false,false,1420);assert(v.scaleIndex==1); // One step per tilt.
  v.update(0,0,false,false,1440);v.update(0,1500,false,false,1460);assert(v.showRaw);
  // Bounded tag table retains the selected target when new identities arrive.
  uint16_t chosen=v.tags[v.selected].reading.tag;
  for(int i=0;i<20;++i)send(v,frame(false,0,100+i,0,100),1500+i);
  assert(v.tags[v.selected].reading.tag==chosen);
  int count=0;for(auto &tag:v.tags)count+=tag.used;assert(count==8);
  int px,py;UwbTagView::point(-30,2,5,px,py);assert(px<104 && py<180);
  UwbTagView::point(30,2,5,px,py);assert(px>104 && py<180);
  assert(!UwbTagView::inView(100,1,2) && !UwbTagView::inView(0,3,2));

  ArcadeDisplay display;assert(display.begin());
  v.start();int transactions=tftTrace.transactions;v.draw(display,2000);
  assert(tftTrace.transactions==transactions+6);
  send(v,frame(),2020);v.update(0,0,false,false,2020);
  auto bytes=v.parser.bytesSeen;v.draw(display,2120);
  assert(v.parser.bytesSeen==bytes && v.live(2120) && !tftTrace.writing);
  std::cout<<"PASS: both UART formats, signed fields, checksums, re-sync, staleness, smoothing, tags, controls and TFT rendering.\n";
}
